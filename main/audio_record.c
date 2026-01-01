#include "audio_record.h"
#include "driver/gpio.h"
#include "dirent.h"
#include "string.h"
#include "ctype.h"

#include "esp_log.h"
#include "esp_err.h"
#include "esp_vfs_fat.h"

#include "driver/i2s_std.h"
#include "driver/sdmmc_host.h"
#include "sdmmc_cmd.h"
#include <sys/unistd.h>
#include "vars.h"
#include "lvgl.h"
#include "lcd.h"
#include "screens.h"
#include "actions.h"

#define NS_LRCLK 45
#define NS_BCLK 48
#define NS_SDATA 47
#define NS_CTRL 46

// 录音引脚
#define MIC_BCLK_IO 19
#define MIC_WS_IO 20
#define MIC_DATA_IO 21

#define SAMPLE_RATE 16000
#define READ_LEN 2048 // 建议 ≥2048，SD 写更稳

#define MOUNT_POINT "/sdcard"

sdmmc_card_t *sdcard;
FILE *wav_file = NULL;
uint32_t pcm_bytes = 0;
static bool recording_active = false;
static i2s_chan_handle_t tx_chan = NULL;
static bool tx_inited = false;
static lv_obj_t *g_wav_list = NULL; // 录音列表对象（外部传入）
static volatile bool g_need_refresh_wav_list = false;

#define CHUNK_SAMPLES 512
#define CHUNK_BYTES   (CHUNK_SAMPLES * 3)

static uint8_t pcm_buffer[CHUNK_BYTES];
static int32_t i2s_buffer[CHUNK_SAMPLES * 2];





#define TAG "REC_24BIT"

// 使用24bit进行录音

i2s_chan_handle_t rx_chan = NULL;



// i2s tx和rx 通道初始化
 void i2s_tx_init(void)
{
    if (tx_inited)
        return;

    i2s_chan_config_t chan_cfg =
        I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_0, I2S_ROLE_MASTER);
    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, NULL));

    i2s_std_slot_config_t slot_cfg = {
        .data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
        .slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT,

        .slot_mode = I2S_SLOT_MODE_STEREO, // ✅ 必须 stereo
        .slot_mask = I2S_STD_SLOT_BOTH,    // ✅ L + R

        .ws_width = 32,
        .ws_pol = false,
        .bit_shift = true,

        .left_align = true,
        .big_endian = false,
        .bit_order_lsb = false,
    };

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = slot_cfg,
        .gpio_cfg = {
            .bclk = NS_BCLK,
            .ws = NS_LRCLK,
            .dout = NS_SDATA,
            .din = I2S_GPIO_UNUSED,
            .mclk = I2S_GPIO_UNUSED,
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));

    tx_inited = true;
}

void i2s_rx_init(void)
{
    i2s_chan_config_t chan_cfg =
        I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_1, I2S_ROLE_MASTER);

    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, NULL, &rx_chan));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT,
            .slot_mode = I2S_SLOT_MODE_MONO,
            .slot_mask = I2S_STD_SLOT_LEFT,
            .ws_width = 32,
            .ws_pol = false,
            .bit_shift = true, // 大多数 MEMS 需要 true
        },
        .gpio_cfg = {
            .bclk = MIC_BCLK_IO,
            .ws = MIC_WS_IO,
            .din = MIC_DATA_IO,
            .dout = I2S_GPIO_UNUSED,
            .mclk = I2S_GPIO_UNUSED,
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));
}


// wav相关函数

void write_wav_header(FILE *f, uint32_t pcm_size)
{
    uint32_t sample_rate = SAMPLE_RATE;
    uint16_t bits = 24;
    uint16_t channels = 1;

    uint32_t byte_rate = sample_rate * channels * bits / 8;
    uint16_t block_align = channels * bits / 8;
    uint32_t chunk_size = pcm_size + 36;

    fseek(f, 0, SEEK_SET);

    fwrite("RIFF", 1, 4, f);
    fwrite(&chunk_size, 4, 1, f);
    fwrite("WAVE", 1, 4, f);

    fwrite("fmt ", 1, 4, f);
    uint32_t subchunk1 = 16;
    uint16_t audio_fmt = 1; // PCM
    fwrite(&subchunk1, 4, 1, f);
    fwrite(&audio_fmt, 2, 1, f);
    fwrite(&channels, 2, 1, f);
    fwrite(&sample_rate, 4, 1, f);
    fwrite(&byte_rate, 4, 1, f);
    fwrite(&block_align, 2, 1, f);
    fwrite(&bits, 2, 1, f);

    fwrite("data", 1, 4, f);
    fwrite(&pcm_size, 4, 1, f);
}

