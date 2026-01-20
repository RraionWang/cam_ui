#include "actions.h"
#include "esp_log.h"
#include "my_fs.h"
#include "screens.h"
#include "vars.h"
#include "lcd.h"


void action_prev_pic(lv_event_t *e)
{
  // TODO: Implement action prev_pic here
  ESP_LOGI("EEZ", "上一幅画");
}

void action_next_pic(lv_event_t *e)
{
  // TODO: Implement action next_pic here
  ESP_LOGI("EEZ", "下一幅画");
  //    load_sd_jpg_to_obj(objects.pic_window_obj) ;
}

void action_show_pic(lv_event_t *e)
{
  ESP_LOGI("测hi", "测试");
}

#include "cam.h"

#include "lcd.h"

// 每次切换页面的之后执行焦点组函数

void action_set_focus_group(lv_event_t *e)
{

  int id = get_var_screen_id();
  ESP_LOGI("TAG", "当前id为%d", id);

  switch (id)
  {
  case 0:
    /* code */
    lv_indev_set_group(indev, g_focus_group_main);
    g_camera_streaming = false;
    ESP_LOGI("TAG", "切换焦点组为main");
    break;

  case 1:
    lv_indev_set_group(indev, g_focus_group_browser_page);
    g_camera_streaming = false;
    ESP_LOGI("TAG", "切换焦点组为浏览页面");
    break;

  case 2:

    lv_indev_set_group(indev, g_focus_group_shot);
    g_camera_streaming = true;
    ESP_LOGI("TAG", "切换焦点组为拍照页面");

    break;

  case 3:

    lv_indev_set_group(indev, g_focus_group_letter_page);
    g_camera_streaming = false;
    ESP_LOGI("TAG", "切换焦点组为信件页面");

    break;


  default:
    ESP_LOGI("TAG", "切换焦点组为main页面");
    lv_indev_set_group(indev, g_focus_group_main);
    break;
  }

  // TODO: Implement action set_focus_group here
}

// 重新刷新sd卡
void action_refresh_sd(lv_event_t *e)
{
  // TODO: Implement action refresh_sd here
  fill_jpg_list(objects.file_list_obj);
}





// 播放文件
void action_play_func(lv_event_t *e) {
    // TODO: Implement action play_func here
}

#include "poker.h"

void action_pre_poker(lv_event_t *e) {
    ESP_LOGI("POCKER","上一个滤镜");

    int id = get_var_filter_id();

    id--;
    if (id < 0) {
        id = POKER_FILTER_MAX - 1;   // 循环到最后一个
    }

    set_var_filter_id(id);

    ESP_LOGI("POCKER","当前滤镜 ID = %d", id);
    set_var_pocker_name(poker_filter_name(id)) ; 
}


void action_next_poker(lv_event_t *e) {
    ESP_LOGI("POCKER","下一个滤镜");

    int id = get_var_filter_id();

    id++;
    if (id >= POKER_FILTER_MAX) {
        id = 0;   // 回到第一个
    }

    set_var_filter_id(id);

    ESP_LOGI("POCKER","当前滤镜 ID = %d", id);
        set_var_pocker_name(poker_filter_name(id)) ; 
}


