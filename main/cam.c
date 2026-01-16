#include "cam.h"
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/queue.h"

#include "esp_camera.h"
#include "iot_button.h"
#include "esp_timer.h"
#include "esp_jpeg_common.h"
#include "esp_lvgl_port.h"

// 请确保这些头文件路径在您的项目中正确
#include "camera_pinout.h"
#include "ui.h"
#include "button_gpio.h"
#include "screens.h"
#include "vars.h"
#include "ws2812.h"
#include "esp_jpeg_enc.h"
#include "poker.h"


static const char *TAG = "CAM_CTRL";

// --- 全局变量与状态控制 ---
button_handle_t gpio_btn = NULL;
bool g_camera_streaming = false;      // UI控制开关：true=开启预览, false=停止预览
volatile bool g_take_photo = false;   // 拍照标志位

static lv_image_dsc_t cam_dsc;        // LVGL 图像描述符
static uint8_t *cam_buf = NULL;       // 存放在 PSRAM 的显存 Buffer
static lv_obj_t *cam_img_obj = NULL;  // ???????????? LVGL image ??????
static QueueHandle_t save_queue = NULL;

typedef struct {
    uint16_t *buf;
    int w;
    int h;
} save_job_t;
  // 预览用的 LVGL image 对象

// --- 相机硬件配置 ---
static camera_config_t camera_config = {
    .pin_pwdn = CAM_PIN_PWDN,
    .pin_reset = CAM_PIN_RESET,
    .pin_xclk = CAM_PIN_XCLK,
    .pin_sccb_sda = CAM_PIN_SIOD,
    .pin_sccb_scl = CAM_PIN_SIOC,
    .pin_d7 = CAM_PIN_D7,
    .pin_d6 = CAM_PIN_D6,
    .pin_d5 = CAM_PIN_D5,
    .pin_d4 = CAM_PIN_D4,
    .pin_d3 = CAM_PIN_D3,
    .pin_d2 = CAM_PIN_D2,
    .pin_d1 = CAM_PIN_D1,
    .pin_d0 = CAM_PIN_D0,
    .pin_vsync = CAM_PIN_VSYNC,
    .pin_href = CAM_PIN_HREF,
    .pin_pclk = CAM_PIN_PCLK,
    .xclk_freq_hz = 20000000,           // S3 通常建议 20M 比较稳定
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_RGB565, 
    .frame_size = FRAMESIZE_SVGA,        
    .jpeg_quality = 12, 
    .fb_count = 2,                      // 预览流建议使用双缓冲
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,
    
};

// --- 生成时间戳文件名 ---
static void get_uptime_hhmmss(char *out, size_t len) {
    int64_t sec = esp_timer_get_time() / 1000000;
    snprintf(out, len, "%02d%02d%02d", (int)(sec/3600)%24, (int)(sec/60)%60, (int)sec%60);
}

// --- 保存照片到 SD 卡 ---
static void save_to_sd_rgb565(const uint16_t *rgb565, int w, int h)
{

     ESP_LOGI("PIC","进入保存sd卡函数");
    char ts[16], filename[64];
    get_uptime_hhmmss(ts, sizeof(ts));
    snprintf(filename, sizeof(filename), "/sdcard/IMG_%s.jpg", ts);

    set_var_shot_info("SHOT");

    const int pixel_count = w * h;
    uint8_t *rgb888 = heap_caps_malloc(
        pixel_count * 3,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );


    ESP_LOGI(TAG, "Free heap before malloc: %u", esp_get_free_heap_size());

    if (!rgb888) {

         ESP_LOGI(TAG, "分配失败");

        return;
    }

    rgb565_to_rgb888(rgb565, rgb888, pixel_count);

    jpeg_enc_config_t cfg = {
        .src_type    = JPEG_PIXEL_FORMAT_RGB888,
        .subsampling = JPEG_SUBSAMPLE_420,
        .quality     = 80,
        .width       = w,
        .height      = h,
    };

    jpeg_enc_handle_t enc = NULL;
    if (jpeg_enc_open(&cfg, &enc) != JPEG_ERR_OK) {
        heap_caps_free(rgb888);
        return;
    }

    uint8_t *jpeg_buf = heap_caps_malloc(
        w * h * 3,
        MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT
    );
    if (!jpeg_buf) {
        jpeg_enc_close(enc);
        heap_caps_free(rgb888);
        return;
    }

    int jpeg_len = 0;
    if (jpeg_enc_process(
            enc,
            rgb888, pixel_count * 3,
            jpeg_buf, w * h * 3,
            &jpeg_len
        ) == JPEG_ERR_OK) {
        FILE *f = fopen(filename, "wb");
        if (f) {
            fwrite(jpeg_buf, 1, jpeg_len, f);
            fclose(f);
            ESP_LOGI(TAG, "Saved photo %s", filename);
        } else {
            ESP_LOGE(TAG, "Failed to save file to SD");
        }
    }

    jpeg_enc_close(enc);
    heap_caps_free(rgb888);
    heap_caps_free(jpeg_buf);

    set_var_shot_info(filename);
        ESP_LOGI("PIC","退出保存到sd卡函数");
}