static int ends_with_wav(const char *name)
{
    int len = strlen(name);
    if (len < 4)
        return 0;
    const char *ext = name + len - 4;
    return (tolower((unsigned char)ext[0]) == '.' &&
            tolower((unsigned char)ext[1]) == 'w' &&
            tolower((unsigned char)ext[2]) == 'a' &&
            tolower((unsigned char)ext[3]) == 'v');
}

// 生成下一个 0000.wav ~ 9999.wav 文件名，返回 0 成功
static int next_wav_filename(char *out, size_t out_len)
{
    static uint8_t used[10000];
    memset(used, 0, sizeof(used));
    DIR *dir = opendir(MOUNT_POINT);
    if (dir)
    {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL)
        {
            if (entry->d_type != DT_REG)
                continue;
            const char *name = entry->d_name;
            int len = strlen(name);
            if (len != 8)
                continue; // "0000.wav"
            if (!ends_with_wav(name))
                continue;
            int num = (name[0] - '0') * 1000 + (name[1] - '0') * 100 + (name[2] - '0') * 10 + (name[3] - '0');
            if (num >= 0 && num <= 9999)
            {
                used[num] = 1;
            }
        }
        closedir(dir);
    }

    for (int i = 0; i < 10000; i++)
    {
        if (!used[i])
        {
            snprintf(out, out_len, MOUNT_POINT "/%04d.wav", i);
            return 0;
        }
    }
    return -1; // 满了
}
// 配置ns4168的使能初始化
void ns4168_en_init()
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,        // ????
        .mode = GPIO_MODE_OUTPUT,              // ???????
        .pin_bit_mask = (1ULL << 46),         // ????GPIO?????
        .pull_down_en = GPIO_PULLDOWN_DISABLE, // ??????
        .pull_up_en = GPIO_PULLUP_DISABLE      // ??????
    };

    gpio_config(&io_conf); // ????
    gpio_set_level(46, 0);
}


void record_task(void *arg)
{
    int32_t buffer[READ_LEN / 4]; // I2S 32bit FIFO
    size_t bytes_read;
    const TickType_t read_timeout = pdMS_TO_TICKS(50);

    while (1)
    {

        /* ▶ 开始录音 */
        if (get_var_is_record() && !recording_active)
        {
            ESP_LOGI(TAG, "Start recording");

            /* 若正在播放则先停止，避免录音/播放并行 */
            stop_playback();

            char filename[64];
            if (next_wav_filename(filename, sizeof(filename)) != 0)
            {
                ESP_LOGE(TAG, "No available wav filename");
                continue;
            }
            wav_file = fopen(filename, "wb");
            if (!wav_file)
            {
                ESP_LOGE(TAG, "Failed to open file");
                continue;
            }
            uint8_t header[44] = {0};
            fwrite(header, 1, 44, wav_file);
            pcm_bytes = 0;
            recording_active = true;
        }

        /* 🎙 录音中 */
        if (recording_active && wav_file)
        {
            i2s_channel_read(rx_chan,
                             buffer,
                             sizeof(buffer),
                             &bytes_read,
                             read_timeout);

            if (bytes_read == 0)
            {
                // 读超时，检查标志后继续循环
                continue;
            }

            int samples = bytes_read / 4;

            for (int i = 0; i < samples; i++)
            {
                // 取高 24bit（保持符号）
                int32_t s = buffer[i] >> 8;

                uint8_t out[3];
                out[0] = s & 0xFF;
                out[1] = (s >> 8) & 0xFF;
                out[2] = (s >> 16) & 0xFF;

                fwrite(out, 1, 3, wav_file);
                pcm_bytes += 3;
            }
        }

        /* ⏹ 停止录音 */
        if (!get_var_is_record() && recording_active)
        {
            ESP_LOGI(TAG, "Stop recording");
            if (wav_file)
            {
                write_wav_header(wav_file, pcm_bytes);
                fclose(wav_file);
                wav_file = NULL;
            }
            recording_active = false;
            ESP_LOGI(TAG, "Saved wav file");
            // 录音完成后刷新列表
            g_need_refresh_wav_list = true;
        }

        vTaskDelay(pdMS_TO_TICKS(10));


        audio_record_ui_poll() ; 

    }
}

