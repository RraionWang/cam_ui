#pragma once
#include "stdbool.h"
#include "lvgl.h"


void init_ns4168_gpio_pin();
 void record_task(void *arg) ; 
 void i2s_rx_init(void)  ; 
 void fill_wav_list(lv_obj_t *list);
 void set_wav_list_obj(lv_obj_t *list);
 void start_record();
 void stop_record();
