#include "lcd.h"


#include "pin.h"
#include "esp_err.h"
#include "esp_log.h"
#include "esp_check.h"
#include "driver/gpio.h"

#include "driver/spi_master.h"
#include "esp_lcd_panel_io.h"
#include "esp_lcd_panel_vendor.h"
#include "esp_lcd_panel_ops.h"
#include "esp_lvgl_port.h"
#include "esp_lvgl_port_disp.h"


#include "ui/ui.h"  // 引入 UI 头文件
#include "lvgl.h"




/* LCD size */
#define EXAMPLE_LCD_H_RES   (172)
#define EXAMPLE_LCD_V_RES   (320)

/* LCD settings */
#define EXAMPLE_LCD_SPI_NUM         (SPI3_HOST)
#define EXAMPLE_LCD_PIXEL_CLK_HZ    (20 * 1000 * 1000)
#define EXAMPLE_LCD_CMD_BITS        (8)
#define EXAMPLE_LCD_PARAM_BITS      (8)
#define EXAMPLE_LCD_BITS_PER_PIXEL  (16)
#define EXAMPLE_LCD_DRAW_BUFF_DOUBLE (1)
#define EXAMPLE_LCD_DRAW_BUFF_HEIGHT (50)
#define EXAMPLE_LCD_BL_ON_LEVEL     (0)



volatile uint32_t g_last_key = 0;
volatile bool g_key_pressed = false;




static const char *TAG = "LCD";

static esp_lcd_panel_io_handle_t lcd_io = NULL;
static esp_lcd_panel_handle_t lcd_panel = NULL;
static lv_display_t *lvgl_disp = NULL;

/* 初始化 LCD */
esp_err_t app_lcd_init(void)
{
    esp_err_t ret = ESP_OK;

    gpio_config_t bk_gpio_config = {
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << EXAMPLE_LCD_GPIO_BL
    };
    ESP_ERROR_CHECK(gpio_config(&bk_gpio_config));

    const spi_bus_config_t buscfg = {
        .sclk_io_num = EXAMPLE_LCD_GPIO_SCLK,
        .mosi_io_num = EXAMPLE_LCD_GPIO_MOSI,
        .miso_io_num = GPIO_NUM_NC,
        .quadwp_io_num = GPIO_NUM_NC,
        .quadhd_io_num = GPIO_NUM_NC,
        .max_transfer_sz = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_DRAW_BUFF_HEIGHT * sizeof(uint16_t),
    };
    ESP_RETURN_ON_ERROR(spi_bus_initialize(EXAMPLE_LCD_SPI_NUM, &buscfg, SPI_DMA_CH_AUTO), TAG, "SPI init failed");

    const esp_lcd_panel_io_spi_config_t io_config = {
        .dc_gpio_num = EXAMPLE_LCD_GPIO_DC,
        .cs_gpio_num = EXAMPLE_LCD_GPIO_CS,
        .pclk_hz = EXAMPLE_LCD_PIXEL_CLK_HZ,
        .lcd_cmd_bits = EXAMPLE_LCD_CMD_BITS,
        .lcd_param_bits = EXAMPLE_LCD_PARAM_BITS,
        .spi_mode = 0,
        .trans_queue_depth = 10,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_io_spi((esp_lcd_spi_bus_handle_t)EXAMPLE_LCD_SPI_NUM, &io_config, &lcd_io),
                      err, TAG, "New panel IO failed");

    // ✅ 正确顺序：先创建 panel，再初始化
    const esp_lcd_panel_dev_config_t panel_config = {
        .reset_gpio_num = EXAMPLE_LCD_GPIO_RST,
        .rgb_endian = LCD_RGB_ENDIAN_RGB,
        .bits_per_pixel = EXAMPLE_LCD_BITS_PER_PIXEL,
    };
    ESP_GOTO_ON_ERROR(esp_lcd_new_panel_st7789(lcd_io, &panel_config, &lcd_panel),
                      err, TAG, "New panel failed");

    // ✅ 然后再 reset + init
    esp_lcd_panel_reset(lcd_panel);
    esp_lcd_panel_init(lcd_panel);

    // ✅ 显示方向修正
    esp_lcd_panel_mirror(lcd_panel, true, true);
    esp_lcd_panel_set_gap(lcd_panel, 0, 34);

    // // ✅ 关闭反色
     esp_lcd_panel_invert_color(lcd_panel, true);

    // ✅ 打开背光
    gpio_set_level(EXAMPLE_LCD_GPIO_BL, EXAMPLE_LCD_BL_ON_LEVEL);
    esp_lcd_panel_disp_on_off(lcd_panel, true);

    return ESP_OK;

err:
    if (lcd_panel) esp_lcd_panel_del(lcd_panel);
    if (lcd_io) esp_lcd_panel_io_del(lcd_io);
    spi_bus_free(EXAMPLE_LCD_SPI_NUM);
    return ret;
}







