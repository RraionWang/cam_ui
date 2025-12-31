#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *main;
    lv_obj_t *shot;
    lv_obj_t *browser_page;
    lv_obj_t *letter_page;
    lv_obj_t *record_page;
    lv_obj_t *video_page;
    lv_obj_t *bt_browser_pics;
    lv_obj_t *bt_shot;
    lv_obj_t *bt_letter;
    lv_obj_t *bt_record;
    lv_obj_t *bt_video;
    lv_obj_t *bt_back_from_shot;
    lv_obj_t *bt_back_from_browser;
    lv_obj_t *file_list_obj;
    lv_obj_t *bt_back_from_letter;
    lv_obj_t *bt_back_from_record;
    lv_obj_t *bt_record_from_record_page;
    lv_obj_t *bt_back_from_video;
    lv_obj_t *shot_window_obj;
    lv_obj_t *obj0;
    lv_obj_t *pic_window_obj;
    lv_obj_t *letter_text_obj;
    lv_obj_t *list_wav;
    lv_obj_t *obj1;
    lv_obj_t *bt_video_from_video_page;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_MAIN = 1,
    SCREEN_ID_SHOT = 2,
    SCREEN_ID_BROWSER_PAGE = 3,
    SCREEN_ID_LETTER_PAGE = 4,
    SCREEN_ID_RECORD_PAGE = 5,
    SCREEN_ID_VIDEO_PAGE = 6,
};

void create_screen_main();
void tick_screen_main();

void create_screen_shot();
void tick_screen_shot();

void create_screen_browser_page();
void tick_screen_browser_page();

void create_screen_letter_page();
void tick_screen_letter_page();

void create_screen_record_page();
void tick_screen_record_page();

void create_screen_video_page();
void tick_screen_video_page();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/