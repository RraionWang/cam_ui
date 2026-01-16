#pragma once 

#include "stdbool.h"
#include "lvgl.h"


extern volatile bool g_take_photo  ; 
extern bool g_camera_streaming ;  
void cam_init_and_start(lv_obj_t *ui_container)  ; 

 void rgb565_to_rgb888(
    const uint16_t *src,
    uint8_t *dst,
    int pixel_count); 