void disable_speaker()
{
    gpio_set_level(NS_CTRL, 0);
}

void enable_speaker()
{

    gpio_set_level(NS_CTRL, 1);
}

void set_wav_list_obj(lv_obj_t *list)
{
    g_wav_list = list;
    if (g_wav_list)
    {
        fill_wav_list(g_wav_list); // 立即填充一次，避免初始为空
    }
}

// ================= WAV 列表与播放 =================

bool is_playing = false;

// 播放相关变量
static TaskHandle_t play_task_handle = NULL;
static SemaphoreHandle_t play_sem = NULL;
static char play_file_path[256] = {0};
static bool play_requested = false;
static const TickType_t i2s_write_timeout = pdMS_TO_TICKS(500);

static void playback_task(void *arg)
{
    ESP_LOGI(TAG, "Playback task started");

    while (1)
    {
        if (!play_sem)
        {
            vTaskDelay(pdMS_TO_TICKS(10));
            continue;
        }

        if (!play_requested)
        {
            xSemaphoreTake(play_sem, portMAX_DELAY);
        }

        if (!play_requested)
            continue;

        char current_file[256];
        strncpy(current_file, play_file_path, sizeof(current_file));
        play_requested = false;

        ESP_LOGI(TAG, "Playing: %s", current_file);

        // 如果正在录音，直接拒绝播放
        if (get_var_is_record())
        {
            ESP_LOGW(TAG, "Recording active, skip playback");
            is_playing = false;
            continue;
        }

        // 启用扬声器
        enable_speaker();



        // ESP_ERROR_CHECK(i2s_channel_disable(tx_chan));
        // ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));


        FILE *f = fopen(current_file, "rb");
        if (!f)
        {
            ESP_LOGE(TAG, "Open failed: %s", current_file);
            disable_speaker();
            is_playing = false;

            // 更新UI
            if (g_wav_list)
            {
                lv_obj_invalidate(g_wav_list);
            }
            continue;
        }

        is_playing = true;

        // 跳过WAV头
        fseek(f, 44, SEEK_SET);



        size_t bytes_read;
        while (is_playing && (bytes_read = fread(pcm_buffer, 1, CHUNK_BYTES, f)) > 0)
        {
            size_t samples_read = bytes_read / 3;

            // 24位PCM转I2S格式
            for (size_t i = 0; i < samples_read; i++)
            {
                uint8_t b0 = pcm_buffer[i * 3];
                uint8_t b1 = pcm_buffer[i * 3 + 1];
                uint8_t b2 = pcm_buffer[i * 3 + 2];

                int32_t sample = ((int32_t)b0) |
                                 ((int32_t)b1 << 8) |
                                 ((int32_t)b2 << 16);

                if (sample & 0x00800000)
                {
                    sample |= 0xFF000000;
                }
                else
                {
                    sample &= 0x00FFFFFF;
                }

                int32_t out = sample << 8;

                i2s_buffer[i * 2 + 0] = out; // Left
                i2s_buffer[i * 2 + 1] = out; // Right
            }

            // 发送到I2S
            size_t bytes_to_write = samples_read * 2 * sizeof(int32_t);

            size_t bytes_written = 0;

            esp_err_t ret = i2s_channel_write(tx_chan, i2s_buffer, bytes_to_write,
                                              &bytes_written, i2s_write_timeout);


        if (ret != ESP_OK) {
    ESP_LOGE(TAG, "I2S write error: %d (%s)", ret, esp_err_to_name(ret));
    break;
}

            if (bytes_written < bytes_to_write)
            {
                ESP_LOGW(TAG, "I2S write timeout/partial: %u/%u",
                         (unsigned int)bytes_written,
                         (unsigned int)bytes_to_write);
            }

            // 让出CPU，给UI和其他任务机会
            vTaskDelay(pdMS_TO_TICKS(1));

            // 检查是否被停止
            if (!is_playing)
                break;

            // 录音过程中不允许继续播放
            if (get_var_is_record())
                break;
        }

        fclose(f);

        // 等待I2S缓冲区清空
        vTaskDelay(pdMS_TO_TICKS(50));

        // 关闭扬声器
        disable_speaker();

        ESP_LOGI(TAG, "Playback finished");
        is_playing = false;

        // 刷新列表
        if (g_wav_list)
        {
            lv_obj_invalidate(g_wav_list);
        }
    }

    play_task_handle = NULL;
vTaskDelete(NULL);


}


