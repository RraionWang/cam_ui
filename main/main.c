#include "lcd.h"
#include "ui.h"
#include "freertos/FreeRTOS.h"
#include "freertos/FreeRTOSConfig.h"
#include "sdmmc_cmd.h"
#include "driver/sdmmc_host.h"
#include "my_fs.h"
#include "key.h"
#include "screens.h"
#include "esp_lvgl_port.h"
#include "esp_adc_cal.h"
#include "driver/adc.h"
#include "vars.h"
#include "cam.h"


void tick_task(void *pvParam)
{


  while (1)
  {
    if (lvgl_port_lock(pdMS_TO_TICKS(50))) {
      ui_tick();
      lvgl_port_unlock();
    }
   
    vTaskDelay(pdMS_TO_TICKS(100)); // 每隔 1 秒执行一次
  }
}

void app_main(){

    sdmmc_card_t* card = init_sdcard() ; 

    if(!card){
      set_var_sd_detect_info("检查SD卡!");
    }

    ESP_ERROR_CHECK(app_lcd_init());
    ESP_ERROR_CHECK(app_lvgl_init());
  
    ui_init();
    init_but();

    binding_key() ;

    fill_jpg_list(objects.file_list_obj) ; 

      
    xTaskCreatePinnedToCore(tick_task, "tick_task", 8192*2, NULL, 5, NULL,0);
   
    
    cam_init_and_start(objects.shot_window_obj);
}