// --- 按钮单击回调 ---
static void button_shot_cb(void *arg, void *usr_data) {
    if (g_camera_streaming) {
        ESP_LOGI(TAG, "触发拍照...");
        g_take_photo = true; 
    }
}


static void save_filtered_frame(uint16_t *rgb565, int w, int h)
{
    save_to_sd_rgb565(rgb565, w, h);
}

static void save_task(void *arg)
{
    save_job_t job;
    while (1) {
        if (xQueueReceive(save_queue, &job, portMAX_DELAY) == pdTRUE) {
            save_to_sd_rgb565(job.buf, job.w, job.h);
            heap_caps_free(job.buf);
        }
    }
}

void rgb565_to_rgb888(
    const uint16_t *src,
    uint8_t *dst,
    int pixel_count)
{
    for (int i = 0; i < pixel_count; i++) {
        uint16_t c = __builtin_bswap16(src[i]); 

        uint8_t r5 = (c >> 11) & 0x1F;
        uint8_t g6 = (c >> 5)  & 0x3F;
        uint8_t b5 =  c        & 0x1F;

        // 映射到 888
        dst[i*3 + 0] = (r5 << 3) | (r5 >> 2);
        dst[i*3 + 1] = (g6 << 2) | (g6 >> 4);
        dst[i*3 + 2] = (b5 << 3) | (b5 >> 2);
    }
}

static void resize_rgb565_nn(const uint16_t *src, int src_w, int src_h,
                             uint16_t *dst, int dst_w, int dst_h)
{
    for (int y = 0; y < dst_h; y++) {
        int sy = (y * src_h) / dst_h;
        const uint16_t *src_row = src + (sy * src_w);
        uint16_t *dst_row = dst + (y * dst_w);
        for (int x = 0; x < dst_w; x++) {
            int sx = (x * src_w) / dst_w;
            // 关键修改：在这里进行字节交换
            // __builtin_bswap16 会把 [R5G3, G3B5] 变成 [G3B5, R5G3]
            dst_row[x] = __builtin_bswap16(src_row[sx]);
        }
    }
}



/**
 * 相机渲染与处理任务
 * @param pvParam 指向 LVGL 父容器对象 (lv_obj_t*)
 */
