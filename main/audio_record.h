#pragma once
#include "stdbool.h"
#include "lvgl.h"



 void record_task(void *arg) ; 
 void i2s_rx_init(void)  ; 
 void fill_wav_list(lv_obj_t *list);
 void set_wav_list_obj(lv_obj_t *list);
void enable_speaker();
void disable_speaker();
void audio_play_init(void) ; 
void stop_playback(void) ;
void audio_record_ui_poll(void)  ; 
void ns4168_en_init() ; 
 void i2s_tx_init(void) ; 