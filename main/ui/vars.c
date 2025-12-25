#include "vars.h"
#include "lcd.h"

int32_t screen_id;

int32_t get_var_screen_id() {
    return screen_id;
}

void set_var_screen_id(int32_t value) {
 
    screen_id = value;
}
