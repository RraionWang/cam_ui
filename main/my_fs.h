#pragma once 
#include "esp_err.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"


sdmmc_card_t*  init_sdcard() ; 
void load_sd_jpg_to_obj(const char* path ,  lv_obj_t *obj) ;
void fill_jpg_list(lv_obj_t *list)  ; 