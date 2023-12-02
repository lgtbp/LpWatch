
#include "lvgl_inc.h"

static lv_obj_t *lab_log_obj;
static lv_timer_t *timer;
static char *test_buf;
#define TEST_MODE_WIFI_SSID "tes"
#define TEST_MODE_WIFI_PASSWORD "12345678"

static lv_obj_t *obj_up;
static lv_obj_t *obj_up_sitting;
static char up_site_url[128];

static void view_page_close_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (LV_EVENT_DELETE == code)
    {
        lvgl_sleep_time_ms(15000);
        lpbsp_free(test_buf);
        lv_timer_del(timer);
        timer = 0;
    }
}

static void timer_cb(lv_timer_t *timer)
{
    int angle;
    int x = 0, y = 0, z = 0;
    lv_obj_t *btn, *lab;
    uint32_t temp = 0;
    int temp1 = 0, temp2 = 0, ret = 0;
    char tbuf[32];
    uint8_t tbuf2[16] = {0};
    uint8_t gps_buf[128];
    struct lp_gps_info_t gpsinfo;

    test_buf[0] = 0;

    // if (0 == lpbsp_wifi_event(E_WIFI_CONNECT))
    // {
    //     lpbsp_wifi_connect(TEST_MODE_WIFI_SSID, TEST_MODE_WIFI_PASSWORD);
    //     strcat(test_buf, "wifi no connet\n");
    // }
    // sprintf(tbuf, "%d,%d:%d\n", lpbsp_wifi_event(E_WIFI_CONNECT), lpbsp_charge_get(), lpbsp_bat_get_mv());
    // strcat(test_buf, tbuf);

    temp = lpbsp_sensor_magnetic_id();
    lpbsp_sensor_magnetic_xyz_read(&x, &y, &z);
    temp2 = lpbsp_sensor_accelerometer_id();
    ret = lp_sensor_accelerometer_step_read();

    angle = atan2((double)y, (double)x) * (180 / 3.14159265) + 180;
    sprintf(tbuf, "%lx/%d,%d,%d,%d\n%lx/%d\n", temp, angle, x, y, z, temp2, ret);
    strcat(test_buf, tbuf);

    lpbsp_sensor_nfc_id(tbuf2);
    sprintf(tbuf, "NFC:%X %X %X %X %X %X %X %X\n", tbuf2[0], tbuf2[1], tbuf2[2], tbuf2[3], tbuf2[4], tbuf2[5], tbuf2[6], tbuf2[7]);
    strcat(test_buf, tbuf);

    lp_fs_volume("/C", &temp1, &temp2);
    sprintf(tbuf, "sd:%d/%dMb\n", temp2 / (1048576), temp1 / (1048576));
    strcat(test_buf, tbuf);

    ret = lp_gps_read(&gpsinfo);
    if (ret)
    {
        sprintf(tbuf, "[%f,%f]\n", gpsinfo.latitude, gpsinfo.longitude);
        strcat(test_buf, tbuf);
    }
    if (0 == test_buf[0]) // 测试成功了
    {
        lv_label_set_text(lab_log_obj, "test ok");

        lpbsp_free(test_buf);
        lv_timer_del(timer);
    }
    else
    {
        lv_label_set_text(lab_log_obj, test_buf);
    }
}
/*
测试模式
*/
int view_page_test(lv_obj_t *appup_bg_obj)
{
    lv_obj_t *lab_obj;

    lpbsp_moto_set(1);

    lvgl_sleep_time_ms(0xfffffff);

    lab_log_obj = lv_label_create(appup_bg_obj); //----标题----//
    lv_obj_set_pos(lab_log_obj, 0, 0);
    lv_obj_set_size(lab_log_obj, 240, 235);
    lv_label_set_text(lab_log_obj, "start test");
    lv_obj_set_style_text_font(lab_log_obj, &lv_font_montserrat_22, 0);

    timer = lv_timer_create(timer_cb, 1000, NULL);

    test_buf = lpbsp_malloc(512);

    if (0 == test_buf)
    {
        return 1;
    }

    lv_obj_add_event_cb(appup_bg_obj, view_page_close_cb, LV_EVENT_DELETE, 0);
    lpbsp_delayms(50);
    lpbsp_moto_set(0);
    return 0;
}
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////
static lv_obj_t *obj_up_porg = 0;
static lv_obj_t *obj_lab_up_url = 0;

