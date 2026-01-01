#ifndef EEZ_LVGL_UI_VARS_H
#define EEZ_LVGL_UI_VARS_H

#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// enum declarations



// Flow global variables

enum FlowGlobalVariables {
    FLOW_GLOBAL_VARIABLE_RECORD_LABEL_CONTENT = 0
};

// Native global variables

extern int32_t get_var_screen_id();
extern void set_var_screen_id(int32_t value);
extern const char *get_var_shot_info();
extern void set_var_shot_info(const char *value);
extern bool get_var_is_record();
extern void set_var_is_record(bool value);


#ifdef __cplusplus
}
#endif

#endif /*EEZ_LVGL_UI_VARS_H*/