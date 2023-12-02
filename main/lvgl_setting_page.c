
#include "lvgl_inc.h"

static lv_obj_t *setting_scr;
// LP_I18N_STRING_START
const char *SheZhi_TEXT[] = {
    "Wifi",
    "General",
    "Display",
    "Strong",
};
// LP_I18N_STRING_END
static const lp_lvgl_page_cb lv_page_cb_arr[] = // 界面或事件处理
    {
        view_page_wifi_setting,
        view_page_general,
        view_page_display_setting,
        view_page_strong,
};

/// @brief app 列表被点击了
/// @param e
static void setting_page_btn_callback(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    int page_id = (uint32_t)lv_obj_get_user_data(btn);

    if (code == LV_EVENT_SHORT_CLICKED)
    {
        APP_LOGD("setting_page_btn_callback %d\r\n", page_id);
        lp_lvgl_page_add_next_with_init(lv_page_cb_arr[page_id], 1);
    }
}

/// @brief 设置界面被删除
/// @param e
static void setting_page_clsoe_callback(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_DELETE)
    {
    }
}

int view_page_setting(lv_obj_t *last_obj)
{
    int i = 0;
    lv_obj_t *btn_obj, *lab_obj;

    setting_scr = last_obj;

    for (i = 0; i < ARR_SIZE(SheZhi_TEXT); i++)
    {
        btn_obj = lv_btn_create(setting_scr);
        lv_obj_set_user_data(btn_obj, (void *)i);
        lv_obj_remove_style_all(btn_obj);
        lv_obj_set_pos(btn_obj, 20, i * TFT_OBJ_HEIGHT_GAP);
        lv_obj_set_size(btn_obj, 200, TFT_OBJ_HEIGHT);
        lv_obj_set_style_radius(btn_obj, 10, 0);
        lv_obj_set_style_bg_color(btn_obj, lv_color_hex(0xFF), 0);
        lv_obj_set_style_bg_opa(btn_obj, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(btn_obj, lv_color_hex(0xFF), LV_STATE_FOCUSED);
        lv_obj_set_style_bg_opa(btn_obj, LV_OPA_80, LV_STATE_FOCUSED);
        lv_obj_set_style_bg_opa(btn_obj, LV_OPA_60, LV_STATE_PRESSED);
        lv_obj_add_flag(btn_obj, LV_OBJ_FLAG_EVENT_BUBBLE);

        lab_obj = lv_label_create(btn_obj);
        lv_label_set_text(lab_obj, SheZhi_TEXT[i]);
        lv_obj_set_style_text_color(lab_obj, lv_color_white(), 0);
        lv_obj_set_align(lab_obj, LV_ALIGN_LEFT_MID);

        lv_obj_add_event_cb(btn_obj, setting_page_btn_callback, LV_EVENT_SHORT_CLICKED, NULL);
    }

    lv_obj_add_event_cb(last_obj, setting_page_clsoe_callback, LV_EVENT_DELETE, 0);
    return 0;
}
