#pragma once 

#include "esp_err.h"
#include "lvgl.h"



 esp_err_t app_lcd_init(void) ; 
 esp_err_t app_lvgl_init(void) ;

void binding_key() ;


extern volatile uint32_t g_last_key;
extern volatile bool g_key_pressed;
extern lv_group_t*  g_focus_group ;



extern  lv_indev_t* indev  ;  


// 创建三个焦点组
extern lv_group_t* g_focus_group_main ;
extern lv_group_t* g_focus_group_shot ;
extern lv_group_t* g_focus_group_browser_page ;

