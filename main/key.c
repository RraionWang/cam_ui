# include "key.h"
#include "iot_button.h"
#include "button_adc.h"
#include "button_adc.h"
#include "button_types.h"
#include "iot_button.h"
#include "esp_log.h"
#include "lcd.h"
#include "lvgl.h"
#include "freertos/FreeRTOS.h"
#include "vars.h"
#include "screens.h"
#include "actions.h"



// static void but1_cb(void *arg, void *usr_data)
// {
//     ESP_LOGI("BUT","按键1") ; 
//     g_last_key = LV_KEY_NEXT;
//     g_key_pressed = true;
// }

// static void but2_cb(void *arg, void *usr_data)
// {    ESP_LOGI("BUT","按键2") ; 
//     g_last_key = LV_KEY_PREV;
//     g_key_pressed = true;
// }

// static void but3_cb(void *arg, void *usr_data)
// {
//         ESP_LOGI("BUT","按键3") ; 
//     g_last_key = LV_KEY_ENTER;
//     g_key_pressed = true;
// }


// void init_but(){

// const button_config_t btn_cfg = {0};
// button_handle_t adc_btn = NULL;
// button_adc_config_t btn_adc_cfg = {
//     .unit_id = ADC_UNIT_2,
//     .adc_channel = 3,

// };


// // 按键1  1650
// btn_adc_cfg.button_index = 0;
// btn_adc_cfg.min = 1450;
// btn_adc_cfg.max = 1950;
// iot_button_new_adc_device(&btn_cfg, &btn_adc_cfg, &adc_btn);
// if(NULL == adc_btn) {
//     ESP_LOGE("BUT", "Button create failed");
// }
// iot_button_register_cb(adc_btn, BUTTON_SINGLE_CLICK, NULL, but1_cb,NULL);


// // 按键2 2200
// btn_adc_cfg.button_index = 1;
// btn_adc_cfg.min = 2000;
// btn_adc_cfg.max = 2400;
//  iot_button_new_adc_device(&btn_cfg, &btn_adc_cfg, &adc_btn);
// if(NULL == adc_btn) {
//     ESP_LOGE("BUT", "Button create failed");
// }
// iot_button_register_cb(adc_btn, BUTTON_SINGLE_CLICK, NULL, but2_cb,NULL);


// // 按键3 1114
// btn_adc_cfg.button_index = 2;
// btn_adc_cfg.min = 1000;
// btn_adc_cfg.max = 1300;
// iot_button_new_adc_device(&btn_cfg, &btn_adc_cfg, &adc_btn);
// if(NULL == adc_btn) {
//     ESP_LOGE("BUT", "Button create failed");
// }
// iot_button_register_cb(adc_btn, BUTTON_SINGLE_CLICK, NULL, but3_cb,NULL);



// }



static button_handle_t s_btn1 = NULL;
static button_handle_t s_btn2 = NULL;
static button_handle_t s_btn3 = NULL;

static void but1_cb(void *arg, void *usr_data)
{
    ESP_LOGI("BUT", "按键1");
    g_last_key = LV_KEY_NEXT;
    g_key_pressed = true;
}

static void but2_cb(void *arg, void *usr_data)
{
    ESP_LOGI("BUT", "按键2");
    g_last_key = LV_KEY_PREV;
    g_key_pressed = true;
}

static void but3_cb(void *arg, void *usr_data)
{
    ESP_LOGI("BUT", "按键3");
    g_last_key = LV_KEY_ENTER;
    g_key_pressed = true;


}

void init_but(void)
{
    const button_config_t btn_cfg = {0};

    button_adc_config_t btn_adc_cfg = {
        .unit_id     = ADC_UNIT_2,
        .adc_channel = 3,
    };

    /* ========== 按键 1：1650 ========== */
    btn_adc_cfg.button_index = 0;
    btn_adc_cfg.min = 1450;
    btn_adc_cfg.max = 1950;

    iot_button_new_adc_device(&btn_cfg, &btn_adc_cfg, &s_btn1);
    if (s_btn1 == NULL) {
        ESP_LOGE("BUT", "Button1 create failed");
    } else {
        iot_button_register_cb(s_btn1, BUTTON_SINGLE_CLICK, NULL, but1_cb, NULL);
    }

    /* ========== 按键 2：2200 ========== */
    btn_adc_cfg.button_index = 1;
    btn_adc_cfg.min = 2000;
    btn_adc_cfg.max = 2400;

    iot_button_new_adc_device(&btn_cfg, &btn_adc_cfg, &s_btn2);
    if (s_btn2 == NULL) {
        ESP_LOGE("BUT", "Button2 create failed");
    } else {
        iot_button_register_cb(s_btn2, BUTTON_SINGLE_CLICK, NULL, but2_cb, NULL);
    }

    /* ========== 按键 3：1114 ========== */
    btn_adc_cfg.button_index = 2;   // ⚠️ 不要用 2
    btn_adc_cfg.min = 700;
    btn_adc_cfg.max = 1300;

    iot_button_new_adc_device(&btn_cfg, &btn_adc_cfg, &s_btn3);
    if (s_btn3 == NULL) {
        ESP_LOGE("BUT", "Button3 create failed");
    } else {
        iot_button_register_cb(s_btn3, BUTTON_SINGLE_CLICK, NULL, but3_cb, NULL);
    }
}

