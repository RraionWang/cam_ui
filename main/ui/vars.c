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



  bool is_record;

bool get_var_is_record() {

    return is_record;
}

void set_var_is_record(bool value) {
    is_record = value;
}


char sd_detect_info[100] = { 0 };

const char *get_var_sd_detect_info() {
    return sd_detect_info;
}

void set_var_sd_detect_info(const char *value) {
    strncpy(sd_detect_info, value, sizeof(sd_detect_info) / sizeof(char));
    sd_detect_info[sizeof(sd_detect_info) / sizeof(char) - 1] = 0;
}



int32_t filter_id;

int32_t get_var_filter_id() {
    return filter_id;
}

void set_var_filter_id(int32_t value) {
    filter_id = value;
}



char pocker_name[100] = { 0 };

const char *get_var_pocker_name() {
    return pocker_name;
}

void set_var_pocker_name(const char *value) {
    strncpy(pocker_name, value, sizeof(pocker_name) / sizeof(char));
    pocker_name[sizeof(pocker_name) / sizeof(char) - 1] = 0;
}
