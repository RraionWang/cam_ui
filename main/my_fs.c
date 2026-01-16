/* SD card and FAT filesystem example.
   This example code is in the Public Domain (or CC0 licensed, at your option.)

   Unless required by applicable law or agreed to in writing, this
   software is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied.
*/

// This example uses SDMMC peripheral to communicate with SD card.

#include <string.h>
#include <sys/unistd.h>
#include <sys/stat.h>
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
// #include "sd_test_io.h"
#if SOC_SDMMC_IO_POWER_EXTERNAL
#include "sd_pwr_ctrl_by_on_chip_ldo.h"
#endif

#define EXAMPLE_MAX_CHAR_SIZE    64

static const char *TAG = "example";

#define MOUNT_POINT "/sdcard"
#define EXAMPLE_IS_UHS1    (CONFIG_EXAMPLE_SDMMC_SPEED_UHS_I_SDR50 || CONFIG_EXAMPLE_SDMMC_SPEED_UHS_I_DDR50)

#include "lvgl.h"
#include "my_fs.h"
#include "lcd.h"





static esp_err_t s_example_write_file(const char *path, char *data)
{
    ESP_LOGI(TAG, "Opening file %s", path);
    FILE *f = fopen(path, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return ESP_FAIL;
    }
    fprintf(f, data);
    fclose(f);
    ESP_LOGI(TAG, "File written");

    return ESP_OK;
}

static esp_err_t s_example_read_file(const char *path)
{
    ESP_LOGI(TAG, "Reading file %s", path);
    FILE *f = fopen(path, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return ESP_FAIL;
    }
    char line[EXAMPLE_MAX_CHAR_SIZE];
    fgets(line, sizeof(line), f);
    fclose(f);

    // strip newline
    char *pos = strchr(line, '\n');
    if (pos) {
        *pos = '\0';
    }
    ESP_LOGI(TAG, "Read from file: '%s'", line);

    return ESP_OK;
}

sdmmc_card_t*  init_sdcard(void)
{
    esp_err_t ret;

    // Options for mounting the filesystem.
    // If format_if_mount_failed is set to true, SD card will be partitioned and
    // formatted in case when mounting fails.
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif // EXAMPLE_FORMAT_IF_MOUNT_FAILED
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    sdmmc_card_t *card;
    const char mount_point[] = MOUNT_POINT;
    ESP_LOGI(TAG, "Initializing SD card");

    // Use settings defined above to initialize SD card and mount FAT filesystem.
    // Note: esp_vfs_fat_sdmmc/sdspi_mount is all-in-one convenience functions.
    // Please check its source code and implement error recovery when developing
    // production applications.

    ESP_LOGI(TAG, "Using SDMMC peripheral");

    // By default, SD card frequency is initialized to SDMMC_FREQ_DEFAULT (20MHz)
    // For setting a specific frequency, use host.max_freq_khz (range 400kHz - 40MHz for SDMMC)
    // Example: for fixed frequency of 10MHz, use host.max_freq_khz = 10000;
    sdmmc_host_t host = SDMMC_HOST_DEFAULT();

    
#if CONFIG_EXAMPLE_SDMMC_SPEED_HS
    host.max_freq_khz = SDMMC_FREQ_HIGHSPEED;
#elif CONFIG_EXAMPLE_SDMMC_SPEED_UHS_I_SDR50
    host.slot = SDMMC_HOST_SLOT_0;
    host.max_freq_khz = SDMMC_FREQ_SDR50;
    host.flags &= ~SDMMC_HOST_FLAG_DDR;
#elif CONFIG_EXAMPLE_SDMMC_SPEED_UHS_I_DDR50
    host.slot = SDMMC_HOST_SLOT_0;
    host.max_freq_khz = SDMMC_FREQ_DDR50;
#endif

host.max_freq_khz =SDMMC_FREQ_HIGHSPEED ; 

    // For SoCs where the SD power can be supplied both via an internal or external (e.g. on-board LDO) power supply.
    // When using specific IO pins (which can be used for ultra high-speed SDMMC) to connect to the SD card
    // and the internal LDO power supply, we need to initialize the power supply first.
#if CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_INTERNAL_IO
    sd_pwr_ctrl_ldo_config_t ldo_config = {
        .ldo_chan_id = CONFIG_EXAMPLE_SD_PWR_CTRL_LDO_IO_ID,
    };
    sd_pwr_ctrl_handle_t pwr_ctrl_handle = NULL;

    ret = sd_pwr_ctrl_new_on_chip_ldo(&ldo_config, &pwr_ctrl_handle);
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "Failed to create a new on-chip LDO power control driver");
        return;
    }
    host.pwr_ctrl_handle = pwr_ctrl_handle;
#endif

    // This initializes the slot without card detect (CD) and write protect (WP) signals.
    // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
    sdmmc_slot_config_t slot_config = SDMMC_SLOT_CONFIG_DEFAULT();

#if EXAMPLE_IS_UHS1
    slot_config.flags |= SDMMC_SLOT_FLAG_UHS1;
#endif

    // Set bus width to use:
