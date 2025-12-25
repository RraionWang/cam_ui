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

      ESP_ERROR_CHECK(app_lcd_init());
      ESP_ERROR_CHECK(app_lvgl_init());
  
    ui_init();
    init_but();

    
    binding_key() ;

    fill_jpg_list(objects.file_list_obj) ; 


    xTaskCreate(tick_task, "tick_task", 8192, NULL, 4, NULL);
    
    cam_init_and_start(objects.shot_window_obj);

    
}


