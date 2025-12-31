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


#define NS_LRCLK   45
#define NS_BCLK    48
#define NS_SDATA   47
#define NS_CTRL    46



// 录音引脚
#define MIC_BCLK_IO   19
#define MIC_WS_IO     20
#define MIC_DATA_IO   21
#define SAMPLE_RATE   16000
#define READ_LEN      2048   // 建议 ≥2048，SD 写更稳

#define MOUNT_POINT   "/sdcard"

 sdmmc_card_t *sdcard;
 FILE *wav_file = NULL;
 uint32_t pcm_bytes = 0;
static bool recording_active = false;
static i2s_chan_handle_t tx_chan = NULL;
static bool tx_inited = false;
static lv_obj_t *g_wav_list = NULL; // 录音列表对象（外部传入）



#define TAG "REC_24BIT"


// 使用24bit进行录音








 i2s_chan_handle_t rx_chan = NULL;


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



void i2s_rx_init(void)
{
    i2s_chan_config_t chan_cfg =
        I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);

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
            .bit_shift = true,   // 大多数 MEMS 需要 true
        },
        .gpio_cfg = {
            .bclk = MIC_BCLK_IO,
            .ws   = MIC_WS_IO,
            .din  = MIC_DATA_IO,
            .dout = I2S_GPIO_UNUSED,
            .mclk = I2S_GPIO_UNUSED,
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(rx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(rx_chan));
}

static void i2s_tx_init(void)
{
    if (tx_inited) return;

    i2s_chan_config_t chan_cfg =
        I2S_CHANNEL_DEFAULT_CONFIG(I2S_NUM_AUTO, I2S_ROLE_MASTER);

    ESP_ERROR_CHECK(i2s_new_channel(&chan_cfg, &tx_chan, NULL));

    i2s_std_config_t std_cfg = {
        .clk_cfg = I2S_STD_CLK_DEFAULT_CONFIG(SAMPLE_RATE),
        .slot_cfg = {
            .data_bit_width = I2S_DATA_BIT_WIDTH_32BIT,
            .slot_bit_width = I2S_SLOT_BIT_WIDTH_32BIT,
            .slot_mode = I2S_SLOT_MODE_MONO,
            .slot_mask = I2S_STD_SLOT_LEFT,
            .ws_width = 32,
            .ws_pol = false,
            .bit_shift = true,
        },
        .gpio_cfg = {
            .bclk = NS_BCLK,
            .ws   = NS_LRCLK,
            .dout = NS_SDATA,
            .din  = I2S_GPIO_UNUSED,
            .mclk = I2S_GPIO_UNUSED,
        },
    };

    ESP_ERROR_CHECK(i2s_channel_init_std_mode(tx_chan, &std_cfg));
    ESP_ERROR_CHECK(i2s_channel_enable(tx_chan));
    tx_inited = true;
}

static int ends_with_wav(const char *name)
{
    int len = strlen(name);
    if (len < 4) return 0;
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
    if (dir) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            if (entry->d_type != DT_REG) continue;
            const char *name = entry->d_name;
            int len = strlen(name);
            if (len != 8) continue; // "0000.wav"
            if (!ends_with_wav(name)) continue;
            int num = (name[0]-'0')*1000 + (name[1]-'0')*100 + (name[2]-'0')*10 + (name[3]-'0');
            if (num >=0 && num <= 9999) {
                used[num] = 1;
            }
        }
        closedir(dir);
    }

    for (int i = 0; i < 10000; i++) {
        if (!used[i]) {
            snprintf(out, out_len, MOUNT_POINT"/%04d.wav", i);
            return 0;
        }
    }
    return -1; // 满了
}



// 配置ns4168的使能初始化
void gpio_init (uint8_t pin)
{
    gpio_config_t io_conf = {
        .intr_type = GPIO_INTR_DISABLE,       // ????
        .mode = GPIO_MODE_OUTPUT,             // ???????
        .pin_bit_mask = (1ULL << pin),        // ????GPIO?????
        .pull_down_en = GPIO_PULLDOWN_DISABLE,// ??????
        .pull_up_en = GPIO_PULLUP_DISABLE     // ??????
    };

    gpio_config(&io_conf); // ????

    // ???????????
    gpio_set_level(pin, 1);
}

// ???ns4168?????
void init_ns4168_gpio_pin(){
    gpio_init( NS_LRCLK) ; 
    gpio_init( NS_BCLK ) ; 
    gpio_init( NS_SDATA) ; 
    gpio_init( NS_CTRL ) ; 

}