/* 初始化 LVGL */
 esp_err_t app_lvgl_init(void)
{
    const lvgl_port_cfg_t lvgl_cfg = {
        .task_priority = 4,
        .task_stack = 8192,
        .task_affinity = -1,
        .task_max_sleep_ms = 500,
        .timer_period_ms = 5
    };
    ESP_RETURN_ON_ERROR(lvgl_port_init(&lvgl_cfg), TAG, "LVGL init failed");

    const lvgl_port_display_cfg_t disp_cfg = {
        .io_handle = lcd_io,
        .panel_handle = lcd_panel,
        .buffer_size = EXAMPLE_LCD_H_RES * EXAMPLE_LCD_DRAW_BUFF_HEIGHT,
        .double_buffer = EXAMPLE_LCD_DRAW_BUFF_DOUBLE,
        .hres = EXAMPLE_LCD_H_RES,
        .vres = EXAMPLE_LCD_V_RES,
        .rotation = { .swap_xy = false, .mirror_x = false, .mirror_y = false },
        .flags = { .buff_dma = true, .swap_bytes = true }
    };
    lvgl_disp = lvgl_port_add_disp(&disp_cfg);


    ESP_LOGI("TAG","befor rotation is %d",    lvgl_disp->rotation);

    lv_disp_set_rotation(lvgl_disp, LV_DISPLAY_ROTATION_270);

    ESP_LOGI("TAG","after rotation is %d",    lvgl_disp->rotation);


   return ESP_OK;

}




void read_cb(lv_indev_t* indev, lv_indev_data_t* data)
{
    if (g_key_pressed) {
        data->state = LV_INDEV_STATE_PRESSED;
        data->key   = g_last_key;
        g_key_pressed = false;   // ⭐ 一次性消费
    } else {
        data->state = LV_INDEV_STATE_RELEASED;
    }
}



lv_group_t* g_focus_group_main ;
lv_group_t* g_focus_group_shot ;
lv_group_t* g_focus_group_browser_page ;

#include "screens.h"



void init_all_focus_group(){
       g_focus_group_main = lv_group_create();
          g_focus_group_shot = lv_group_create();
             g_focus_group_browser_page = lv_group_create();

             lv_group_set_wrap(g_focus_group_main, true);   // ⭐ 关键
    lv_group_add_obj(g_focus_group_main,objects.bt_browser_pics);
    lv_group_add_obj(g_focus_group_main,objects.bt_shot);


    lv_group_set_wrap(g_focus_group_browser_page, true);   // ⭐ 关键

    // 不要选中列表为可聚焦的，这样当切换到最后一个的时候就会出现选中
    // lv_group_add_obj(g_focus_group_browser_page,objects.file_list_obj);
    lv_group_add_obj(g_focus_group_browser_page,objects.bt_back_from_browser);

lv_group_set_wrap(g_focus_group_shot, true);   // ⭐ 关键
    lv_group_add_obj(g_focus_group_shot,objects.bt_back_from_shot);

}



 lv_indev_t* indev  ; 

void binding_key() {
    init_all_focus_group() ; 
    // 必须在 lvgl_port_init 之后调用
     indev = lv_indev_create();
    lv_indev_set_type(indev, LV_INDEV_TYPE_KEYPAD);
    lv_indev_set_read_cb(indev, read_cb);
    
    // 创建一个焦点组并绑定到输入设备，否则按键不知道发给谁

    lv_group_set_default(g_focus_group_main);
    lv_indev_set_group(indev, g_focus_group_main);
}



