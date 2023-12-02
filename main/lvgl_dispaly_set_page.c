#include "lvgl_inc.h"
static lv_obj_t *display_set_scr;

/// @brief 语言选择
/// @param e
static void display_language_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    char *temp;

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        temp = lp_i18n_lang_get(lv_dropdown_get_selected(obj));
        lplib_language_set(temp);
        lp_i18n_location_set(temp);
        APP_LOGD("language Option: %s\r\n", temp);
    }
}

/// @brief 自动锁屏时间
/// @param e
static void display_time_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    char buf[4], tnum = 0;

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        lv_dropdown_get_selected_str(obj, buf, sizeof(buf));

        tnum = lv_dropdown_get_selected(obj);
        lplib_bl_time_set(tnum);
        APP_LOGD("bt time Option: %s\r\n", buf);
    }
}

/// @brief 背光亮度调整事件
/// @param e
static void slider_event_cb(lv_event_t *e)
{
    uint8_t temp;
    lv_obj_t *slider = lv_event_get_target(e);
    temp = (int)lv_slider_get_value(slider);

    lplib_bl_value_set(temp);
}

// 设置显示相关的
int view_page_display_setting(lv_obj_t *last_obj)
{
    lv_obj_t *lab;
    char *lang_now, *temp;
    int lang_index = 0, lang_num, i;

    display_set_scr = last_obj;

    lab = lv_label_create(display_set_scr);
    lv_obj_set_pos(lab, 0, 15);
    lv_label_set_text(lab, lp_i18n_string("Lang."));

    lv_obj_t *dd = lv_dropdown_create(display_set_scr);
    lv_obj_set_pos(dd, 100, 0);
    lv_obj_set_height(dd, TFT_OBJ_HEIGHT);

    lang_now = lplib_language_get();
    lang_num = lp_i18n_lang_num_get();
    if (lang_num) // bug  新建的 不该有多余数据存在
    {
        lv_dropdown_clear_options(dd);
    }
    for (i = 0; i < lang_num; i++)
    {
        temp = lp_i18n_lang_get(i);
        if (0 == strcmp(temp, lang_now))
        {
            lang_index = i;
        }
        lv_dropdown_add_option(dd, lp_i18n_string(temp), LV_DROPDOWN_POS_LAST);
    }
    if (lang_num) // i18n已经被设置了
    {
        lv_dropdown_set_selected(dd, lang_index);
    }
    else // 默认en
    {
        lv_dropdown_set_options_static(dd, "en");
    }
    lv_obj_add_event_cb(dd, display_language_event_handler, LV_EVENT_VALUE_CHANGED, 0);

    lab = lv_label_create(display_set_scr);
    lv_obj_set_pos(lab, 0, 65);
    lv_label_set_text(lab, lp_i18n_string("Lock"));

    dd = lv_dropdown_create(display_set_scr);
    lv_obj_set_pos(dd, 100, TFT_OBJ_HEIGHT_GAP);
    lv_obj_set_height(dd, TFT_OBJ_HEIGHT);
    lv_dropdown_set_options_static(dd, "10\n"
                                       "30\n"
                                       "60\n"
                                       "120\n"
                                       "180");
    lv_dropdown_set_selected(dd, lplib_bl_time_get());
    lv_obj_add_event_cb(dd, display_time_event_handler, LV_EVENT_VALUE_CHANGED, (void *)1);

    lab = lv_label_create(display_set_scr);
    lv_obj_set_pos(lab, 0, TFT_OBJ_HEIGHT_GAP * 2);
    lv_label_set_text(lab, lp_i18n_string("Luminance"));

    lv_obj_t *slider = lv_slider_create(display_set_scr);
    lv_obj_set_pos(slider, 30, TFT_OBJ_HEIGHT_GAP * 2 + 35);
    lv_obj_set_size(slider, 180, 40);
    lv_slider_set_value(slider, lplib_bl_value_get(), LV_ANIM_OFF);
    lv_obj_add_event_cb(slider, slider_event_cb, LV_EVENT_VALUE_CHANGED, NULL);
    return 0;
}