void record_task(void *arg)
{
    int32_t buffer[READ_LEN / 4];   // I2S 32bit FIFO
    size_t bytes_read;

    while (1) {

        /* ▶ 开始录音 */
        if (is_record && !recording_active) {
            ESP_LOGI(TAG, "Start recording");
            char filename[64];
            if (next_wav_filename(filename, sizeof(filename)) != 0) {
                ESP_LOGE(TAG, "No available wav filename");
                is_record = false;
                continue;
            }
            wav_file = fopen(filename, "wb");
            if (!wav_file) {
                ESP_LOGE(TAG, "Failed to open file");
                is_record = false;
                continue;
            }
            uint8_t header[44] = {0};
            fwrite(header, 1, 44, wav_file);
            pcm_bytes = 0;
            recording_active = true;
        }

        /* 🎙 录音中 */
        if (recording_active && wav_file) {
            i2s_channel_read(rx_chan,
                             buffer,
                             sizeof(buffer),
                             &bytes_read,
                             portMAX_DELAY);

            int samples = bytes_read / 4;

            for (int i = 0; i < samples; i++) {
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
        if (!is_record && recording_active) {
            ESP_LOGI(TAG, "Stop recording");
            if (wav_file) {
                write_wav_header(wav_file, pcm_bytes);
                fclose(wav_file);
                wav_file = NULL;
            }
            recording_active = false;
            ESP_LOGI(TAG, "Saved wav file");
            // 录音完成后刷新列表
            if (g_wav_list) {
                fill_wav_list(g_wav_list);
            }
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }
}




void stop_record(){

    gpio_set_level(NS_CTRL, 0);
    is_record = false;
}


void start_record(){
            gpio_set_level(NS_CTRL, 1);
    is_record = true;
    }

void set_wav_list_obj(lv_obj_t *list)
{
    g_wav_list = list;
    if (g_wav_list) {
        fill_wav_list(g_wav_list); // 立即填充一次，避免初始为空
    }
}


// ================= WAV 列表与播放 =================



static void playback_wav_file(const char *path)
{
    i2s_tx_init();
    FILE *f = fopen(path, "rb");
    if (!f) {
        ESP_LOGE(TAG, "open %s failed", path);
        return;
    }

    // 停止当前录音，避免读写冲突
    is_record = false;

    // 跳过 WAV 头
    fseek(f, 44, SEEK_SET);

    const size_t RAW_CHUNK = 1024;
    uint8_t raw[RAW_CHUNK];
    int32_t samples[RAW_CHUNK / 3 + 2];

    size_t n;
    while ((n = fread(raw, 1, sizeof(raw), f)) > 0) {
        size_t sample_count = n / 3;
        for (size_t i = 0; i < sample_count; i++) {
            uint8_t b0 = raw[i * 3 + 0];
            uint8_t b1 = raw[i * 3 + 1];
            int8_t b2 = (int8_t)raw[i * 3 + 2]; // 符号位

            int32_t s24 = (int32_t)((uint32_t)b0 | ((uint32_t)b1 << 8) | ((int32_t)b2 << 16));
            samples[i] = s24 << 8; // 左移对齐到 32bit
        }

        size_t bytes_to_write = sample_count * sizeof(int32_t);
        size_t bytes_written = 0;
        esp_err_t ret = i2s_channel_write(tx_chan, samples, bytes_to_write, &bytes_written, pdMS_TO_TICKS(200));
        if (ret != ESP_OK) {
            ESP_LOGE(TAG, "i2s write err %d", ret);
            break;
        }
    }

    fclose(f);
    ESP_LOGI(TAG, "Play done: %s", path);
     fill_wav_list(objects.list_wav) ; 
}

static void wav_item_click_cb(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_target(e);
    lv_obj_t * list = lv_obj_get_parent(lv_obj_get_parent(btn));
    const char * file_name = lv_list_get_button_text(list, btn);
    if (!file_name) return;

    char full_path[128];
    snprintf(full_path, sizeof(full_path), "/sdcard/%s", file_name);
    ESP_LOGI(TAG, "Play wav: %s", full_path);
    playback_wav_file(full_path);
}

void fill_wav_list(lv_obj_t *list)
{
    g_wav_list = list; // 记住列表对象，便于录音完成后刷新
    DIR *dir = opendir("/sdcard");
    if (!dir) {
        ESP_LOGE(TAG, "opendir failed");
        return;
    }

    lv_obj_clean(list);

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        const char *name = entry->d_name;
        if (!ends_with_wav(name)) continue;

        lv_obj_t * btn = lv_list_add_button(list, LV_SYMBOL_AUDIO, name);
        lv_group_add_obj(g_focus_group_record_page, btn);
        lv_obj_add_event_cb(btn, wav_item_click_cb, LV_EVENT_CLICKED, NULL);
    }
    closedir(dir);
}
