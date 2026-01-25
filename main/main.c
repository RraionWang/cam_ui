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
#include "myenc.h"


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

void print_ram_info()
{
  // 1. 内部 RAM (速度快，但容量小，通常给 DMA 或关键任务用)
  size_t internal_free = heap_caps_get_free_size(MALLOC_CAP_INTERNAL);

  // 2. 外部 PSRAM (容量大，速度稍慢，存放 UI 图片、大型缓冲区)
  size_t psram_free = heap_caps_get_free_size(MALLOC_CAP_SPIRAM);

  // 3. 历史上曾经出现过的最低剩余内存 (水位线，用于检查是否接近崩溃)
  size_t min_free = esp_get_minimum_free_heap_size();

  ESP_LOGI("ram", "\n--- 内存状态报告 ---\n");
  ESP_LOGI("ram", "内部 RAM 剩余: %d KB\n", internal_free / 1024);

  ESP_LOGI("ram", "外部 PSRAM 剩余: %d KB\n", psram_free / 1024);

  ESP_LOGI("ram", "历史最低水位线: %d KB\n", min_free / 1024);
  ESP_LOGI("ram", "--------------------\n");
}

static void print_mem_task(void *arg)
{

  while (1)
  {
    print_ram_info();
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}







void app_main(){

  //  xTaskCreate(print_mem_task, "printmeme", 4096, NULL, 5, NULL);

       xTaskCreatePinnedToCoreWithCaps(
    print_mem_task,          // 任务函数
    "cam_task",               // 任务名
   4096,                // 栈大小（单位：word，不是字节！）
    NULL,             // 参数
    5,                        // 优先级
    NULL,                     // TaskHandle_t*
    1,                        // 绑定 CPU1
    MALLOC_CAP_SPIRAM         // 👈 强制栈从 PSRAM 分配
);




    sdmmc_card_t* card = init_sdcard() ; 

       


    if(!card){
      set_var_sd_detect_info("检查SD卡!");
    }

    ESP_ERROR_CHECK(app_lcd_init());
    ESP_ERROR_CHECK(app_lvgl_init());

    start_jpeg_filter_task();
  
    ui_init();
    init_but();

    binding_key() ;

    fill_jpg_list(objects.file_list_obj) ; 

      
    // xTaskCreatePinnedToCore(tick_task, "tick_task", 8192*4, NULL, 5, NULL,0);

    
        xTaskCreatePinnedToCoreWithCaps(
    tick_task,          // 任务函数
    "tick_task",               // 任务名
    8192,                // 栈大小（单位：word，不是字节！）
    NULL,             // 参数
    5,                        // 优先级
    NULL,                     // TaskHandle_t*
    1,                        // 绑定 CPU1
    MALLOC_CAP_SPIRAM         // 👈 强制栈从 PSRAM 分配
        );


   
    
    cam_init_and_start(objects.shot_window_obj);

    
}


