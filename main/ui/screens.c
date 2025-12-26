#include <string.h>

#include "screens.h"
#include "images.h"
#include "fonts.h"
#include "actions.h"
#include "vars.h"
#include "styles.h"
#include "ui.h"

#include <string.h>

objects_t objects;
lv_obj_t *tick_value_change_obj;

static void event_handler_cb_main_bt_browser_pics(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 0, 0, e);
    }
}

static void event_handler_cb_main_bt_shot(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 3, 0, e);
    }
}

static void event_handler_cb_main_bt_letter(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 5, 0, e);
    }
}

static void event_handler_cb_shot_bt_back_from_shot(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 2, 0, e);
    }
}

static void event_handler_cb_browser_page_bt_back_from_browser(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_PRESSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 0, 0, e);
    }
}

static void event_handler_cb_browser_page_file_list_obj(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_FOCUSED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 4, 0, e);
    }
}

static void event_handler_cb_letter_page_bt_back_from_letter(lv_event_t *e) {
    lv_event_code_t event = lv_event_get_code(e);
    void *flowState = lv_event_get_user_data(e);
    (void)flowState;
    
    if (event == LV_EVENT_CLICKED) {
        e->user_data = (void *)0;
        flowPropagateValueLVGLEvent(flowState, 2, 0, e);
    }
}

void create_screen_main() {
    void *flowState = getFlowState(0, 0);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.main = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 172);
    {
        lv_obj_t *parent_obj = obj;
        {
            // bt_browser_pics
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.bt_browser_pics = obj;
            lv_obj_set_pos(obj, 14, 10);
            lv_obj_set_size(obj, 100, 50);
            lv_obj_add_event_cb(obj, event_handler_cb_main_bt_browser_pics, LV_EVENT_ALL, flowState);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_cn_font_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_letter_space(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "浏览照片");
                }
            }
        }
        {
            // bt_shot
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.bt_shot = obj;
            lv_obj_set_pos(obj, 141, 10);
            lv_obj_set_size(obj, 100, 50);
            lv_obj_add_event_cb(obj, event_handler_cb_main_bt_shot, LV_EVENT_ALL, flowState);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xff54ac6d), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_cn_font_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "拍照");
                }
            }
        }
        {
            // bt_letter
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.bt_letter = obj;
            lv_obj_set_pos(obj, 14, 96);
            lv_obj_set_size(obj, 100, 50);
            lv_obj_add_event_cb(obj, event_handler_cb_main_bt_letter, LV_EVENT_ALL, flowState);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_cn_font_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "一封信");
                }
            }
        }
    }
    
    tick_screen_main();
}

void tick_screen_main() {
    void *flowState = getFlowState(0, 0);
    (void)flowState;
}

void create_screen_shot() {
    void *flowState = getFlowState(0, 1);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.shot = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 172);
    {
        lv_obj_t *parent_obj = obj;
        {
            // shot_window_obj
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.shot_window_obj = obj;
            lv_obj_set_pos(obj, 30, 20);
            lv_obj_set_size(obj, 260, 148);
        }
        {
            // bt_back_from_shot
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.bt_back_from_shot = obj;
            lv_obj_set_pos(obj, 302, 0);
            lv_obj_set_size(obj, 18, 15);
            lv_obj_add_event_cb(obj, event_handler_cb_shot_bt_back_from_shot, LV_EVENT_ALL, flowState);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffeb3a3a), LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "X");
                }
            }
        }
        {
            lv_obj_t *obj = lv_label_create(parent_obj);
            objects.obj0 = obj;
            lv_obj_set_pos(obj, 30, 1);
            lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
            lv_obj_set_style_text_font(obj, &ui_font_cn_font_16, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_label_set_text(obj, "");
        }
    }
    
    tick_screen_shot();
}

void tick_screen_shot() {
    void *flowState = getFlowState(0, 1);
    (void)flowState;
    {
        const char *new_val = evalTextProperty(flowState, 4, 3, "Failed to evaluate Text in Label widget");
        const char *cur_val = lv_label_get_text(objects.obj0);
        if (strcmp(new_val, cur_val) != 0) {
            tick_value_change_obj = objects.obj0;
            lv_label_set_text(objects.obj0, new_val);
            tick_value_change_obj = NULL;
        }
    }
}

