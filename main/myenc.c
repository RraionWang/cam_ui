#include "myenc.h"

#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "esp_heap_caps.h"

#include "esp_jpeg_dec.h"
#include "esp_jpeg_enc.h"

#include "poker.h"
#include "freertos/event_groups.h"
#include "freertos/queue.h"
#include "screens.h"
#include "esp_lvgl_port.h"



#define JPEG_PATH_MAX 128

static QueueHandle_t jpeg_job_queue = NULL;





static EventGroupHandle_t jpeg_event = NULL;
#define JPEG_EVENT_DONE (1 << 0)


typedef struct {
    char in_path[JPEG_PATH_MAX];
    char out_path[JPEG_PATH_MAX];
    poker_filter_t filter_id;
} jpeg_job_t;




static esp_err_t process_jpeg_with_filter(
    const char *in_path,
    const char *out_path,
    poker_filter_t filter_id)
{
    ESP_LOGI("JPEG_TASK", "Processing %s", in_path);

    esp_err_t ret = ESP_FAIL;
    FILE *f = NULL;
    FILE *fo = NULL;
    uint8_t *jpeg_in = NULL;
    uint16_t *rgb565 = NULL;
    uint8_t *rgb888 = NULL;
    uint8_t *jpeg_out = NULL;
    jpeg_dec_handle_t dec = NULL;
    jpeg_enc_handle_t enc = NULL;
    int jpeg_len = 0;
    size_t jpeg_size = 0;
    jpeg_dec_header_info_t info = {0};

    /* ---------- read JPEG ---------- */
    f = fopen(in_path, "rb");
    if (!f) {
        ESP_LOGE("JPEG_TASK", "open input failed");
        goto cleanup;
    }

    fseek(f, 0, SEEK_END);
    long file_size = ftell(f);
    if (file_size <= 0) {
        ESP_LOGE("JPEG_TASK", "invalid jpeg size");
        goto cleanup;
    }
    jpeg_size = (size_t)file_size;
    fseek(f, 0, SEEK_SET);

    jpeg_in = heap_caps_malloc(jpeg_size, MALLOC_CAP_SPIRAM);
    if (!jpeg_in) {
        ESP_LOGE("JPEG_TASK", "malloc jpeg_in failed");
        goto cleanup;
    }
    if (fread(jpeg_in, 1, jpeg_size, f) != jpeg_size) {
        ESP_LOGE("JPEG_TASK", "read jpeg failed");
        goto cleanup;
    }
    fclose(f);
    f = NULL;

    /* ---------- JPEG decode ---------- */
    jpeg_dec_config_t dec_cfg = DEFAULT_JPEG_DEC_CONFIG();
    dec_cfg.output_type = JPEG_PIXEL_FORMAT_RGB565_LE;

    if (jpeg_dec_open(&dec_cfg, &dec) != JPEG_ERR_OK) {
        ESP_LOGE("JPEG_TASK", "jpeg_dec_open failed");
        goto cleanup;
    }

    jpeg_dec_io_t io = {
        .inbuf = jpeg_in,
        .inbuf_len = jpeg_size,
    };

    if (jpeg_dec_parse_header(dec, &io, &info) != JPEG_ERR_OK) {
        ESP_LOGE("JPEG_TASK", "jpeg_dec_parse_header failed");
        goto cleanup;
    }

    const int w = info.width;
    const int h = info.height;
    if (w <= 0 || h <= 0) {
        ESP_LOGE("JPEG_TASK", "invalid image size %dx%d", w, h);
        goto cleanup;
    }

    int dec_out_len = 0;
    if (jpeg_dec_get_outbuf_len(dec, &dec_out_len) != JPEG_ERR_OK || dec_out_len <= 0) {
        ESP_LOGE("JPEG_TASK", "jpeg_dec_get_outbuf_len failed");
        goto cleanup;
    }

    rgb565 = heap_caps_aligned_alloc(16, (size_t)dec_out_len, MALLOC_CAP_SPIRAM);
    if (!rgb565) {
        ESP_LOGE("JPEG_TASK", "malloc rgb565 failed");
        goto cleanup;
    }
    io.outbuf = (uint8_t *)rgb565;

    if (jpeg_dec_process(dec, &io) != JPEG_ERR_OK) {
        ESP_LOGE("JPEG_TASK", "jpeg_dec_process failed");
        goto cleanup;
    }

    /* ---------- apply filter ---------- */
    poker_apply_filter(rgb565, w, h, filter_id);

    /* ---------- RGB565 -> RGB888 ---------- */
    rgb888 = heap_caps_aligned_alloc(16, (size_t)w * (size_t)h * 3, MALLOC_CAP_SPIRAM);
    if (!rgb888) {
        ESP_LOGE("JPEG_TASK", "malloc rgb888 failed");
        goto cleanup;
    }
    for (int i = 0; i < w * h; i++) {
        uint16_t c = rgb565[i];
        rgb888[i * 3 + 0] = ((c >> 11) & 0x1F) << 3;
        rgb888[i * 3 + 1] = ((c >> 5) & 0x3F) << 2;
        rgb888[i * 3 + 2] = (c & 0x1F) << 3;
    }

    /* ---------- JPEG encode ---------- */
    jpeg_enc_config_t enc_cfg = {
        .src_type = JPEG_PIXEL_FORMAT_RGB888,
        .subsampling = JPEG_SUBSAMPLE_420,
        .quality = 80,
        .width = w,
        .height = h,
        
    };

    if (jpeg_enc_open(&enc_cfg, &enc) != JPEG_ERR_OK) {
        ESP_LOGE("JPEG_TASK", "jpeg_enc_open failed");
        goto cleanup;
    }

    size_t out_buf_size = (size_t)w * (size_t)h * 3;
    jpeg_out = heap_caps_malloc(out_buf_size, MALLOC_CAP_SPIRAM);
    if (!jpeg_out) {
        ESP_LOGE("JPEG_TASK", "malloc jpeg_out failed");
        goto cleanup;
    }

    if (jpeg_enc_process(
            enc,
            rgb888,
            out_buf_size,
            jpeg_out,
            out_buf_size,
            &jpeg_len) != JPEG_ERR_OK) {
        ESP_LOGE("JPEG_TASK", "jpeg_enc_process failed");
        goto cleanup;
    }
    if (jpeg_len <= 0) {
        ESP_LOGE("JPEG_TASK", "jpeg_len invalid");
        goto cleanup;
    }

    /* ---------- write to SD ---------- */
    fo = fopen(out_path, "wb");
    if (!fo) {
        ESP_LOGE("JPEG_TASK", "open output failed");
        goto cleanup;
    }
    if (fwrite(jpeg_out, 1, (size_t)jpeg_len, fo) != (size_t)jpeg_len) {
        ESP_LOGE("JPEG_TASK", "write jpeg failed");
        goto cleanup;
    }
    fclose(fo);
    fo = NULL;

    ESP_LOGI("JPEG_TASK", "Saved %s", out_path);
   

    
   

    
    ret = ESP_OK;

cleanup:
    if (f) {
        fclose(f);
    }
    if (fo) {
        fclose(fo);
    }
    if (dec) {
        jpeg_dec_close(dec);
    }
    if (enc) {
        jpeg_enc_close(enc);
    }
    if (jpeg_in) {
        heap_caps_free(jpeg_in);
    }
    if (rgb565) {
        heap_caps_free(rgb565);
    }
    if (rgb888) {
        heap_caps_free(rgb888);
    }
    if (jpeg_out) {
        heap_caps_free(jpeg_out);
    }
    return ret;
}

   BaseType_t ret;


