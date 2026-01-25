#include "cam.h"
#include <esp_log.h>
#include <esp_system.h>
#include <nvs_flash.h>
#include <sys/param.h>
#include <string.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "esp_camera.h"
#include "iot_button.h"
#include "esp_timer.h"
#include "esp_jpeg_common.h"
#include "esp_jpeg_dec.h"
#include "esp_lvgl_port.h"

// 请确保这些头文件路径在您的项目中正确
#include "camera_pinout.h"
#include "ui.h"
#include "button_gpio.h"
#include "screens.h"
#include "vars.h"
#include "poker.h"
#include "myenc.h"

#include <dirent.h>

#include <sys/stat.h>
#include "esp_heap_caps.h"

#define IMAGE_EXT       ".jpg"



static const char *TAG = "CAM_CTRL";

// --- 全局变量与状态控制 ---
button_handle_t gpio_btn = NULL;
bool g_camera_streaming = false;    // UI控制开关：true=开启预览, false=停止预览
volatile bool g_take_photo = false; // 拍照标志位

static lv_image_dsc_t cam_dsc;       // LVGL 图像描述符
static uint8_t *cam_buf = NULL;      // 存放在 PSRAM 的显存 Buffer
static lv_obj_t *cam_img_obj = NULL; // 预览用的 LVGL image 对象

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
    .xclk_freq_hz = 20000000, // S3 通常建议 20M 比较稳定
    .ledc_timer = LEDC_TIMER_0,
    .ledc_channel = LEDC_CHANNEL_0,
    .pixel_format = PIXFORMAT_JPEG,
    .frame_size = FRAMESIZE_P_HD,
    .jpeg_quality = 12,
    .fb_count = 2, // 预览流建议使用双缓冲
    .fb_location = CAMERA_FB_IN_PSRAM,
    .grab_mode = CAMERA_GRAB_WHEN_EMPTY,

};

// --- 生成时间戳文件名 ---
static void get_uptime_hhmmss(char *out, size_t len)
{
    int64_t sec = esp_timer_get_time() / 1000000;
    snprintf(out, len, "%02d%02d%02d", (int)(sec / 3600) % 24, (int)(sec / 60) % 60, (int)sec % 60);
}




// 


/* 判断文件是否存在 */
static bool file_exists(const char *path)
{
    struct stat st;
    return (stat(path, &st) == 0);
}

/* 生成 000000.jpg ~ 999999.jpg 中第一个可用名 */
bool generate_next_image_name_fast(const char *dir_path,
                                   char *out_name,
                                   size_t out_len)
{
    if (out_len < 13) return false;

    char fullpath[128];

    for (int i = 0; i <= 999999; i++) {
        snprintf(out_name, out_len, "%06d%s", i, IMAGE_EXT);
        snprintf(fullpath, sizeof(fullpath), "%s/%s", dir_path, out_name);

        if (!file_exists(fullpath)) {
            return true;  // 找到空位
        }
    }

    return false; // 全满
}


// --- 保存照片到 SD 卡 ---
static void save_to_sd(camera_fb_t *fb)
{
    char ts[16], filename[64] ,poker_filename[64];
    // get_uptime_hhmmss(ts, sizeof(ts)); // 删除时间戳命名，使用新的命名方式
    generate_next_image_name_fast("/sdcard", ts, sizeof(ts)); //新的命名方式

    snprintf(filename, sizeof(filename), "/sdcard/%s", ts);
     snprintf(poker_filename, sizeof(poker_filename), "/sdcard/POK%d_%s", (int)get_var_filter_id(),ts);

    set_var_shot_info("拍照中");

    FILE *f = fopen(filename, "wb");
    if (f)
    {
        fwrite(fb->buf, 1, fb->len, f);

        fclose(f);
        ESP_LOGI(TAG, "照片已保存: %s", filename);
        // set_var_shot_info("拍照完成,照片已保存到");
         set_var_shot_info(filename);

         // 发送信号
         send_jpeg_process_job(filename,poker_filename,get_var_filter_id()) ;

    }
    else
    {
        ESP_LOGE(TAG, "文件保存失败，请检查文件系统或SD卡");
    }

    set_var_shot_info(filename);
}

// --- 按钮单击回调 ---
static void button_shot_cb(void *arg, void *usr_data)
{
    if (g_camera_streaming)
    {
        ESP_LOGI(TAG, "触发拍照...");
        g_take_photo = true;
    }
}

/**
 * 相机渲染与处理任务
 * @param pvParam 指向 LVGL 父容器对象 (lv_obj_t*)
 */
