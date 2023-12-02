
#include "lvgl_inc.h"

#define APP_VERSION 1

static lv_obj_t *general_src;

static int view_page_about(lv_obj_t *last_obj);

// LP_I18N_STRING_START
const char *GENERAL_TEXT[] = {
    "About",
    "Up",
    "test",
    "Rst",
};
// LP_I18N_STRING_END

static void msgbox_resrt_btn_callback(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        lpbsp_restart();
    }
}

/// @brief 复位
/// @param last_obj
/// @return
static int view_page_rerst(lv_obj_t *last_obj)
{
    static const char *btns[] = {"Yes", 0};

    lv_obj_t *mbox1 = lv_msgbox_create(NULL, "Tips", "Rerst", btns, true);
    lv_obj_add_event_cb(mbox1, msgbox_resrt_btn_callback, LV_EVENT_VALUE_CHANGED, 0);
    lv_obj_center(mbox1);

    return 0;
}
static const lp_lvgl_page_cb lv_page_cb_arr[] = // 界面或事件处理
    {
        view_page_about,
        view_page_up,
        view_page_test,
        view_page_rerst,
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
        lp_lvgl_page_add_next_with_init(lv_page_cb_arr[page_id], 1);
    }
}

int view_page_general(lv_obj_t *last_obj)
{

    int i = 0;
    lv_obj_t *btn_obj, *lab_obj;
    general_src = last_obj; //----创建容器----//

    for (i = 0; i < ARR_SIZE(GENERAL_TEXT); i++)
    {
        btn_obj = lv_btn_create(general_src);
        lv_obj_set_user_data(btn_obj, (void *)i);
        lv_obj_remove_style_all(btn_obj);
        lv_obj_set_pos(btn_obj, 20, i * TFT_OBJ_HEIGHT_GAP);
        lv_obj_set_size(btn_obj, 200, TFT_OBJ_HEIGHT);
        lv_obj_set_style_radius(btn_obj, 10, 0);
        lv_obj_set_style_bg_color(btn_obj, lv_color_hex(0xff), 0);
        lv_obj_set_style_bg_opa(btn_obj, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(btn_obj, lv_color_hex(0xff), LV_STATE_FOCUSED);
        lv_obj_set_style_bg_opa(btn_obj, LV_OPA_80, LV_STATE_FOCUSED);
        lv_obj_set_style_bg_opa(btn_obj, LV_OPA_60, LV_STATE_PRESSED);
        lv_obj_add_flag(btn_obj, LV_OBJ_FLAG_EVENT_BUBBLE);

        lab_obj = lv_label_create(btn_obj);
        lv_label_set_text(lab_obj, GENERAL_TEXT[i]);
        lv_obj_set_style_text_color(lab_obj, lv_color_white(), 0);
        lv_obj_set_align(lab_obj, LV_ALIGN_LEFT_MID);

        lv_obj_add_event_cb(btn_obj, setting_page_btn_callback, LV_EVENT_SHORT_CLICKED, NULL);
    }

    return 0;
}
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////
// LP_I18N_STRING_START
const char *GENERAL_ABOUT_TEXT[] = {
    "Name",
    "Version",
    "Model",
    "Sn",
    0,
};

const char *GENERAL_ABOUT_SSD_TEXT[] = {
    "Apps",
    "Total",
    "Use",
    0,
};

const char *GENERAL_ABOUT_NET_TEXT[] = {
    "Wifi",
    "IP",
    0,
};
// LP_I18N_STRING_END

static const char *GENERALS_TEXT[] = {GENERAL_ABOUT_TEXT, GENERAL_ABOUT_SSD_TEXT, GENERAL_ABOUT_NET_TEXT};

/// @brief 关于参数获取
/// @param num index
/// @param tbuf 输出buf
/// @return
static int view_page_about_get(int num, char *tbuf)
{
    switch (num)
    {
    case 0:
        strcpy(tbuf, "Lwatch");
        break;
    case 1:
        sprintf(tbuf, "%d", APP_VERSION);
        break;
    case 2:
        lpbsp_sn_get(tbuf, 9);
        tbuf[2] = 0;
        break;
    case 3:
        lpbsp_sn_get(tbuf, 9);
        break;
    }
    return 0;
}
/// @brief 关于参数获取
/// @param num index
/// @param tbuf 输出buf
/// @return
static int view_page_ssd_get(int num, char *tbuf)
{
    int total, free_bs;

    lp_fs_volume("/C", &total, &free_bs);
    switch (num)
    {
    case 0:
        strcpy(tbuf, "13");
        break;
    case 1:
        sprintf(tbuf, "%dMB", total / 1048576);
        break;
    case 2:
        sprintf(tbuf, "%dMB", free_bs / 1048576);
        break;
    }
    return 0;
}
/// @brief 关于参数获取
/// @param num index
/// @param tbuf 输出buf
/// @return
static int view_page_net_get(int num, char *tbuf)
{
    switch (num)
    {
    case 0:
        strcpy(tbuf, "ad:ad:ad:ad:ad:Ad");
        break;
    case 1:
        lpbsp_wifi_addr_ip(tbuf);
        break;
    }
    return 0;
}
static int view_page_index_get(int page, int num, char *tbuf)
{
    switch (page)
    {
    case 0:
        return view_page_about_get(num, tbuf);
    case 1:
        return view_page_ssd_get(num, tbuf);
    case 2:
        return view_page_net_get(num, tbuf);
    }
    return 0;
}

static int view_page_about(lv_obj_t *last_obj)
{
    lv_obj_t *obj_lab, *obj_base;
    lv_obj_t *btn_obj;
    int i = 0, next_y = 0, page = 0, index_num = 0;
    char tbuf[32], **temp;

    for (page = 0; page < ARR_SIZE(GENERALS_TEXT); page++)
    {
        temp = GENERALS_TEXT[page];
        for (i = 0; i < 10; i++)
            if (0 == temp[i])
                break;
        index_num = i; // 这个page 多线item

        obj_base = lv_obj_create(last_obj);
        lv_obj_set_pos(obj_base, 5, next_y);
        lv_obj_set_size(obj_base, 230, index_num * 35 + 30);
        next_y += index_num * 35 + 50;

        for (i = 0; i < index_num; i++) // 显示item
        {
            obj_lab = lv_label_create(obj_base);
            lv_obj_remove_style_all(obj_lab);
            lv_obj_set_style_text_font(obj_lab, &lv_font_montserrat_22, 0);
            lv_obj_set_pos(obj_lab, 0, i * 35);
            lv_label_set_text(obj_lab, lp_i18n_string(temp[i]));

            view_page_index_get(page, i, tbuf);
            obj_lab = lv_label_create(obj_base);
            lv_obj_remove_style_all(obj_lab);
            lv_obj_set_style_text_font(obj_lab, &lv_font_montserrat_22, 0);
            lv_obj_set_style_text_align(obj_lab, LV_TEXT_ALIGN_RIGHT, 0);
            lv_obj_set_pos(obj_lab, 80, i * 35);
            lv_obj_set_width(obj_lab, 120);
            lv_label_set_text(obj_lab, tbuf);
            lv_label_set_long_mode(obj_lab, LV_LABEL_LONG_SCROLL_CIRCULAR);

            obj_lab = lv_line_create(obj_base);
            lv_obj_set_pos(obj_lab, 0, i * 35 + 30);
            lv_obj_set_size(obj_lab, 200, 5);
            lv_obj_set_scrollbar_mode(obj_lab, LV_SCROLLBAR_MODE_OFF);

            lv_obj_set_style_line_color(obj_lab, lv_color_make(40, 40, 40), LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_line_width(obj_lab, 1, LV_PART_MAIN | LV_STATE_DEFAULT);
            lv_obj_set_style_line_rounded(obj_lab, true, LV_PART_MAIN | LV_STATE_DEFAULT);
            const static lv_point_t screen_1_line_1[2] = {
                {0, 3},
                {195, 3},
            };
            lv_line_set_points(obj_lab, screen_1_line_1, 2);
        }
    }
    return 0;
}