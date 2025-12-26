#include "vars.h"
#include "lcd.h"
#include "string.h"


int32_t screen_id;

int32_t get_var_screen_id() {
    return screen_id;
}

void set_var_screen_id(int32_t value) {
 
    screen_id = value;
}



char shot_info[100] = { 0 };

const char *get_var_shot_info() {
    return shot_info;
}

void set_var_shot_info(const char *value) {
    strncpy(shot_info, value, sizeof(shot_info) / sizeof(char));
    shot_info[sizeof(shot_info) / sizeof(char) - 1] = 0;
}