void create_screen_browser_page() {
    void *flowState = getFlowState(0, 2);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.browser_page = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 172);
    {
        lv_obj_t *parent_obj = obj;
        {
            // bt_back_from_browser
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.bt_back_from_browser = obj;
            lv_obj_set_pos(obj, 295, 0);
            lv_obj_set_size(obj, 25, 26);
            lv_obj_add_event_cb(obj, event_handler_cb_browser_page_bt_back_from_browser, LV_EVENT_ALL, flowState);
            lv_obj_set_style_bg_color(obj, lv_color_hex(0xffd21111), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_14, LV_PART_MAIN | LV_STATE_DEFAULT);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_obj_set_style_text_font(obj, &ui_font_cn_font_16, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "<");
                }
            }
        }
        {
            // pic_window_obj
            lv_obj_t *obj = lv_obj_create(parent_obj);
            objects.pic_window_obj = obj;
            lv_obj_set_pos(obj, 191, 6);
            lv_obj_set_size(obj, 96, 160);
            lv_obj_set_style_pad_left(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_top(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_bottom(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_bg_opa(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_radius(obj, 0, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // file_list_obj
            lv_obj_t *obj = lv_list_create(parent_obj);
            objects.file_list_obj = obj;
            lv_obj_set_pos(obj, 0, 0);
            lv_obj_set_size(obj, 177, 172);
            lv_obj_add_event_cb(obj, event_handler_cb_browser_page_file_list_obj, LV_EVENT_ALL, flowState);
            lv_obj_add_state(obj, LV_STATE_FOCUSED|LV_STATE_FOCUS_KEY|LV_STATE_PRESSED);
            lv_obj_set_style_text_font(obj, &lv_font_montserrat_12, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_border_width(obj, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_left(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_pad_right(obj, 2, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
    }
    
    tick_screen_browser_page();
}

void tick_screen_browser_page() {
    void *flowState = getFlowState(0, 2);
    (void)flowState;
}

void create_screen_letter_page() {
    void *flowState = getFlowState(0, 3);
    (void)flowState;
    lv_obj_t *obj = lv_obj_create(0);
    objects.letter_page = obj;
    lv_obj_set_pos(obj, 0, 0);
    lv_obj_set_size(obj, 320, 172);
    {
        lv_obj_t *parent_obj = obj;
        {
            // letter_text_obj
            lv_obj_t *obj = lv_textarea_create(parent_obj);
            objects.letter_text_obj = obj;
            lv_obj_set_pos(obj, 0, 11);
            lv_obj_set_size(obj, 298, 156);
            lv_textarea_set_max_length(obj, 128);
            lv_textarea_set_text(obj, "大飞，这是给你的新年礼物，赶在了2025年之前给你做好，虽然没有那么完美，但是希望这一个世界上仅此一个的小相机，可以给你带来特殊的意义。在2025年遇到你真好。\n\n2025/12/27 小狮子");
            lv_textarea_set_one_line(obj, false);
            lv_textarea_set_password_mode(obj, false);
            lv_obj_set_style_text_font(obj, &ui_font_cn_font_16, LV_PART_MAIN | LV_STATE_DEFAULT);
        }
        {
            // bt_back_from_letter
            lv_obj_t *obj = lv_button_create(parent_obj);
            objects.bt_back_from_letter = obj;
            lv_obj_set_pos(obj, 298, 1);
            lv_obj_set_size(obj, 22, 15);
            lv_obj_add_event_cb(obj, event_handler_cb_letter_page_bt_back_from_letter, LV_EVENT_ALL, flowState);
            {
                lv_obj_t *parent_obj = obj;
                {
                    lv_obj_t *obj = lv_label_create(parent_obj);
                    lv_obj_set_pos(obj, 0, 0);
                    lv_obj_set_size(obj, LV_SIZE_CONTENT, LV_SIZE_CONTENT);
                    lv_obj_set_style_align(obj, LV_ALIGN_CENTER, LV_PART_MAIN | LV_STATE_DEFAULT);
                    lv_label_set_text(obj, "X");
                }
            }
        }
    }
    
    tick_screen_letter_page();
}

void tick_screen_letter_page() {
    void *flowState = getFlowState(0, 3);
    (void)flowState;
}


static const char *screen_names[] = { "Main", "shot", "browser_page", "letter_page" };
static const char *object_names[] = { "main", "shot", "browser_page", "letter_page", "bt_browser_pics", "bt_shot", "bt_letter", "bt_back_from_shot", "bt_back_from_browser", "file_list_obj", "bt_back_from_letter", "shot_window_obj", "obj0", "pic_window_obj", "letter_text_obj" };


typedef void (*tick_screen_func_t)();
tick_screen_func_t tick_screen_funcs[] = {
    tick_screen_main,
    tick_screen_shot,
    tick_screen_browser_page,
    tick_screen_letter_page,
};
void tick_screen(int screen_index) {
    tick_screen_funcs[screen_index]();
}
void tick_screen_by_id(enum ScreensEnum screenId) {
    tick_screen_funcs[screenId - 1]();
}

void create_screens() {
    eez_flow_init_screen_names(screen_names, sizeof(screen_names) / sizeof(const char *));
    eez_flow_init_object_names(object_names, sizeof(object_names) / sizeof(const char *));
    
    lv_disp_t *dispp = lv_disp_get_default();
    lv_theme_t *theme = lv_theme_default_init(dispp, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_RED), false, LV_FONT_DEFAULT);
    lv_disp_set_theme(dispp, theme);
    
    create_screen_main();
    create_screen_shot();
    create_screen_browser_page();
    create_screen_letter_page();
}