void cam_render_task(void *pvParam) {
    lv_obj_t *parent = (lv_obj_t *)pvParam;
    if (!parent) {
        ESP_LOGE(TAG, "错误：未传入有效的 LVGL 容器对象");
        vTaskDelete(NULL);
        return;
    }

    // 创建 LVGL 图像显示对象（需持有 LVGL 锁）
    if (lvgl_port_lock(pdMS_TO_TICKS(50))) {
        cam_img_obj = lv_image_create(parent);
        lv_obj_center(cam_img_obj);
            lv_image_set_rotation(cam_img_obj, 900); // 900 = 90°

        lv_obj_add_flag(cam_img_obj, LV_OBJ_FLAG_HIDDEN);
        lvgl_port_unlock();
    } else {
        ESP_LOGE(TAG, "创建预览 image 失败，获取 LVGL 锁超时");
        vTaskDelete(NULL);
        return;
    }

    // 1. 在 PSRAM 分配最终显示 Buffer (144x256, RGB565 每个像素2字节)
    cam_buf = (uint8_t *)heap_caps_malloc(144 * 256 * 2, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);

    if (!cam_buf) {
        ESP_LOGE(TAG, "cam_buf 分配失败");
        vTaskDelete(NULL);
        return;
    }
    
    // 2. 初始化显示描述符
    cam_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
    cam_dsc.header.w  = 144;
    cam_dsc.header.h  = 256;
    cam_dsc.header.stride = 144 * 2;
    cam_dsc.data_size = 144 * 256 * 2;
    cam_dsc.data = cam_buf;

    while (1) {
        // 如果开关关闭，隐藏图像并休眠
        if (!g_camera_streaming) {
            if (lvgl_port_lock(pdMS_TO_TICKS(10))) {
                lv_obj_add_flag(cam_img_obj, LV_OBJ_FLAG_HIDDEN);
                lvgl_port_unlock();
            }
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        if (lvgl_port_lock(pdMS_TO_TICKS(10))) {
            lv_obj_remove_flag(cam_img_obj, LV_OBJ_FLAG_HIDDEN);
            lvgl_port_unlock();
        }

        // 获取相机帧
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb) {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        // --- filter source frame (RGB565) ---
        uint16_t *src_rgb565 = (uint16_t *)fb->buf;
        const int src_w = fb->width;
        const int src_h = fb->height;
        poker_apply_filter(src_rgb565, src_w, src_h, get_var_filter_id());

        // --- photo logic ---
        if (g_take_photo) {
            if (!save_queue) {
                ESP_LOGE(TAG, "save queue not ready");
                g_take_photo = false;
            } else {
                const size_t bytes = (size_t)src_w * (size_t)src_h * 2;
                uint16_t *copy = heap_caps_malloc(bytes, MALLOC_CAP_SPIRAM | MALLOC_CAP_8BIT);
                if (copy) {
                    memcpy(copy, src_rgb565, bytes);
                    save_job_t job = {
                        .buf = copy,
                        .w = src_w,
                        .h = src_h,
                    };
                    if (xQueueSend(save_queue, &job, 0) != pdTRUE) {
                        ESP_LOGW(TAG, "save queue full, drop frame");
                        heap_caps_free(copy);
                    }
                } else {
                    ESP_LOGE(TAG, "no memory for save buffer");
                }
                g_take_photo = false;
            }
        }

        // --- scale to UI ---
        resize_rgb565_nn(src_rgb565, src_w, src_h, (uint16_t *)cam_buf, 144, 256);

        // update LVGL image source and redraw
        if (lvgl_port_lock(pdMS_TO_TICKS(10))) {
            lv_image_set_src(cam_img_obj, &cam_dsc);
            lv_obj_invalidate(cam_img_obj);
            lvgl_port_unlock();
        }

        esp_camera_fb_return(fb);
        vTaskDelay(pdMS_TO_TICKS(1)); // 防止任务饿死
    }
}

// --- 初始化与任务创建 ---
void cam_init_and_start(lv_obj_t *ui_container) {
    // 1. 相机硬件初始化
    if (esp_camera_init(&camera_config) != ESP_OK) {
        ESP_LOGE(TAG, "相机初始化失败");
        return;
    }


    // 添加代码
       sensor_t *s = esp_camera_sensor_get();
    if (s) {
        s->set_hmirror(s, 0); // 关键
        s->set_vflip(s, 1);   // 视情况
    
    }





    // 2. 初始化物理按键 (GPIO 0)
    const button_config_t btn_cfg = {0};
    const button_gpio_config_t btn_gpio_cfg = {
        .gpio_num = 0,
        .active_level = 0,
    };
    iot_button_new_gpio_device(&btn_cfg, &btn_gpio_cfg, &gpio_btn);
    if (gpio_btn) {
        iot_button_register_cb(gpio_btn, BUTTON_SINGLE_CLICK, NULL, button_shot_cb, NULL);
    }

    if (!save_queue) {
        save_queue = xQueueCreate(2, sizeof(save_job_t));
        if (save_queue) {
            xTaskCreatePinnedToCore(save_task, "save_task", 1024 * 8, NULL, 4, NULL, 0);
        } else {
            ESP_LOGE(TAG, "save queue create failed");
        }
    }

    // 3. 开启相机渲染处理任务 (绑定到 Core 1 以免影响 UI 交互)
    xTaskCreatePinnedToCore(cam_render_task, "cam_task", 8192 * 4, ui_container, 5, NULL, 1);
}
