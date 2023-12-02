#include "lvgl_inc.h"

static lv_obj_t *wifi_set_scr = 0;
static lv_timer_t *timer = 0;
static lpbsp_wifi_ap_info_t *ap_info = 0;
static uint8_t ap_selcel_num = -1;
static lv_obj_t *wifi_ap_list_obj;
static lv_obj_t *wifi_connect_scr = 0; // 连接的背景

static void timer_cb(lv_timer_t *t);

static void timer_connect_cb(lv_timer_t *t)
{
    static uint8_t conn_flg = 0;
    char addip[20];

    if (lpbsp_wifi_event(E_WIFI_CONNECT))
    {
        lpbsp_wifi_addr_ip(addip);
        lv_label_set_text_fmt(wifi_ap_list_obj, "ip:%s", addip);
    }
    else
    {
        if (0 == conn_flg)
            lv_label_set_text(wifi_ap_list_obj, "connect .");
        if (1 == conn_flg)
            lv_label_set_text(wifi_ap_list_obj, "connect . .");
        if (2 == conn_flg)
            lv_label_set_text(wifi_ap_list_obj, "connect . . .");

        conn_flg++;
        if (conn_flg > 2)
            conn_flg = 0;
    }
}
static void kb_event_cb(lv_event_t *e)
{
    char tbuf[16];
    lv_keyboard_t *kb = (lv_keyboard_t *)lv_event_get_target(e);
    lv_event_code_t code = lv_event_get_code(e);
    if (code == LV_EVENT_CANCEL)
    {
        lp_lvgl_page_back(wifi_set_scr);
    }
    else if (code == LV_EVENT_READY)
    {
        APP_LOGD("connect[%s][%s]\r\n", ap_info[ap_selcel_num].ssid, lv_textarea_get_text(kb->ta));
        strcpy(tbuf, (char *)lv_textarea_get_text(kb->ta));

        if (8 > strlen(tbuf))
        {
            lv_obj_t *mbox1 = lv_msgbox_create(NULL, "Error", "password max 8 len.", 0, true);
            lv_obj_center(mbox1);
            //  password last 8 byte
            return;
        }
        else
        {
            APP_LOGD("lf_api_wifi_connect[%s]\r\n", tbuf);
            lpbsp_wifi_connect((char *)ap_info[ap_selcel_num].ssid, tbuf);

            lp_lvgl_page_back(wifi_set_scr);
            timer = lv_timer_create(timer_connect_cb, 500, 0);
        }
    }
}
// wifi open /close
static void wifi_setting_page_open_callback(lv_event_t *e)
{
    uint8_t temp;
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        temp = lv_obj_get_state(obj) & LV_STATE_CHECKED;

        APP_LOGD("wifi_open: %d\r\n", temp);

        if (temp)
        {
            lpbsp_wifi_scan();
            if (0 == timer)
            {
                timer = lv_timer_create(timer_cb, 200, 0);
            }
        }
        else // 如果有list 就清空他们
        {
            lv_obj_clean(wifi_ap_list_obj);
            lv_label_set_text(wifi_ap_list_obj, lp_i18n_string("Net"));
        }
    }
}

static void wifi_setting_page_btn_callback(lv_event_t *e)
{
    lv_obj_t *lab;
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    int page_id = (uint32_t)lv_obj_get_user_data(btn);

    if (code == LV_EVENT_SHORT_CLICKED)
    {
        wifi_connect_scr = lp_lvgl_page_add_next_no_sysbar(wifi_set_scr);
        lv_obj_clear_flag(wifi_connect_scr, LV_OBJ_FLAG_SCROLL_CHAIN);

        ap_selcel_num = page_id;
        lab = lv_label_create(wifi_connect_scr); // 添加文字
        lv_obj_set_pos(lab, 0, 0);
        lv_obj_set_size(lab, 240, 32);
        lv_label_set_text_fmt(lab, "%s %s", lp_i18n_string("Connect"), ap_info[page_id].ssid);
        lv_label_set_long_mode(lab, LV_LABEL_LONG_SCROLL); /*Break the long lines*/

        lv_obj_t *ta = lv_textarea_create(wifi_connect_scr);
        lv_textarea_set_one_line(ta, true);
        lv_obj_set_pos(ta, 5, 33);
        lv_obj_set_size(ta, 230, 53);

        lv_obj_t *kb = lv_keyboard_create(wifi_connect_scr);
        lv_obj_set_height(kb, 182);
        lv_obj_add_event_cb(kb, kb_event_cb, LV_EVENT_ALL, 0);
        lv_keyboard_set_textarea(kb, ta);

        lv_keyboard_set_mode(kb, LV_KEYBOARD_MODE_NUMBER);
    }
}