static void main_page_btn_up_callback(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_SHORT_CLICKED)
    {
        lp_lvgl_down2file(up_site_url, "/C/Sys/app.zip");
    }
}

static void main_page_btn_back_callback(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_SHORT_CLICKED)
    {
        lp_lvgl_app_rollback();
    }
}

void up_page_up_url_cb(void *arg, char *input_ole, char *input_new)
{
    char addip[20];

    strcpy(up_site_url, input_new);
    if (lpbsp_wifi_event(1))
    {
        lpbsp_wifi_addr_ip(addip);
        lv_label_set_text_fmt(obj_lab_up_url, "ip:%s,%s", addip, input_new);
    }
    else
    {
        lv_label_set_text(obj_lab_up_url, "not net");
    }
}
static void up_page_btn_up_sitting_callback(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_SHORT_CLICKED)
    {
        lp_lvgl_input_en(0, up_site_url, up_page_up_url_cb);
    }
}

// 升级页面关闭
static void up_view_page_close_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (LV_EVENT_DELETE == code)
    {
        lvgl_sleep_time_ms(15000);
    }
}

static void up_page_btn_up_callback(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_SHORT_CLICKED)
    {
        lp_lvgl_app_up();
    }
}
int view_page_up(lv_obj_t *page)
{
    lv_obj_t *btn_obj, *lab_obj;
    char addip[20];
    lvgl_sleep_time_ms(0xfffffff);

    sprintf(up_site_url, "http://192.168.137.1:5000/app.zip");
    obj_up = page;

    lv_obj_add_event_cb(page, up_view_page_close_cb, LV_EVENT_DELETE, 0);

    obj_up_porg = lv_bar_create(page);
    lv_obj_set_pos(obj_up_porg, 20, 0);
    lv_obj_set_size(obj_up_porg, 200, 5);

    obj_lab_up_url = lv_label_create(page);
    lv_obj_set_pos(obj_lab_up_url, 0, 5);
    lv_obj_set_size(obj_lab_up_url, 240, 70);
    lv_obj_set_style_text_font(obj_lab_up_url, &lv_font_montserrat_22, 0);

    if (lpbsp_wifi_event(1))
    {
        lpbsp_wifi_addr_ip(addip);
        lv_label_set_text_fmt(obj_lab_up_url, "ip:%s,%s", addip, up_site_url);
    }
    else
    {
        lv_label_set_text(obj_lab_up_url, "not net");
    }

    btn_obj = lv_btn_create(page);
    lv_obj_set_pos(btn_obj, 10, 75);
    lv_obj_set_size(btn_obj, 220, 70);
    lab_obj = lv_label_create(btn_obj);
    lv_label_set_text(lab_obj, "connect up");
    lv_obj_set_align(lab_obj, LV_ALIGN_CENTER);
    lv_obj_add_event_cb(btn_obj, main_page_btn_up_callback, LV_EVENT_SHORT_CLICKED, NULL);

    btn_obj = lv_btn_create(page);
    lv_obj_set_pos(btn_obj, 10, 150);
    lv_obj_set_size(btn_obj, 220, 70);
    lab_obj = lv_label_create(btn_obj);
    lv_label_set_text(lab_obj, "Up url edit");
    lv_obj_set_align(lab_obj, LV_ALIGN_CENTER);
    lv_obj_add_event_cb(btn_obj, up_page_btn_up_sitting_callback, LV_EVENT_SHORT_CLICKED, NULL);

    btn_obj = lv_btn_create(page);
    lv_obj_set_pos(btn_obj, 10, 225);
    lv_obj_set_size(btn_obj, 220, 70);
    lab_obj = lv_label_create(btn_obj);
    lv_label_set_text(lab_obj, "App up");
    lv_obj_set_align(lab_obj, LV_ALIGN_CENTER);
    lv_obj_add_event_cb(btn_obj, up_page_btn_up_callback, LV_EVENT_SHORT_CLICKED, NULL);

    btn_obj = lv_btn_create(page);
    lv_obj_set_pos(btn_obj, 10, 300);
    lv_obj_set_size(btn_obj, 220, 70);
    lab_obj = lv_label_create(btn_obj);
    lv_label_set_text(lab_obj, "back up");
    lv_obj_set_align(lab_obj, LV_ALIGN_CENTER);
    lv_obj_add_event_cb(btn_obj, main_page_btn_back_callback, LV_EVENT_SHORT_CLICKED, NULL);

    return 0;
}
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////