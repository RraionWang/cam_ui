#ifndef EEZ_LVGL_UI_SCREENS_H
#define EEZ_LVGL_UI_SCREENS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _objects_t {
    lv_obj_t *shot;
    lv_obj_t *bt_back_from_shot;
    lv_obj_t *pre_pocker_btn;
    lv_obj_t *next_pocker_btn;
    lv_obj_t *shot_window_obj;
    lv_obj_t *status_led;
    lv_obj_t *obj0;
    lv_obj_t *obj1;
    lv_obj_t *obj2;
    lv_obj_t *obj3;
} objects_t;

extern objects_t objects;

enum ScreensEnum {
    SCREEN_ID_SHOT = 1,
};

void create_screen_shot();
void tick_screen_shot();

void tick_screen_by_id(enum ScreensEnum screenId);
void tick_screen(int screen_index);

void create_screens();


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_SCREENS_H*/