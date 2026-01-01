#ifndef EEZ_LVGL_UI_EVENTS_H
#define EEZ_LVGL_UI_EVENTS_H

#include <lvgl.h>

#ifdef __cplusplus
extern "C" {
#endif

extern void action_prev_pic(lv_event_t * e);
extern void action_next_pic(lv_event_t * e);
extern void action_show_pic(lv_event_t * e);
extern void action_set_focus_group(lv_event_t * e);
extern void action_refresh_sd(lv_event_t * e);
extern void action_record_func(lv_event_t * e);
extern void action_update_wav_list(lv_event_t * e);
extern void action_play_func(lv_event_t * e);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_EVENTS_H*/