// #ifdef CONFIG_EXAMPLE_SDMMC_BUS_WIDTH_4
//     slot_config.width = 4;
// #else
    slot_config.width = 1;
// #endif



    slot_config.clk = 39;
    slot_config.cmd = 38;
    slot_config.d0 = 40;
    // slot_config.d1 = 41;
    // slot_config.d2 = 14;
    // slot_config.d3 = 47;



    // Enable internal pullups on enabled pins. The internal pullups
    // are insufficient however, please make sure 10k external pullups are
    // connected on the bus. This is for debug / example purpose only.
    slot_config.flags |= SDMMC_SLOT_FLAG_INTERNAL_PULLUP;

    ESP_LOGI(TAG, "Mounting filesystem");
    ret = esp_vfs_fat_sdmmc_mount(mount_point, &host, &slot_config, &mount_config, &card);

    if (ret != ESP_OK) {
        if (ret == ESP_FAIL) {
            ESP_LOGE(TAG, "Failed to mount filesystem. "
                     "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        } else {
            ESP_LOGE(TAG, "Failed to initialize the card (%s). "
                     "Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
#ifdef CONFIG_EXAMPLE_DEBUG_PIN_CONNECTIONS
            check_sd_card_pins(&config, pin_count);
#endif
        }
        return NULL;
    }
    ESP_LOGI(TAG, "Filesystem mounted");

    // Card has been initialized, print its properties
    sdmmc_card_print_info(stdout, card);

    // Use POSIX and C standard library functions to work with files:

    // // First create a file.
    // const char *file_hello = MOUNT_POINT"/hello.txt";
    // char data[EXAMPLE_MAX_CHAR_SIZE];
    // snprintf(data, EXAMPLE_MAX_CHAR_SIZE, "%s %s!\n", "Hello", card->cid.name);
    // ret = s_example_write_file(file_hello, data);
    // if (ret != ESP_OK) {
    //     return NULL;
    // }

    // const char *file_foo = MOUNT_POINT"/foo.txt";
    // // Check if destination file exists before renaming
    // struct stat st;
    // if (stat(file_foo, &st) == 0) {
    //     // Delete it if it exists
    //     unlink(file_foo);
    // }

    // Rename original file
    // ESP_LOGI(TAG, "Renaming file %s to %s", file_hello, file_foo);
    // if (rename(file_hello, file_foo) != 0) {
    //     ESP_LOGE(TAG, "Rename failed");
    //     return NULL;
    // }

    // ret = s_example_read_file(file_foo);
    // if (ret != ESP_OK) {
    //     return NULL;
    // }

    // Format FATFS

    // ret = esp_vfs_fat_sdcard_format(mount_point, card);
    // if (ret != ESP_OK) {
    //     ESP_LOGE(TAG, "Failed to format FATFS (%s)", esp_err_to_name(ret));
    //     return NULL;
    // }

    // if (stat(file_foo, &st) == 0) {
    //     ESP_LOGI(TAG, "file still exists");
    //     return NULL;
    // } else {
    //     ESP_LOGI(TAG, "file doesn't exist, formatting done");
    // }


    // const char *file_nihao = MOUNT_POINT"/nihao.txt";
    // memset(data, 0, EXAMPLE_MAX_CHAR_SIZE);
    // snprintf(data, EXAMPLE_MAX_CHAR_SIZE, "%s %s!\n", "Nihao", card->cid.name);
    // ret = s_example_write_file(file_nihao, data);
    // if (ret != ESP_OK) {
    //     return NULL;
    // }

    // //Open file for reading
    // ret = s_example_read_file(file_nihao);
    // if (ret != ESP_OK) {
    //     return NULL;
    // }

    return card ; 
    // // All done, unmount partition and disable SDMMC peripheral

    // esp_vfs_fat_sdcard_unmount(mount_point, card);
    // ESP_LOGI(TAG, "Card unmounted");


}


// 解码函数 从sd卡读取到内存

static esp_err_t read_file_to_mem(const char *path, uint8_t **buf, int *len)
{
    FILE *f = fopen(path, "rb");
    if (!f) {
        ESP_LOGE("JPG", "open failed: %s", path);
        return ESP_FAIL;
    }

    fseek(f, 0, SEEK_END);
    int size = ftell(f);
    rewind(f);

    uint8_t *data = heap_caps_malloc(size, MALLOC_CAP_8BIT);
    if (!data) {
        fclose(f);
        return ESP_ERR_NO_MEM;
    }

    fread(data, 1, size, f);
    fclose(f);

    *buf = data;
    *len = size;
    return ESP_OK;
}




#include "esp_jpeg_common.h"
#include "esp_jpeg_dec.h"



// 解码函数

jpeg_error_t esp_jpeg_decode_to_rgb565(
    uint8_t *input_buf, int len,
    int out_w, int out_h,
    uint8_t **output_buf, int *out_len)
{
    jpeg_error_t ret;
    jpeg_dec_handle_t jpeg_dec = NULL;
    jpeg_dec_io_t jpeg_io = {0};

    jpeg_dec_config_t config = DEFAULT_JPEG_DEC_CONFIG();
    config.output_type = JPEG_PIXEL_FORMAT_RGB565_LE;
    config.scale.width  = out_w;
    config.scale.height = out_h;

    ret = jpeg_dec_open(&config, &jpeg_dec);
    if (ret != JPEG_ERR_OK) return ret;

    jpeg_io.inbuf = input_buf;
    jpeg_io.inbuf_len = len;

    jpeg_dec_header_info_t header;
    ret = jpeg_dec_parse_header(jpeg_dec, &jpeg_io, &header);
    if (ret != JPEG_ERR_OK) goto end;

    *out_len = out_w * out_h * 2;
    uint8_t *out_buf = jpeg_calloc_align(*out_len, 16);
    if (!out_buf) {
        ret = JPEG_ERR_NO_MEM;
        goto end;
    }

    jpeg_io.outbuf = out_buf;
    *output_buf = out_buf;

    ret = jpeg_dec_process(jpeg_dec, &jpeg_io);

end:
    jpeg_dec_close(jpeg_dec);
    return ret;
}

// 使用静态或全局变量，确保图片显示期间数据描述符有效
static lv_image_dsc_t img_dsc;

static void delete_old_img_cb(lv_event_t * e) {
    void * data = lv_event_get_user_data(e);
    if(data) free(data); // 释放解码出来的 RGB565 缓冲区
}

static void show_jpg_in_obj(lv_obj_t *parent, uint8_t *rgb565, int w, int h)
{
    // 1. 清除父容器里旧的预览图，触发 DELETE 事件释放内存
    lv_obj_clean(parent); 

    static lv_image_dsc_t img_dsc; // 注意：如果是单实例预览，dsc 可以静态
    img_dsc.header.cf = LV_COLOR_FORMAT_RGB565;
    img_dsc.header.w  = h;
    img_dsc.header.h  = w;
    img_dsc.header.stride = h * 2;
    img_dsc.data_size = w * h * 2;
    img_dsc.data = rgb565;

    lv_obj_t *img = lv_image_create(parent);
    lv_image_set_src(img, &img_dsc);
    lv_obj_center(img);

    // 2. 当这个图片对象被删除时，自动释放这一坨 rgb565 内存
    lv_obj_add_event_cb(img, delete_old_img_cb, LV_EVENT_DELETE, rgb565);
}


#include <dirent.h>
#include <string.h>

#include "screens.h"





// 1. 统一显示函数名和参数顺序
void load_sd_jpg_to_obj(const char* path, lv_obj_t *obj)
{
    uint8_t *jpg_buf = NULL;
    int jpg_len = 0;

    if (read_file_to_mem(path, &jpg_buf, &jpg_len) != ESP_OK)
        return;

    uint8_t *rgb565 = NULL;
    int rgb_len = 0;

    // 解码为 160x120 (请确保你的预览框 obj 也是这个尺寸)
    jpeg_error_t ret = esp_jpeg_decode_to_rgb565(
        jpg_buf, jpg_len,
        96, 160,
        &rgb565, &rgb_len
    );

    free(jpg_buf);

    if (ret != JPEG_ERR_OK) {
        ESP_LOGE("JPG", "decode failed");
        return;
    }

    // 显示图片
    show_jpg_in_obj(obj, rgb565, 160, 96);
}




// 回调函数
static void my_jpg_preview_cb(lv_event_t * e)
{
    lv_obj_t * btn = lv_event_get_target(e);    // 当前获得焦点的按钮
    lv_obj_t * list = lv_obj_get_parent(lv_obj_get_parent(btn)); // 按钮的爷爷是 List 对象

    // 1. 获取按钮上的文件名
    const char * file_name = lv_list_get_button_text(list, btn);

    if (file_name) {
        // 2. 拼接完整路径
        char full_path[128];
        snprintf(full_path, sizeof(full_path), "/sdcard/%s", file_name);

        ESP_LOGI("JPG", "Focus changed to: %s", full_path);

      


        load_sd_jpg_to_obj(full_path,objects.pic_window_obj) ;
    }
}


void fill_jpg_list(lv_obj_t *list)
{
    DIR *dir = opendir("/sdcard");
    if (!dir) {
        ESP_LOGE("SD", "opendir failed");
        return;
    }

    // 清空旧列表项
    lv_obj_clean(list);

    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_type != DT_REG) continue;

        const char *name = entry->d_name;
        int len = strlen(name);
        if (len < 4) continue;

        if (strcasecmp(name + len - 4, ".jpg") == 0) {
            // 1. 创建列表按钮
            lv_obj_t * btn = lv_list_add_button(list, LV_SYMBOL_IMAGE, name);

            // 确保键盘焦点落在浏览页面的组，而不是默认的 main 组
            lv_group_add_obj(g_focus_group_browser_page, btn);

            // 2. 绑定聚焦事件
            // 我们不传 flowState，而是直接传 NULL 或者特定的上下文
            lv_obj_add_event_cb(btn, my_jpg_preview_cb, LV_EVENT_FOCUSED, NULL);
         
        }
    }
    closedir(dir);
}