static void wifi_setting_page_clsoe_callback(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    APP_LOGD("lv_event_code_t %d \r\n", code);

    if (code == LV_EVENT_DELETE)
    {
        APP_LOGD("wifi page close\r\n");
        if (timer)
        {
            lv_timer_del(timer);
        }
        if (ap_info)
        {
            lpbsp_free(ap_info);
            ap_info = 0;
        }
    }
}

static void timer_cb(lv_timer_t *t)
{
    lv_obj_t *btn, *lab, *up_obj;
    int index = 0, ap_num = 0;

    ap_num = lpbsp_wifi_scan_ap_num();
    if (ap_num)
    {
        ap_info = lpbsp_malloc(sizeof(lpbsp_wifi_ap_info_t) * ap_num);
        if (0 == ap_info)
        {
            APP_LOGD("ram error\r\n");
            return;
        }

        ap_num = lpbsp_wifi_scan_ap_info(ap_info, ap_num);

        up_obj = wifi_ap_list_obj;
        for (index = 0; index < ap_num; index++)
        {
            btn = lv_btn_create(wifi_ap_list_obj);
            // lv_obj_add_flag(btn, LV_OBJ_FLAG_EVENT_BUBBLE);
            lv_obj_set_size(btn, 200, 45);
            lv_obj_align_to(btn, up_obj, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);
            up_obj = btn;

            lv_obj_set_user_data(btn, (void *)index);

            lv_obj_set_style_bg_opa(btn, LV_STATE_DEFAULT, 0);
            lv_obj_set_style_shadow_opa(btn, LV_STATE_DEFAULT, 0); // 这2行 让btn透明背景
            // 添加文字
            lab = lv_label_create(btn);
            lv_obj_set_style_text_color(lab, lv_color_hex(0), 0);
            lv_label_set_text_fmt(lab, "%s", ap_info[index].ssid);

            lv_obj_add_event_cb(btn, wifi_setting_page_btn_callback, LV_EVENT_SHORT_CLICKED, NULL);
        }
        lv_timer_del(timer);
        timer = 0;
    }
}
// wifi扫描链接
int view_page_wifi_setting(lv_obj_t *last_obj)
{
    lv_obj_t *lab;
    char addip[20];

    wifi_set_scr = last_obj; //----创建容器----//

    lab = lv_label_create(wifi_set_scr);
    lv_obj_set_pos(lab, 0, 0);
    lv_label_set_text(lab, "Wifi");

    lv_obj_t *sw = lv_switch_create(lab);
    lv_obj_set_pos(sw, 120, 0);
    lv_obj_set_size(sw, 115, 60);

    {
        lv_obj_add_state(sw, LV_STATE_CHECKED); /*Make the chekbox checked*/
    }
    lv_obj_add_event_cb(sw, wifi_setting_page_open_callback, LV_EVENT_VALUE_CHANGED, NULL);

    wifi_ap_list_obj = lv_label_create(wifi_set_scr);
    lv_obj_add_flag(wifi_ap_list_obj, LV_OBJ_FLAG_EVENT_BUBBLE);
    if (lpbsp_wifi_event(1))
    {
        lpbsp_wifi_addr_ip(addip);
        lv_label_set_text_fmt(wifi_ap_list_obj, "ip:%s", addip);
    }
    else
    {
        lv_label_set_text(wifi_ap_list_obj, lp_i18n_string("Net"));
    }
    lv_obj_align_to(wifi_ap_list_obj, lab, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 0);

    lv_obj_add_event_cb(wifi_set_scr, wifi_setting_page_clsoe_callback, LV_EVENT_DELETE, 0);

    {
        lpbsp_wifi_scan();
        timer = lv_timer_create(timer_cb, 200, 0);
    }
    return 0;
}