void cam_render_task(void *pvParam)
{
    lv_obj_t *parent = (lv_obj_t *)pvParam;
    if (!parent)
    {
        ESP_LOGE(TAG, "错误：未传入有效的 LVGL 容器对象");
        vTaskDelete(NULL);
        return;
    }

    // 创建 LVGL 图像显示对象（需持有 LVGL 锁）
    if (lvgl_port_lock(pdMS_TO_TICKS(50)))
    {
        cam_img_obj = lv_image_create(parent);
        lv_obj_center(cam_img_obj);
        lv_image_set_rotation(cam_img_obj, -900); // 900 = 90°

        lv_obj_add_flag(cam_img_obj, LV_OBJ_FLAG_HIDDEN);
        lvgl_port_unlock();
    }
    else
    {
        ESP_LOGE(TAG, "创建预览 image 失败，获取 LVGL 锁超时");
        vTaskDelete(NULL);
        return;
    }

    // 1. 在 PSRAM 分配最终显示 Buffer (144x256, RGB565 每个像素2字节)
    cam_buf = (uint8_t *)heap_caps_malloc(144 * 256 * 2, MALLOC_CAP_SPIRAM);
    if (!cam_buf)
    {
        ESP_LOGE(TAG, "cam_buf 分配失败");
        vTaskDelete(NULL);
        return;
    }

    // 2. 初始化显示描述符
    cam_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
    cam_dsc.header.w = 144;
    cam_dsc.header.h = 256;
    cam_dsc.header.stride = 144 * 2;
    cam_dsc.data_size = 144 * 256 * 2;
    cam_dsc.data = cam_buf;

    while (1)
    {
        // 如果开关关闭，隐藏图像并休眠
        if (!g_camera_streaming)
        {
            if (lvgl_port_lock(pdMS_TO_TICKS(10)))
            {
                lv_obj_add_flag(cam_img_obj, LV_OBJ_FLAG_HIDDEN);
                lvgl_port_unlock();
            }
            vTaskDelay(pdMS_TO_TICKS(100));
            continue;
        }
        if (lvgl_port_lock(pdMS_TO_TICKS(10)))
        {
            lv_obj_remove_flag(cam_img_obj, LV_OBJ_FLAG_HIDDEN);
            lvgl_port_unlock();
        }

        // 获取相机帧
        camera_fb_t *fb = esp_camera_fb_get();
        if (!fb)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        // --- 拍照逻辑 ---
        if (g_take_photo)
        {
            save_to_sd(fb);
            g_take_photo = false;
        }

        // --- 解码并推送到 UI ---
        jpeg_dec_config_t config = DEFAULT_JPEG_DEC_CONFIG();
        config.output_type = JPEG_PIXEL_FORMAT_RGB565_LE;
        config.scale.width = 144;
        config.scale.height = 256;

        jpeg_dec_handle_t dec;
        jpeg_dec_io_t io = {.inbuf = fb->buf, .inbuf_len = fb->len};
        jpeg_dec_header_info_t info;

        if (jpeg_dec_open(&config, &dec) == JPEG_ERR_OK)
        {
            if (jpeg_dec_parse_header(dec, &io, &info) == JPEG_ERR_OK)
            {
                uint8_t *decoded = jpeg_calloc_align(144 * 256 * 2, 16);
                if (decoded)
                {
                    io.outbuf = decoded;
                    if (jpeg_dec_process(dec, &io) == JPEG_ERR_OK)
                    {

                        // 拷贝解码数据到预览 Buffer
                        memcpy(cam_buf, decoded, 144 * 256 * 2);


                        // poker预览

                        poker_apply_filter(
                            (uint16_t *)cam_buf,
                            144,
                            256,
                            get_var_filter_id() // 你 UI 当前选择的滤镜
                        );

                        




                        // 更新 LVGL 对象源并重绘
                        if (lvgl_port_lock(pdMS_TO_TICKS(10)))
                        {
                            lv_image_set_src(cam_img_obj, &cam_dsc);
                            lv_obj_invalidate(cam_img_obj);
                            lvgl_port_unlock();
                        }
                    }
                    jpeg_free_align(decoded);
                }
            }
            jpeg_dec_close(dec);
        }

        // 释放相机 Buffer
        esp_camera_fb_return(fb);
        vTaskDelay(pdMS_TO_TICKS(1)); // 防止任务饿死
    }
}

// --- 初始化与任务创建 ---
void cam_init_and_start(lv_obj_t *ui_container)
{
    // 1. 相机硬件初始化
    if (esp_camera_init(&camera_config) != ESP_OK)
    {
        ESP_LOGE(TAG, "相机初始化失败");
        return;
    }

    // 添加代码
    sensor_t *s = esp_camera_sensor_get();
    if (s)
    {
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
    if (gpio_btn)
    {
        iot_button_register_cb(gpio_btn, BUTTON_SINGLE_CLICK, NULL, button_shot_cb, NULL);
    }

    // 3. 开启相机渲染处理任务 (绑定到 Core 1 以免影响 UI 交互)
    // xTaskCreatePinnedToCore(cam_render_task, "cam_task", 1024 * 20, ui_container, 5, NULL, 1);


    xTaskCreatePinnedToCoreWithCaps(
    cam_render_task,          // 任务函数
    "cam_task",               // 任务名
    1024 * 20,                // 栈大小（单位：word，不是字节！）
    ui_container,             // 参数
    5,                        // 优先级
    NULL,                     // TaskHandle_t*
    1,                        // 绑定 CPU1
    MALLOC_CAP_SPIRAM         // 👈 强制栈从 PSRAM 分配
);



}
