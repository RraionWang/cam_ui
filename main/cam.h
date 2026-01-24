#pragma once 

#include "stdbool.h"
#include "lvgl.h"


extern volatile bool g_take_photo  ; 
extern bool g_camera_streaming ;  
void cam_init_and_start(lv_obj_t *ui_container)  ; 



bool generate_next_image_name_fast(const char *dir_path,
                                   char *out_name,
                                   size_t out_len) ; 



 void rgb565_to_rgb888(
    const uint16_t *src,
    uint8_t *dst,
    int pixel_count); 