static void jpeg_filter_task(void *arg)
{


    ESP_LOGI("JPEG_TASK", "========================================");
    ESP_LOGI("JPEG_TASK", "jpeg_filter_task 启动成功！");
    ESP_LOGI("JPEG_TASK", "堆栈剩余: %d 字节", uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t));
    ESP_LOGI("JPEG_TASK", "========================================");


    jpeg_job_t job;

    while (1) {
        ESP_LOGI("TAG","任务运行中");
         ESP_LOGI("JPEG_TASK", "队列句柄: %p", jpeg_job_queue);

        ret = xQueueReceive(jpeg_job_queue, &job, 1000 / portTICK_PERIOD_MS); // 1秒超时

        // 1️⃣ 阻塞等待新的文件名
        if (ret == pdTRUE) {

            

            ESP_LOGI("JPEG_TASK", "Got job: %s", job.in_path);

            lvgl_port_lock(portMAX_DELAY);
            lv_led_set_color( objects.status_led, lv_color_hex(0xffc058)); // 黄色
            lvgl_port_unlock();


            // 清除完成标志
            xEventGroupClearBits(jpeg_event, JPEG_EVENT_DONE);

            // 2️⃣ 处理一次
            process_jpeg_with_filter(
                job.in_path,
                job.out_path,
                job.filter_id
            );

            // 3️⃣ 发送“处理完成”信号
            xEventGroupSetBits(jpeg_event, JPEG_EVENT_DONE);

            lvgl_port_lock(portMAX_DELAY);
            lv_led_set_color( objects.status_led, lv_color_hex(0x6f9a35)); // 绿色
            lvgl_port_unlock();


        }else if (ret == errQUEUE_EMPTY) {
            ESP_LOGI("JPEG_TASK", "队列为空，等待超时");}
            else{
            ESP_LOGI("JPEG_TASK", "没有接受到图片处理任务");
        }
    }
}


