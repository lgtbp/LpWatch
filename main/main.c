#include "lvgl_inc.h"

// _START
const char *MAIN_TEXT[] = {

    "Setting",
    "File",
    "Map",
    "Write Sn",
};
// _END

static const lp_lvgl_page_cb lv_page_cb_arr[] = // 界面或事件处理
    {
        view_page_setting,
        view_page_file_explorer,
        view_page_map,
};

static const lpbsp_main_api_t lpbsp_main_api = {
    .num = ARR_SIZE(lv_page_cb_arr),
    .name = MAIN_TEXT,
    .api = lv_page_cb_arr,
};
static void lv_tick_task(void *arg)
{
    (void)arg;

    lv_tick_inc(1);
}
static int touch_time_ms = 15000;

/// @brief 设置休眠时间
/// @param ms
/// @return
int lvgl_sleep_time_ms(int ms)
{
    touch_time_ms = ms;
    return 0;
}

int init_page_task(void *pvParameter)
{
    int temp = 0;

    lpbsp_iic_init(0, 1);
    lp_lvgl_init(240 * 140);

    lpbsp_time_create(&lv_tick_task);

    lp_lvgl_page_view_clock_set(view_page_clock_1);
    lp_lvgl_page_view_clock_app_set(view_page_main_clock);
    lp_lvgl_page_view_main_api_set(&lpbsp_main_api);

    view_page_main_clock(lv_scr_act());

    lv_task_handler();
    APP_LOGD("[%d]%s %s free %d,min=%d\r\n", lpbsp_tick_ms(), __DATE__, __TIME__, lpbsp_ram_free_get(), lpbsp_ram_min_get());
    {
        lpbsp_bl_set(10);
        lpbsp_sensor_init();
        // lpbsp_wifi_init(1);
        lp_fs_init();
    }

    lpbsp_wdt_add();
    lplib_rtc_ram_init();
    lptls_app_crt_chain_init();
    while (1)
    {
        lpbsp_wdt_feed();
        if (touch_time_ms < lv_disp_get_inactive_time(0))
        {
            if (5 > lp_bat_value_get())
                lpbsp_sleep_config(1, 0);

            temp = lplib_power_mode_get();
            if (1 == temp)
            {
                lpbsp_sleep_config(2, 60 - lpbsp_rtc_get() % 60);
                lv_disp_trig_activity(0);
                continue;
            }
            if (0xffffff < touch_time_ms)
            {
                lv_disp_trig_activity(0);
                continue;
            }
            lpbsp_sleep_config(1, 0);
        }
        lp_lvgl_ui_notify_run();
        lv_task_handler();
        lpbsp_delayms(5);
    }
    return 0;
}

// lvgl main ui
void lvgl_app_main(void)
{
    // 这配置 栈最小是4428
    lpbsp_task_create("gui", 0, (task_function_t)init_page_task, 1024 * 16, 3);
}

///////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////

int app_main(void)
{
    lpbsp_init_all();
    lvgl_app_main();
    return 0;
}