static void playback_wav_file(const char *path)
{
    if (!play_sem)
    {
        ESP_LOGE(TAG, "Playback not initialized");
        return;
    }

    if (play_task_handle == NULL)
    {
        xTaskCreate(playback_task, "playback_task", 4096, NULL, 3, &play_task_handle);
    }

    if (is_playing)
    {
        stop_playback();
    }

    strncpy(play_file_path, path, sizeof(play_file_path)-1);
    play_file_path[sizeof(play_file_path) - 1] = '\0';
    play_requested = true;
    xSemaphoreGive(play_sem);
}


void stop_playback(void)
{
    if (is_playing)
    {
        ESP_LOGI(TAG, "Stopping playback...");
        is_playing = false;

        // 等待播放任务结束
        vTaskDelay(pdMS_TO_TICKS(150));
    }
}

static void wav_item_click_cb(lv_event_t *e)
{
    lv_obj_t *btn = lv_event_get_target(e);


    const char *file_name =  lv_list_get_button_text(objects.list_wav,btn) ; 
    if (!file_name) {
        ESP_LOGE(TAG, "Label text is NULL!");
        return;
    }

    char full_path[128];
    snprintf(full_path, sizeof(full_path), "/sdcard/%s", file_name);
    ESP_LOGI(TAG, "Play wav: %s", full_path);
    playback_wav_file(full_path);
}

void fill_wav_list(lv_obj_t *list)
{
    g_wav_list = list; // 记住列表对象，便于录音完成后刷新
    DIR *dir = opendir("/sdcard");
    if (!dir)
    {
        ESP_LOGE(TAG, "opendir failed");
        return;
    }

    lv_obj_clean(list);

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL)
    {
        const char *name = entry->d_name;
        if (!ends_with_wav(name))
            continue;

        lv_obj_t *btn = lv_list_add_button(list, LV_SYMBOL_AUDIO, name);
        lv_group_add_obj(g_focus_group_record_page, btn);
        lv_obj_add_event_cb(btn, wav_item_click_cb, LV_EVENT_CLICKED, NULL);
    }
    closedir(dir);
}

void audio_play_init(void)
{

    ns4168_en_init() ; 

    // 初始化播放控制
    play_sem = xSemaphoreCreateBinary();
    play_requested = false;
    is_playing = false;

    // 创建播放任务
play_task_handle = NULL;
}



void audio_ui_fill_wav_list(lv_obj_t *list)
{
    DIR *dir = opendir("/sdcard");
    if (!dir) {
        ESP_LOGE("UI", "opendir failed");
        return;
    }

    lv_obj_clean(list);   // ⚠️ 必须在 UI 线程

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (!ends_with_wav(entry->d_name))
            continue;

        lv_obj_t *btn = lv_list_add_button(list, LV_SYMBOL_AUDIO, entry->d_name);
        lv_group_add_obj(g_focus_group_record_page, btn);
        lv_obj_add_event_cb(btn, wav_item_click_cb, LV_EVENT_CLICKED, NULL);
    }

    closedir(dir);
    
}

void audio_record_ui_poll(void)
{
    if (!g_need_refresh_wav_list)
        return;

    g_need_refresh_wav_list = false;

    if (!g_wav_list)
        return;

    // ⚠️ 必须保证在 LVGL 线程 / UI 线程中调用
    fill_wav_list(g_wav_list);
}