void start_jpeg_filter_task(void)
{

     if (!jpeg_job_queue) {
        jpeg_job_queue = xQueueCreate(2, sizeof(jpeg_job_t));
    }

    if (!jpeg_event) {
        jpeg_event = xEventGroupCreate();
    }



    // xTaskCreatePinnedToCore(
    //     jpeg_filter_task,
    //     "jpeg_filter_task",
    //     8192 * 8,
    //     NULL,
    //     4,
    //     NULL,
    //     0
    // );


            xTaskCreatePinnedToCoreWithCaps(
    jpeg_filter_task,          // 任务函数
    "jpeg_filter_task",               // 任务名
    8192*8,                // 栈大小（单位：word，不是字节！）
    NULL,             // 参数
    4,                        // 优先级
    NULL,                     // TaskHandle_t*
    0,                        // 绑定 CPU1
    MALLOC_CAP_SPIRAM         // 👈 强制栈从 PSRAM 分配
        );


}


#include "vars.h"

void send_jpeg_process_job(
    const char *in,
    const char *out,
    poker_filter_t filter)
{
    ESP_LOGI("信号", "开始发送信号,滤镜id是%d", filter);
    
    jpeg_job_t job = {0};
    
    strncpy(job.in_path, in, JPEG_PATH_MAX - 1);
    job.in_path[JPEG_PATH_MAX - 1] = '\0';
    
    strncpy(job.out_path, out, JPEG_PATH_MAX - 1);
    job.out_path[JPEG_PATH_MAX - 1] = '\0';
    
    job.filter_id = filter;
    
    ESP_LOGI("信号", "队列句柄地址: %p", jpeg_job_queue);
    
    if (jpeg_job_queue == NULL) {
        ESP_LOGE("myenc", "Error: Queue not initialized!");
        return;
    }
    
    // 检查队列是否已满
    if (uxQueueSpacesAvailable(jpeg_job_queue) == 0) {
        ESP_LOGW("信号", "队列已满，等待...");
    }
    
    BaseType_t ret = xQueueSend(jpeg_job_queue, &job, portMAX_DELAY);
    
    if (ret == pdTRUE) {
        ESP_LOGI("信号", "成功发送任务到队列");
    } else {
        ESP_LOGE("信号", "发送任务失败: %d", ret);
    }
}
