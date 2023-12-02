
#include "lplib.h"

#include "minmea.h"
#ifdef CONFIG_LP_USE_MINIZ
#include "./miniz_300.h"
#endif
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#if 0
#define LP_LOGD(format, ...) printf("%04d,%16s:%04d " format, lpbsp_tick_ms(), __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define LP_LOGD(...)
#endif

#if 0
#define LP_LOGW(format, ...) printf("%04d,%16s:%04d " format, lpbsp_tick_ms(), __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define LP_LOGW(...)
#endif

#if 0
#define LP_LOGE(format, ...) printf("%04d,%16s:%04d " format, lpbsp_tick_ms(), __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define LP_LOGE(...)
#endif
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#define LP_LVGL_SYSBAR_BACK_X 18  // 返回 或 用户界面提醒
#define LP_LVGL_SYSBAR_ICON_X 65  // 图标 wifi 与 gps
#define LP_LVGL_SYSBAR_BAT_X 139  // 电池图标
#define LP_LVGL_SYSBAR_TIME_X 160 // 时间

#define LP_LVGL_SYSBAR_BAT 0  // 电池图标
#define LP_LVGL_SYSBAR_TIME 1 // 时间
#define LP_LVGL_SYSBAR_ICON 2 // 图标 wifi 与 gps
#define LP_LVGL_SYSBAR_BACK 3 // 返回 或 用户界面提醒
typedef struct
{
    lv_obj_t obj;               // lv的obj
    lv_obj_t *tile_act;         // 当前可视的page
    uint8_t last_page_id;       // 上一个page的id,用于后退判断
    lp_lvgl_page_cb_t close_cb; // 滑动结束本的事件回调
} lp_lvgl_page_t;
typedef struct
{
    lv_obj_t obj;                      // lv的obj
    lv_dir_t dir;                      // 它的方向
    lv_obj_t *page_tv;                 // 它的父亲tv
    lv_obj_t *sysbar[4];               // 这是sys bar上的控件
    lv_obj_t *page;                    // 这才是真的page
    lp_lvgl_page_view_cb_t refresh_cb; // 这是当前界面的初始化函数
} lp_lvgl_page_tile_t;
static uint8_t bar_ico_flg = 0; // ico 显示标记
/*
0: 1 有gps
1: 1 gps定位成功了
2: 1 有tips消息
*/

static lp_lvgl_page_view_cb_t lv_page_view_clock;     // 初始化上拉时间界面的用于预览回调
static lp_lvgl_page_view_cb_t lv_page_view_main;      // 初始化app列表用于预览的回调
static lp_lvgl_page_view_cb_t lv_page_view_clock_app; // 初始化上拉时钟界面【列表】的回调
static lp_lvgl_page_view_cb_t lv_page_view_main_app;  // 初始化app列表【列表】的回调
static uint8_t sys_bar_flg = 1;                       // 是否显示系统bar
static lp_fifo_t lp_fifo = {0};                       // msg tip fifo
void *lp_queue_ui_notify;                             //
static lp_lvgl_page_t *lp_page_app;                   // 当前焦点的app
static lpbsp_main_api_t *lpbsp_main_api;

static void lp_lvgl_page_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void lp_lvgl_page_tile_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj);
static void page_event_cb(lv_event_t *e);
static int lp_lvgl_page_view_top_show(lv_obj_t *src_obj);
static void top_ico_btn_event_handler(lv_event_t *e);
static int view_page_main_showup(lv_obj_t *main_cont_obj);
static int view_page_main_app(lv_obj_t *obj);
static void lp_lvgl_fs_init(void);

const lv_obj_class_t lp_page_class = {.constructor_cb = lp_lvgl_page_constructor,
                                      .base_class = &lv_obj_class,
                                      .instance_size = sizeof(lp_lvgl_page_t)};

const lv_obj_class_t lp_page_tile_class = {.constructor_cb = lp_lvgl_page_tile_constructor,
                                           .base_class = &lv_obj_class,
                                           .instance_size = sizeof(lp_lvgl_page_tile_t)};

static lv_dir_t create_dir;
static uint8_t create_col_id;
static uint8_t create_row_id;
//////////////////////////////////////////////////

/// @brief 系统栏上显示
/// @param lab_bat
/// @param lab_time
/// @param lab_ico
/// @return
static int lp_lvgl_sysbar_value_set(lv_obj_t *lab_bat,  // bat 电量显示
                                    lv_obj_t *lab_time, // lab_time 显示
                                    lv_obj_t *lab_ico)
{
    datetime_t dt_now;
    uint32_t temp1 = 0, temp2 = 0;
    lv_obj_t *lab_bat_bar; // bat bar 电量显示
    char tbuf[32] = {0};

    if (lab_time)
    {
        temp1 = lpbsp_wifi_event(1);
        temp2 = bar_ico_flg;
        if (temp1 && temp2)
        {
            if (bar_ico_flg & 2)
                sprintf(tbuf, "%s%02x%s", LV_SYMBOL_WIFI "#00", lp_gps_read(0) * 4, "00 " LV_SYMBOL_GPS "#");
            else
                sprintf(tbuf, "%s", LV_SYMBOL_WIFI LV_SYMBOL_GPS);
        }
        else if (temp1)
            sprintf(tbuf, "%s", LV_SYMBOL_WIFI);
        else if (temp2)
        {
            if (bar_ico_flg & 2)
                sprintf(tbuf, "%s%02x%s", " #00", lp_gps_read(0) * 4, "00 " LV_SYMBOL_GPS "#");
            else
                sprintf(tbuf, "%s", " " LV_SYMBOL_GPS);
        }
        lp_lvgl_label_set_text(lab_ico, tbuf);

        lp_time_get(&dt_now);
        lp_lvgl_label_set_text_fmt(lab_time, "%02d:%02d", dt_now.tm_hour, dt_now.tm_min);

        lab_bat_bar = lab_bat->user_data;
        temp1 = lp_bat_value_get();
        if (lab_bat_bar)
            lv_bar_set_value(lab_bat_bar, temp1 % 100, 0);

        temp1 = temp1 / 10;
        if (lpbsp_charge_get())
            lp_lvgl_label_set_text_fmt(lab_bat, "#00ff00 %d#", temp1);
        else
        {
            if (1 > temp1)
            {
                lp_lvgl_label_set_text_fmt(lab_bat, "#ff0000 %d#", temp1);
            }
            else
            {
                lp_lvgl_label_set_text_fmt(lab_bat, "%d", temp1);
            }
        }

        return 0;
    }

    return 1;
}

/// @brief 这是时间回调
/// @param timer
static void lv_sysbar_timer_cb(lv_timer_t *timer)
{
    (void)timer;

    lp_lvgl_page_tile_t *page = (lp_lvgl_page_tile_t *)lp_page_app->tile_act;

    if (lp_gps_read(0))
        bar_ico_flg |= 0x2;
    else
        bar_ico_flg &= 0xfd;

    lp_lvgl_sysbar_value_set(
        page->sysbar[LP_LVGL_SYSBAR_BAT],
        page->sysbar[LP_LVGL_SYSBAR_TIME],
        page->sysbar[LP_LVGL_SYSBAR_ICON]);
}

/// @brief 这是滑动中发生的时间回调
/// @param evetn
/// @param value
static void app_close_cb_event(int32_t evetn, uint32_t value)
{
    LP_LOGD("app_close_cb_test %d %x\r\n", evetn, value);

    if (1 == evetn) // 被上拉了
    {
        lv_obj_del(lv_obj_get_parent((lv_obj_t *)value)); // 删除了
        lv_page_view_clock_app(lv_scr_act());
    }
    else if (-1 == evetn) // 这是后退活动了
    {
        lv_obj_t *obj = lv_obj_get_parent((lv_obj_t *)value);

        lv_obj_del(lv_obj_get_child(obj, lv_obj_get_child_cnt(obj) - 1));
    }
}
/// @brief 这是滑动中发生的时间回调
/// @param evetn
/// @param value
static void clock_close_cb_event(int32_t evetn, uint32_t value)
{
    // LP_LOGD("clock_close_cb_event %d %x\r\n", evetn, value);

    if (1 == evetn) // 被上拉了
    {
        lv_obj_del(lv_obj_get_parent((lv_obj_t *)value)); // 删除了
        lv_page_view_main_app(lv_scr_act());
    }
}
/// @brief 系统状态栏初始化
/// @param scr 当前界面
/// @return
static int lp_lvgl_page_title_init(lv_obj_t *page_obj, lv_obj_t *scr)
{
    lv_obj_t *obj_bat_bar;
    lp_lvgl_page_tile_t *page = (lp_lvgl_page_tile_t *)page_obj;

    page->sysbar[LP_LVGL_SYSBAR_BACK] = lv_label_create(scr);
    lv_obj_remove_style_all(page->sysbar[LP_LVGL_SYSBAR_BACK]);
    lv_obj_set_pos(page->sysbar[LP_LVGL_SYSBAR_BACK], LP_LVGL_SYSBAR_BACK_X, 0);

    page->sysbar[LP_LVGL_SYSBAR_ICON] = lv_label_create(scr);
    lv_obj_remove_style_all(page->sysbar[LP_LVGL_SYSBAR_ICON]);
    lv_obj_set_pos(page->sysbar[LP_LVGL_SYSBAR_ICON], LP_LVGL_SYSBAR_ICON_X, 0);
    lv_label_set_recolor(page->sysbar[LP_LVGL_SYSBAR_ICON], true); // 带颜色的

    obj_bat_bar = lv_bar_create(scr);
    lv_obj_set_pos(obj_bat_bar, LP_LVGL_SYSBAR_BAT_X, 0);
    lv_obj_set_size(obj_bat_bar, 20, 25);
    lv_obj_set_style_radius(obj_bat_bar, 4, LV_PART_MAIN);
    lv_obj_set_style_radius(obj_bat_bar, 4, LV_PART_INDICATOR);

    page->sysbar[LP_LVGL_SYSBAR_BAT] = lv_label_create(scr);
    lv_obj_remove_style_all(page->sysbar[LP_LVGL_SYSBAR_BAT]);
    lv_obj_set_pos(page->sysbar[LP_LVGL_SYSBAR_BAT], LP_LVGL_SYSBAR_BAT_X, 0);
    lv_label_set_recolor(page->sysbar[LP_LVGL_SYSBAR_BAT], true); // 带颜色的
    lv_obj_set_size(page->sysbar[LP_LVGL_SYSBAR_BAT], 20, 25);
    lv_obj_set_style_text_align(page->sysbar[LP_LVGL_SYSBAR_BAT], LV_TEXT_ALIGN_CENTER, 0);
    page->sysbar[LP_LVGL_SYSBAR_BAT]->user_data = obj_bat_bar;

    page->sysbar[LP_LVGL_SYSBAR_TIME] = lv_label_create(scr);
    lv_obj_remove_style_all(page->sysbar[LP_LVGL_SYSBAR_TIME]);
    lv_obj_set_pos(page->sysbar[LP_LVGL_SYSBAR_TIME], LP_LVGL_SYSBAR_TIME_X, 0);

    lp_lvgl_label_set_text(page->sysbar[LP_LVGL_SYSBAR_BACK], "<");

    lv_obj_set_style_text_font(page->sysbar[LP_LVGL_SYSBAR_BACK], &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_font(page->sysbar[LP_LVGL_SYSBAR_TIME], &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_font(page->sysbar[LP_LVGL_SYSBAR_ICON], &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_font(page->sysbar[LP_LVGL_SYSBAR_BAT], &lv_font_montserrat_22, 0);

    lp_lvgl_sysbar_value_set(
        page->sysbar[LP_LVGL_SYSBAR_BAT],
        page->sysbar[LP_LVGL_SYSBAR_TIME],
        page->sysbar[LP_LVGL_SYSBAR_ICON]);

    return 0;
}
static int lp_lvgl_page_title_for_main_init(lv_obj_t *scr, int y)
{
    lv_obj_t *lab_time, *lab_bat, *lab_icon;
    lv_obj_t *obj_bar;

    lab_icon = lv_label_create(scr);
    lv_obj_remove_style_all(lab_icon);
    lv_obj_set_pos(lab_icon, LP_LVGL_SYSBAR_ICON_X, y);
    lv_label_set_recolor(lab_icon, true); // 带颜色的

    obj_bar = lv_bar_create(scr);
    lv_obj_set_pos(obj_bar, LP_LVGL_SYSBAR_BAT_X, y);
    lv_obj_set_size(obj_bar, 20, 25);
    lv_obj_set_style_radius(obj_bar, 4, LV_PART_MAIN);
    lv_obj_set_style_radius(obj_bar, 4, LV_PART_INDICATOR);

    lab_bat = lv_label_create(scr);
    lv_obj_remove_style_all(lab_bat);
    lv_obj_set_size(lab_bat, 20, 25);
    lv_obj_set_pos(lab_bat, LP_LVGL_SYSBAR_BAT_X, y);
    lv_label_set_recolor(lab_bat, true); // 带颜色的
    lv_obj_set_style_text_align(lab_bat, LV_TEXT_ALIGN_CENTER, 0);
    lab_bat->user_data = obj_bar;

    lab_time = lv_label_create(scr);
    lv_obj_remove_style_all(lab_time);
    lv_obj_set_pos(lab_time, LP_LVGL_SYSBAR_TIME_X, y);

    lv_obj_set_style_text_font(lab_time, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_font(lab_bat, &lv_font_montserrat_22, 0);
    lv_obj_set_style_text_font(lab_icon, &lv_font_montserrat_22, 0);

    lp_lvgl_sysbar_value_set(lab_bat, lab_time, lab_icon);

    return 0;
}

static void lp_lvgl_page_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{
    LV_UNUSED(class_p);
    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
    lv_obj_add_event_cb(obj, page_event_cb, LV_EVENT_SCROLL_END, NULL);
    lv_obj_add_flag(obj, LV_OBJ_FLAG_SCROLL_ONE);
    lv_obj_set_scroll_snap_x(obj, LV_SCROLL_SNAP_CENTER);
    lv_obj_set_scroll_snap_y(obj, LV_SCROLL_SNAP_CENTER);
}

static void lp_lvgl_page_tile_constructor(const lv_obj_class_t *class_p, lv_obj_t *obj)
{

    LV_UNUSED(class_p);
    lv_obj_t *parent = lv_obj_get_parent(obj);
    lv_obj_set_size(obj, LV_PCT(100), LV_PCT(100));
    lv_obj_update_layout(obj); /*Be sure the size is correct*/
    lv_obj_set_pos(obj, create_col_id * lv_obj_get_content_width(parent),
                   create_row_id * lv_obj_get_content_height(parent));

    lp_lvgl_page_tile_t *tile = (lp_lvgl_page_tile_t *)obj;
    tile->dir = create_dir;

    if (create_col_id == 0 && create_row_id == 0)
    {
        lv_obj_set_scroll_dir(parent, create_dir);
    }
}

static void page_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    lp_lvgl_page_t *tv = (lp_lvgl_page_t *)obj;
    static int8_t event_flg = -1;
    uint8_t child_cnt = lv_obj_get_child_cnt(obj);

    if (code == LV_EVENT_SCROLL_END)
    {
        lv_indev_t *indev = lv_indev_get_act();
        if (indev && indev->proc.state == LV_INDEV_STATE_PRESSED)
        {
            return;
        }

        // 跑到这里说明，这个界面已经确认显示了
        // 但会有2次，第一次是进来然后发指令让它到位，第二次是绘制结束了

        lv_coord_t w = lv_obj_get_content_width(obj);
        lv_coord_t h = lv_obj_get_content_height(obj);

        lv_point_t scroll_end;
        lv_obj_get_scroll_end(obj, &scroll_end);
        lv_coord_t left = scroll_end.x;
        lv_coord_t top = scroll_end.y;

        lv_coord_t tx = ((left + (w / 2)) / w) * w;
        lv_coord_t ty = ((top + (h / 2)) / h) * h;

        lv_dir_t dir = LV_DIR_ALL;
        uint32_t i;
        for (i = 0; i < child_cnt; i++)
        {
            lv_obj_t *tile_obj = lv_obj_get_child(obj, i);
            lv_coord_t x = lv_obj_get_x(tile_obj);
            lv_coord_t y = lv_obj_get_y(tile_obj);

            if (x == tx && y == ty)
            {
                lp_lvgl_page_tile_t *tile = (lp_lvgl_page_tile_t *)tile_obj;
                tv->tile_act = (lv_obj_t *)tile;
                dir = tile->dir;

                lp_page_app = tv; // 设置当前app
                if (tv->close_cb) // 绘制结束了--需要执行啥的，在这里
                {
                    if (event_flg == i)
                    {
                        event_flg = -1;
                        tv->close_cb(i, (uint32_t)tile); // 滑动结束了
                        return;
                    }
                    event_flg = i;
                }

                // LP_LOGD("DDDD obj addr %x, %x,%x,[%x],%x\r\n", event_flg, (uint32_t)lv_obj_get_child(obj, 0), i, (uint32_t)tv, tv->type);
                if (i > 1) // 0 1 是top和bottom
                {
                    lv_obj_set_x(lv_obj_get_child(obj, 0), i * 240 - 480);
                    lv_obj_set_x(lv_obj_get_child(obj, 1), i * 240 - 480);
                    // LP_LOGD("last_page_id %d,i %d\r\n", tv->last_page_id, i);
                    if (tv->last_page_id == (i + 1)) // 删除后面那个page
                    {
                        if (tv->close_cb)
                        {
                            tv->close_cb(-1, (uint32_t)tile); // 滑动结束了  lv_obj_del(lv_obj_get_child(obj, child_cnt - 1));
                        }
                    }
                }

                tv->last_page_id = i;

                lv_event_send(obj, LV_EVENT_VALUE_CHANGED, NULL);
                break;
            }
        }
        lv_obj_set_scroll_dir(obj, dir);
    }
}

/////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////

/// @brief 主时间界面-用于在非clock界面下-上拉的显示
/// @param cb
void lp_lvgl_page_view_clock_set(lp_lvgl_page_view_cb_t cb)
{
    lv_page_view_clock = cb;
}
/// @brief 主界面app列表-用于在clock界面下-上拉的显示
/// @param cb
void lp_lvgl_page_view_main_set(lp_lvgl_page_view_cb_t cb)
{
    lv_page_view_main = cb;
}
/// @brief 主时间界面-用于在非clock界面下-上拉的初始化
/// @param cb
void lp_lvgl_page_view_clock_app_set(lp_lvgl_page_view_cb_t cb)
{
    lv_page_view_clock_app = cb;
}
/// @brief 主界面app列表-用于在clock界面下-上拉的初始化
/// @param cb
void lp_lvgl_page_view_main_app_set(lp_lvgl_page_view_cb_t cb)
{
    lv_page_view_main_app = cb;
}

/// @brief 主界面app列表-设置页面的回调
/// @param cb
void lp_lvgl_page_view_main_api_set(lpbsp_main_api_t *api)
{
    lpbsp_main_api = api;
    lp_lvgl_page_view_main_set(view_page_main_showup);
    lp_lvgl_page_view_main_app_set(view_page_main_app);
}
/// @brief 设置系统的bar是否出现
/// @param eanble 1出现 0不出现
void lp_lvgl_page_sysbar_set(uint32_t eanble)
{
    sys_bar_flg = eanble;
}
/// @brief 新建一个page
/// @param parent 它的父亲
/// @return page的obj
lv_obj_t *lp_lvgl_page_create(lv_obj_t *parent)
{
    lv_obj_t *obj = lv_obj_class_create_obj(&lp_page_class, parent);
    lv_obj_class_init_obj(obj);

    // LP_LOGD("lp_lvgl_page_create addr [%x]\r\n", (uint32_t)obj);

    static int init = 0;
    if (0 == init)
    {
        init = 1;
        lv_timer_create(lv_sysbar_timer_cb, 1000, NULL);
    }
    return obj;
}

/// @brief 设置obj被关闭的回调
/// @param tv obj
/// @param cb 关闭的回调函数
void lp_lvgl_page_close_cb_set(lv_obj_t *tv, lp_lvgl_page_cb_t cb)
{
    lp_lvgl_page_t *tv_obj = (lp_lvgl_page_t *)tv;

    tv_obj->close_cb = cb;
    // LP_LOGD("app page   %d\r\n", (uint32_t)tv_obj->close_cb);
}

/// @brief 用户app有权利去改它的显示
/// @param data 显示字符
/// @return 0ok 1error
int lp_lvgl_page_sysmenu_text_set(char *data)
{
    lp_lvgl_page_tile_t *page = (lp_lvgl_page_tile_t *)lp_page_app->tile_act;
    char tbuf[5] = {0};

    if (page->sysbar[LP_LVGL_SYSBAR_BACK])
    {
        memcpy(tbuf, data, 4);
        lp_lvgl_label_set_text(page->sysbar[LP_LVGL_SYSBAR_BACK], tbuf);
        return 0;
    }
    return 1;
}

/// @brief 为app创建一个新的page
/// @param src page下面的子obj
/// @param type 1带状态栏 0不带状态栏的
/// @return 返回用户可操作区域obj
lv_obj_t *lp_lvgl_page_app_create_title(lv_obj_t *src, int type)
{
    lv_obj_t *obj_page = lv_obj_create(src);
    lv_obj_remove_style_all(obj_page);

#if 0
    lv_obj_t *temp_obj = lv_obj_create(src); //下面用黑线伪装，移动的时候会有一条黑线
    lv_obj_remove_style_all(temp_obj);
    lv_obj_set_pos(temp_obj, 0, 275);
    lv_obj_set_size(temp_obj, 240, 5);
    lv_obj_set_style_bg_opa(temp_obj, LV_OPA_COVER, 0);
    lv_obj_set_style_bg_color(temp_obj, lv_color_make(0, 0, 0), 0); // 全黑背景
#endif
    LP_LOGD("lf page app_create_title %d\r\n", type);
    if (0 == type) // 0不带状态栏的
    {
        lv_obj_set_pos(obj_page, 0, 5);
        lv_obj_set_size(obj_page, 240, 270); // 用户可视
    }
    else
    {
        lv_obj_t *obj_title = lv_obj_create(src);
        lv_obj_remove_style_all(obj_title);
        lv_obj_set_pos(obj_title, 0, 0);
        lv_obj_set_size(obj_title, 240, 25);
        lp_lvgl_page_title_init(src, obj_title); // 总的title

        lv_obj_set_pos(obj_page, 0, 25);
        lv_obj_set_size(obj_page, 240, 250); // 用户可视
    }

#if 0
    lv_obj_clear_flag(obj_page, LV_OBJ_FLAG_SCROLL_CHAIN_VER); // 不允许将滚动传播到父级
#else
    lv_obj_add_flag(obj_page, LV_OBJ_FLAG_SCROLL_CHAIN_VER); //  允许将滚动传播到父级
#endif

    return obj_page;
}

/// @brief 新建一个page
/// @param parent 它的父亲
/// @param type  1 特殊的界面不带状态栏的 0普通的app page带状态栏的
/// @return page的obj
lv_obj_t *lp_lvgl_page_app_create(lv_obj_t *parent, uint8_t type)
{
    lv_obj_t *tv;
    tv = lp_lvgl_page_create(parent);
    lp_lvgl_page_close_cb_set(tv, app_close_cb_event);

    lv_obj_t *tile0 = lp_lvgl_page_add_tile(tv, 0, 0, LV_DIR_BOTTOM);
    lv_obj_t *tile1 = lp_lvgl_page_add_tile(tv, 0, 2, LV_DIR_TOP);
    lv_obj_t *tile2 = lp_lvgl_page_add_tile(tv, 0, 1, LV_DIR_TOP | LV_DIR_LEFT | LV_DIR_RIGHT | LV_DIR_BOTTOM);

    lp_lvgl_page_view_top_show(tile0);
    if (lv_page_view_clock)
        lv_page_view_clock(tile1);

    lp_lvgl_page_set_tile_id(tv, 0, 1, 0);

    lv_obj_t *obj_page = lp_lvgl_page_app_create_title(tile2, type);

    return obj_page;
}
/// @brief 新建一个page-默认参数
/// @param parent 它的父亲
/// @return
lv_obj_t *lp_lvgl_page_app_create_def(lv_obj_t *parent)
{
    return lp_lvgl_page_app_create(parent, sys_bar_flg);
}

/// @brief 新建一个page
/// @param parent 它的父亲
/// @param type  多小个时钟
/// @return page的obj
lv_obj_t *lp_lvgl_page_clock_create(lv_obj_t *parent, uint8_t count)
{
    int i = 0;
    lv_obj_t *clock_tv = lp_lvgl_page_create(parent);

    lp_lvgl_page_close_cb_set(clock_tv, clock_close_cb_event);

    lv_obj_t *tile0 = lp_lvgl_page_add_tile(clock_tv, 0, 0, LV_DIR_BOTTOM);
    lv_obj_t *tile1 = lp_lvgl_page_add_tile(clock_tv, 0, 2, LV_DIR_TOP);

    lp_lvgl_page_view_top_show(tile0);
    if (lv_page_view_main)
        lv_page_view_main(tile1);
    for (i = 0; i < count; i++)
    {
        lp_lvgl_page_add_tile(clock_tv, i, 1, LV_DIR_TOP | LV_DIR_LEFT | LV_DIR_RIGHT | LV_DIR_BOTTOM);
    }
    lp_lvgl_page_set_tile_id(clock_tv, 0, 1, 0);
    return clock_tv;
}

/// @brief 获取指定的clock界面
/// @param parent
/// @param count 指定num
/// @return 返回obj
lv_obj_t *lp_lvgl_page_clock_get(lv_obj_t *parent, uint8_t count)
{
    if (lv_obj_get_child_cnt(parent) > (uint32_t)(count + 2))
        return lv_obj_get_child(parent, 2 + count);

    return 0;
}
/// @brief 新建一个page的title
/// @param tv obj
/// @param col_id 列
/// @param row_id 行
/// @param dir 能获得的方向
/// @return 新建的obj
lv_obj_t *lp_lvgl_page_add_tile(lv_obj_t *tv, uint8_t col_id, uint8_t row_id, lv_dir_t dir)
{
    lp_lvgl_page_t *tv_obj = (lp_lvgl_page_t *)tv;
    lp_lvgl_page_tile_t *lp_page_tile;
    create_dir = dir;
    create_col_id = col_id;
    create_row_id = row_id;

    lv_obj_t *obj = lv_obj_class_create_obj(&lp_page_tile_class, tv);
    if (0 == obj)
    {
        LP_LOGD("creat obj err\r\n");
        return 0;
    }
    lv_obj_class_init_obj(obj);

    lp_page_tile = (lp_lvgl_page_tile_t *)obj;
    lp_page_tile->page_tv = tv;

    return obj;
}

/// @brief 添加下一个界面
/// @param tv 首页?
/// @return next obj
lv_obj_t *lp_lvgl_page_add_next(lv_obj_t *tv)
{
    lp_lvgl_page_t *page = lp_page_app; // lp_lvgl_page_father_get(lv_obj_get_parent(tv));
    lv_obj_t *scr = 0;
    int c_id = 0;

    c_id = lv_obj_get_child_cnt((lv_obj_t *)page) - 2; // top bottom
    scr = lp_lvgl_page_add_tile((lv_obj_t *)page, c_id, 1, LV_DIR_TOP | LV_DIR_LEFT | LV_DIR_RIGHT | LV_DIR_BOTTOM);
    lp_lvgl_page_set_tile_id(lp_lvgl_page_father_get(scr), c_id, 1, 1);

    return lp_lvgl_page_app_create_title(scr, 1);
}

/// @brief 添加下一个界面-带初始化函数
/// @param cb 页面初始化
/// @param bar_eanble 是否显示bar
/// @return next obj
lv_obj_t *lp_lvgl_page_add_next_with_init(lp_lvgl_page_view_cb_t cb, int bar_eanble)
{
    lp_lvgl_page_t *page = lp_page_app; // lp_lvgl_page_father_get(lv_obj_get_parent(tv));
    lv_obj_t *scr = 0, *out_obj = 0;
    int c_id = 0;
    lp_lvgl_page_tile_t *lp_page_tile;

    c_id = lv_obj_get_child_cnt((lv_obj_t *)page) - 2; // top bottom
    scr = lp_lvgl_page_add_tile((lv_obj_t *)page, c_id, 1, LV_DIR_TOP | LV_DIR_LEFT | LV_DIR_RIGHT | LV_DIR_BOTTOM);
    lp_lvgl_page_set_tile_id(lp_lvgl_page_father_get(scr), c_id, 1, 1);

    out_obj = lp_lvgl_page_app_create_title(scr, bar_eanble);
    if (cb)
    {
        lp_page_tile = (lp_lvgl_page_tile_t *)scr;
        lp_page_tile->refresh_cb = cb;
        cb(out_obj);
    }

    return out_obj;
}
/// @brief 添加下一个界面-没有系统条的
/// @param tv 首页?
/// @return next obj
lv_obj_t *lp_lvgl_page_add_next_no_sysbar(lv_obj_t *tv)
{
    lp_lvgl_page_t *page = lp_page_app; // lp_lvgl_page_father_get(lv_obj_get_parent(tv));
    lv_obj_t *scr = 0;
    int c_id = 0;

    c_id = lv_obj_get_child_cnt((lv_obj_t *)page) - 2; // top bottom
    scr = lp_lvgl_page_add_tile((lv_obj_t *)page, c_id, 1, LV_DIR_TOP | LV_DIR_LEFT | LV_DIR_RIGHT | LV_DIR_BOTTOM);
    lp_lvgl_page_set_tile_id(lp_lvgl_page_father_get(scr), c_id, 1, 1);

    return lp_lvgl_page_app_create_title(scr, 0);
}

/// @brief 返回上一个页面
/// @param tv 首页?
/// @return 0 ok,1 no page can be back
int lp_lvgl_page_back(lv_obj_t *tv)
{
    lp_lvgl_page_t *page = lp_page_app; // lp_lvgl_page_father_get(lv_obj_get_parent(tv));
    int c_id = lv_obj_get_child_cnt((lv_obj_t *)page);

    if (c_id > 3) // 有3个page才能后退的
    {
        page->last_page_id = 0;                                     // 破坏右滑返回的设定
        lp_lvgl_page_set_tile_id((lv_obj_t *)page, c_id - 4, 1, 1); // 设置后退
        lv_obj_del(lv_obj_get_child((lv_obj_t *)page, c_id - 1));   // 删除当前page

        return 0;
    }

    return 1;
}

/// @brief 刷新当前界面
/// @param tv
/// @return
int lp_lvgl_page_refresh(void)
{
    lp_lvgl_page_t *page = lp_page_app;
    lp_lvgl_page_view_cb_t refresh_cb;
    lp_lvgl_page_tile_t *page_tile;
    int c_id = lv_obj_get_child_cnt((lv_obj_t *)page);

    if (c_id > 2) // 有3个page才能后退的
    {
        page_tile = (lp_lvgl_page_tile_t *)lv_obj_get_child((lv_obj_t *)page, c_id - 1);
        refresh_cb = page_tile->refresh_cb;

        if (refresh_cb)
        {
            lv_obj_del((lv_obj_t *)page_tile);              // 删除当前page
            lp_lvgl_page_add_next_with_init(refresh_cb, 1); // 新建一个page
        }
        return 0;
    }

    return 1;
}

/// @brief 现在设置的page -- page只有3个的时候 这函数不一定起作用.迷幻
/// @param obj 当前空间
/// @param tile_obj 需要显示的page
/// @param anim_en 是否滑动过去
void lp_lvgl_page_set_tile(lv_obj_t *obj, lv_obj_t *tile_obj, lv_anim_enable_t anim_en)
{
    lv_coord_t tx = lv_obj_get_x(tile_obj);
    lv_coord_t ty = lv_obj_get_y(tile_obj);

    lp_lvgl_page_tile_t *tile = (lp_lvgl_page_tile_t *)tile_obj;
    lp_lvgl_page_t *tv = (lp_lvgl_page_t *)obj;
    tv->tile_act = (lv_obj_t *)tile;

    lv_obj_set_scroll_dir(obj, tile->dir);
    lv_obj_scroll_to(obj, tx, ty, anim_en);
}
/// @brief 现在设置的page
/// @param tv 当前空间
/// @param col_id 列
/// @param row_id 行
/// @param anim_en 是否滑动过去
void lp_lvgl_page_set_tile_id(lv_obj_t *tv, uint32_t col_id, uint32_t row_id, lv_anim_enable_t anim_en)
{
    lv_obj_update_layout(tv);

    lv_coord_t w = lv_obj_get_content_width(tv);
    lv_coord_t h = lv_obj_get_content_height(tv);

    lv_coord_t tx = col_id * w;
    lv_coord_t ty = row_id * h;

    uint32_t i;
    for (i = 0; i < lv_obj_get_child_cnt(tv); i++)
    {
        lv_obj_t *tile_obj = lv_obj_get_child(tv, i);
        lv_coord_t x = lv_obj_get_x(tile_obj);
        lv_coord_t y = lv_obj_get_y(tile_obj);
        if (x == tx && y == ty)
        {
            lp_lvgl_page_set_tile(tv, tile_obj, anim_en);
            return;
        }
    }

    LV_LOG_WARN("No tile found with at (%d,%d) index", (int)col_id, (int)row_id);
}

lv_obj_t *lp_lvgl_page_get_tile_act(lv_obj_t *obj)
{
    lp_lvgl_page_t *tv = (lp_lvgl_page_t *)obj;
    return tv->tile_act;
}
//
lv_obj_t *lp_lvgl_page_father_get(lv_obj_t *obj)
{
    lp_lvgl_page_tile_t *tv = (lp_lvgl_page_tile_t *)obj;
    return (lv_obj_t *)tv->page_tv;
}

/// @brief 这是app删除的，或可以做隐藏它
/// @param obj page下面的子obj
/// @return 0 ok
int lp_lvgl_page_app_del(lv_obj_t *obj)
{
    lv_obj_del(lp_lvgl_page_father_get(lv_obj_get_parent(obj)));
    return 0;
}
/// @brief 设置label text,但先检查是否原先就是一样的
/// @param obj
/// @param text
void lp_lvgl_label_set_text(lv_obj_t *obj, const char *text)
{
    if (0 == strcmp(lv_label_get_text(obj), text))
    {
        return;
    }
    lv_label_set_text(obj, text);
}
/// @brief 设置label text,但先检查是否原先就是一样的
/// @param obj
/// @param fmt
void lp_lvgl_label_set_text_fmt(lv_obj_t *obj, const char *fmt, ...)
{
    char *temp;
    va_list args;
    va_start(args, fmt);
    temp = _lv_txt_set_text_vfmt(fmt, args);
    va_end(args);

    if (0 == strcmp(lv_label_get_text(obj), temp))
    {
        lv_mem_free(temp);
        return;
    }
    lv_label_set_text(obj, temp);
    lv_mem_free(temp);
}
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////LVGL驱动//////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
static lv_disp_drv_t disp_drv;
static lv_indev_t *indev_touchpad;
static lv_disp_draw_buf_t disp_buf;
static lv_indev_drv_t indev_drv;
static int touch_x = -1, touch_y = -1, touch_status = -1;
void touch_int_get_xy(void)
{
    uint8_t data[6];
    int ret;

    data[0] = 0x15;
    data[1] = 1;
    ret = lpbsp_iic_read(0, data, 6);
    if (ret == 0)
    {
        touch_x = ((0xf & data[2]) << 8) | data[3];
        touch_y = ((0xf & data[4]) << 8) | data[5];
        if (data[1])
            touch_status = 1;
        else
            touch_status = 2;
    }
}

#if 1
int touch_get_xy(int *x, int *y)
{
    uint8_t data[6];
    int ret;
    static uint8_t touch_flg = 0;

    ret = lpbsp_iqr_event(0);
    if (7 == ret)
    {
        data[0] = 0x15;
        data[1] = 1;
        ret = lpbsp_iic_read(0, data, 6);
        if (ret == 0)
        {
            *x = ((0xf & data[2]) << 8) | data[3];
            *y = ((0xf & data[4]) << 8) | data[5];
            if (data[1])
            {
                touch_flg = 3;
                return 1;
            }
            else
            {
                if (touch_flg)
                {
                    touch_flg--;
                    return 1;
                }
            }
        }
    }
    return 0;
}
#else
int touch_get_xy(int *x, int *y)
{
    int status = touch_status;

    *x = touch_x;
    *y = touch_y;
    touch_status = 0;
    return status;
}
#endif
static void touchpad_read(lv_indev_drv_t *indev_drv, lv_indev_data_t *data)
{
    static lv_coord_t last_x = 0;
    static lv_coord_t last_y = 0;
    int x = 0, y = 0;

    if (1 == touch_get_xy(&x, &y))
    {
        last_x = x;
        last_y = y;
        data->state = LV_INDEV_STATE_PR;
    }
    else
    {
        data->state = LV_INDEV_STATE_REL;
    }
    data->point.x = last_x;
    data->point.y = last_y;
}

void lpbsp_lvgl_disp_driver_flush(lv_disp_drv_t *drv, const lv_area_t *area, lv_color_t *color_p)
{
    int dy = area->y2 - area->y1 + 1;
    int dx = area->x2 - area->x1 + 1;
    int dy2 = 0, d2 = 0, t2 = 0;

    dy2 = dy * dx;
    lpbsp_disp_axis(area->x1, area->y1, area->x2, area->y2, dy2, (uint16_t *)color_p);

    lv_disp_flush_ready(drv);
}
/// <summary>
/// lvgl 初始化
/// </summary>
/// <param name=""></param>
int lp_lvgl_init(int draw_buf_size)
{
    int ret = 0;

    if (lv_is_initialized())
        return 1;

    lv_init();
    lpbsp_tft_init(1, 0);
    lp_lvgl_fs_init();

    lp_queue_ui_notify = lpbsp_queue_create(128, 8);

    lv_color_t *buf1 = lpbsp_malloc(draw_buf_size * sizeof(lv_color_t));
    lv_color_t *buf2 = lpbsp_malloc(draw_buf_size * sizeof(lv_color_t));

    lv_disp_draw_buf_init(&disp_buf, buf1, buf2, draw_buf_size);

    lv_disp_drv_init(&disp_drv);
    disp_drv.flush_cb = lpbsp_lvgl_disp_driver_flush;
    disp_drv.hor_res = TFT_VIEW_W;
    disp_drv.ver_res = TFT_VIEW_H;
    disp_drv.draw_buf = &disp_buf;
    lv_disp_drv_register(&disp_drv);

    lv_indev_drv_init(&indev_drv);
    indev_drv.type = LV_INDEV_TYPE_POINTER;
    indev_drv.read_cb = touchpad_read;
    indev_touchpad = lv_indev_drv_register(&indev_drv);

    lv_disp_trig_activity(0); // 软件假装触摸一下
    return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
/// @brief 发送tip提醒
/// @param title 标题
/// @param data 内容
/// @return 0 ok
int lp_lvgl_msg_tip_send(char *title, char *data)
{
    lp_lvgl_msg_tip_t *lp_msg_tip;

    if (0 == title || 0 == data)
        return LF_ERR_PARAMETER_NULL;

    lp_msg_tip = lpbsp_malloc(sizeof(lp_lvgl_msg_tip_t));
    if (0 == lp_msg_tip)
        return LF_ERR_MALLOC;

    lp_msg_tip->title = strlen(title) + 1;
    if (0 == lp_msg_tip->title)
    {
        lpbsp_free(lp_msg_tip);
        return LF_ERR_MALLOC;
    }

    lp_msg_tip->data = strlen(data) + 1;
    if (0 == lp_msg_tip->data)
    {
        lpbsp_free(lp_msg_tip->title);
        lpbsp_free(lp_msg_tip);
        return LF_ERR_MALLOC;
    }

    strcpy(lp_msg_tip->title, title);
    strcpy(lp_msg_tip->data, data);
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
static lv_obj_t *down_prog_obj;
static lv_obj_t *win_obj;
typedef struct _lp_down_info_t
{
    char *url;
    char *fname;
} lp_down_info_t;

static void lp_lvgl_ui_notify_down_cb(void *arg)
{
    uint32_t ret = arg;

    lv_bar_set_value(down_prog_obj, ret, LV_ANIM_ON);
}

/// @brief 下载进度回调处理
/// @param prog 当前进度
/// @param fsize 总大小
static void bootdown_prog_cb(uint32_t prog, uint32_t fsize)
{
    uint32_t ret = 0;

    ret = prog / (fsize / 100);
    lp_lvgl_ui_notify_send(lp_lvgl_ui_notify_down_cb, ret);
}

static void lp_lvgl_ui_notify_cb(void *arg)
{
    int ret = 0;
    lv_obj_t *mbox1 = 0;
    char tbuf[24] = {0};

    lv_obj_del(win_obj);

    ret = (int)arg;
    if (ret)
    {
        sprintf(tbuf, "err code %d", ret);
        mbox1 = lv_msgbox_create(NULL, "Down file", tbuf, 0, true);
    }
    else
    {
        mbox1 = lv_msgbox_create(NULL, "Down file", "Down file ok", 0, true);
    }
    lv_obj_center(mbox1);
}

/// @brief 下载app固件线程
/// @param arg
static void down_task(void *arg)
{
    int ret = 0;
    lp_down_info_t *lp_down_info = (lp_down_info_t *)arg;

    lpbsp_bl_set(1);
    ret = lp_http_file_down(lp_down_info->url, lp_down_info->fname, bootdown_prog_cb);
    LP_LOGD("lp_http_file_down %d\r\n", ret);

    lpbsp_free(lp_down_info);
    lpbsp_bl_set(10);
    lp_lvgl_ui_notify_send(lp_lvgl_ui_notify_cb, ret);

    lpbsp_task_delete(0); // del task
}

/// @brief 把win关闭事件后的线程问题理清楚
/// @param e
static void win_event_handler(lv_event_t *e)
{
    lv_obj_t *obj = lv_event_get_target(e);
    LP_LOGD("Button %d clicked", (int)lv_obj_get_index(obj));
}

/// @brief 下载文件
/// @param url 网址-需要支持分包下载
/// @param fname 存储文件路径
/// @return 0:ok 1:err
int lp_lvgl_down2file(char *url, char *fname)
{
    lv_obj_t *btn, *lab;
    int ret = 0;
    lp_down_info_t *lp_down_info;

    lp_down_info = lpbsp_malloc(sizeof(lp_down_info_t));
    lp_down_info->url = url;
    lp_down_info->fname = fname;
    win_obj = lv_win_create(lv_scr_act(), 60);
    lv_win_add_title(win_obj, "Down file");
    btn = lv_win_add_btn(win_obj, LV_SYMBOL_CLOSE, 60);
    lv_obj_add_event_cb(btn, win_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cont = lv_win_get_content(win_obj); /*Content can be added here*/

    down_prog_obj = lv_bar_create(cont);
    lv_obj_set_pos(down_prog_obj, 5, 0);
    lv_obj_set_size(down_prog_obj, 205, 17);

    lab = lv_label_create(cont);
    lv_obj_set_pos(lab, 5, 20);
    lv_obj_set_size(lab, 205, 170);
    lp_lvgl_label_set_text_fmt(lab, "down [%s] to [%s]", url, fname);

    lp_unlink(fname);
    ret = lpbsp_task_create("down_task", lp_down_info, down_task, 4096, 3);
    LP_LOGD("task produce start %d\r\n", ret);
    return 0;
}

/// @brief 下载app固件线程
/// @param arg
static void up_task(void *arg)
{
    int ret = 0;
    lp_down_info_t *lp_down_info = (lp_down_info_t *)arg;

    lpbsp_bl_set(1);
    ret = lp_http_file_up(lp_down_info->url, lp_down_info->fname, bootdown_prog_cb);
    LP_LOGD("lp_http_file_down %d\r\n", ret);

    lpbsp_free(lp_down_info->fname);
    lpbsp_free(lp_down_info->url);
    lpbsp_free(lp_down_info);
    lpbsp_bl_set(10);
    lp_lvgl_ui_notify_send(lp_lvgl_ui_notify_cb, ret);

    lpbsp_task_delete(0); // del task
}
/// @brief 下载文件
/// @param url 网址-需要支持分包下载
/// @param fname 存储文件路径
/// @return 0:ok 1:err
int lp_lvgl_upfile(char *url, char *fname)
{
    lv_obj_t *btn, *lab;
    int ret = 0;
    lp_down_info_t *lp_down_info;

    lp_down_info = lpbsp_malloc(sizeof(lp_down_info_t));
    lp_down_info->url = lpbsp_malloc(strlen(url) + 1);
    lp_down_info->fname = lpbsp_malloc(strlen(fname) + 1);
    strcpy(lp_down_info->url, url);
    strcpy(lp_down_info->fname, fname);
    win_obj = lv_win_create(lv_scr_act(), 60);
    lv_win_add_title(win_obj, "Up file");
    btn = lv_win_add_btn(win_obj, LV_SYMBOL_CLOSE, 60);
    lv_obj_add_event_cb(btn, win_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cont = lv_win_get_content(win_obj); /*Content can be added here*/

    down_prog_obj = lv_bar_create(cont);
    lv_obj_set_pos(down_prog_obj, 5, 0);
    lv_obj_set_size(down_prog_obj, 205, 17);

    lab = lv_label_create(cont);
    lv_obj_set_pos(lab, 5, 20);
    lv_obj_set_size(lab, 205, 170);
    lp_lvgl_label_set_text_fmt(lab, "down [%s] to [%s]", url, fname);

    ret = lpbsp_task_create("up_task", lp_down_info, up_task, 4096, 3);
    LP_LOGD("task produce start %d\r\n", ret);
    return 0;
}

static void down_file_task(void *arg)
{
    int ret = 0;
    lp_down_info_t *lp_down_info = (lp_down_info_t *)arg;

    lpbsp_bl_set(1);
    ret = lp_http_file_down_data(lp_down_info->url, lp_down_info->fname, bootdown_prog_cb);
    LP_LOGD("lp_http_file_down %d\r\n", ret);

    lpbsp_free(lp_down_info);
    lpbsp_bl_set(10);
    lp_lvgl_ui_notify_send(lp_lvgl_ui_notify_cb, ret);

    lpbsp_task_delete(0); // del task
}
// 下载文件到prog_cb
int lp_lvgl_down2data(char *url, int (*prog_cb)(uint8_t *data, uint32_t type, uint32_t value))
{
    lv_obj_t *btn, *lab;
    int ret = 0;
    lp_down_info_t *lp_down_info;

    lp_down_info = lpbsp_malloc(sizeof(lp_down_info_t));
    lp_down_info->url = url;
    lp_down_info->fname = prog_cb;
    win_obj = lv_win_create(lv_scr_act(), 60);
    lv_win_add_title(win_obj, "Down file");
    btn = lv_win_add_btn(win_obj, LV_SYMBOL_CLOSE, 60);
    lv_obj_add_event_cb(btn, win_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cont = lv_win_get_content(win_obj); /*Content can be added here*/

    down_prog_obj = lv_bar_create(cont);
    lv_obj_set_pos(down_prog_obj, 5, 0);
    lv_obj_set_size(down_prog_obj, 205, 17);

    lab = lv_label_create(cont);
    lv_obj_set_pos(lab, 5, 20);
    lv_obj_set_size(lab, 205, 170);
    lp_lvgl_label_set_text_fmt(lab, "down [%s] to data", url);

    ret = lpbsp_task_create("down_task", lp_down_info, down_file_task, 4096, 3);
    LP_LOGD("task produce start %d\r\n", ret);
    return 0;
}
typedef int (*func_prog_cb)(void *arg, void *func_prog);
static void func_progress_task(void *arg)
{
    int ret = 0;
    lp_down_info_t *lp_down_info = (lp_down_info_t *)arg;
    func_prog_cb func_prog;
    void *func_arg;

    func_arg = lp_down_info->url;
    func_prog = lp_down_info->fname;
    lpbsp_free(lp_down_info);

    ret = func_prog(func_arg, bootdown_prog_cb);

    lp_lvgl_ui_notify_send(lp_lvgl_ui_notify_cb, ret);

    lpbsp_task_delete(0); // del task
}
// func prog_cb
int lp_lvgl_func_progress(char *title, char *show_txt, void *arg, int (*prog_cb)(void *arg, void *func_prog))
{
    lv_obj_t *btn, *lab;
    int ret = 0;
    lp_down_info_t *lp_down_info;

    lp_down_info = lpbsp_malloc(sizeof(lp_down_info_t));
    lp_down_info->url = arg;
    lp_down_info->fname = prog_cb;
    win_obj = lv_win_create(lv_scr_act(), 60);
    lv_win_add_title(win_obj, title);
    btn = lv_win_add_btn(win_obj, LV_SYMBOL_CLOSE, 60);
    lv_obj_add_event_cb(btn, win_event_handler, LV_EVENT_CLICKED, NULL);

    lv_obj_t *cont = lv_win_get_content(win_obj); /*Content can be added here*/

    down_prog_obj = lv_bar_create(cont);
    lv_obj_set_pos(down_prog_obj, 5, 0);
    lv_obj_set_size(down_prog_obj, 205, 17);

    lab = lv_label_create(cont);
    lv_obj_set_pos(lab, 5, 20);
    lv_obj_set_size(lab, 205, 170);
    lp_lvgl_label_set_text(lab, show_txt);

    ret = lpbsp_task_create("down_task", lp_down_info, func_progress_task, 4096, 3);
    LP_LOGD("task produce start %d\r\n", ret);
    return 0;
}
typedef struct _lp_lvgl_func_run_t
{
    char *arg;
    char *func;
} lp_lvgl_func_run_t;
typedef int (*func_run_cb)(void *arg);
static void lvgl_msgbox_run_cb_callback(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_current_target(e);
    lp_lvgl_func_run_t *lp_func;
    func_run_cb func_run;
    void *arg;
    char tbuf[16];
    int ret;

    lp_func = lv_event_get_user_data(e);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        func_run = lp_func->func;
        arg = lp_func->arg;
        ret = func_run(arg);
        lv_msgbox_close(obj);

        sprintf(tbuf, "ret = %d", ret);
        lv_obj_center(lv_msgbox_create(NULL, "func", tbuf, 0, true));
    }
    else if (LV_EVENT_DELETE == code)
    {
        lpbsp_free(lp_func);
    }
}
int lp_lvgl_func_run(char *title, char *show_txt, void *arg, int (*func_cb)(void *arg))
{
    static const char *btns[] = {"Yes", 0};
    lv_obj_t *mbox1;
    lp_lvgl_func_run_t *lp_func;

    lp_func = lpbsp_malloc(sizeof(lp_lvgl_func_run_t));
    lp_func->arg = arg;
    lp_func->func = func_cb;
    mbox1 = lv_msgbox_create(NULL, title, show_txt, btns, true);
    lv_obj_add_event_cb(mbox1, lvgl_msgbox_run_cb_callback, LV_EVENT_VALUE_CHANGED, lp_func);
    lv_obj_center(mbox1);
    return 0;
}

/// @brief lvgl 错误提醒
/// @param code  code
/// @return
int lp_lvgl_msg_err_code(char *title, int code)
{
    lv_obj_t *mbox1;
    char tbuf[24] = {0};

    sprintf(tbuf, "error code %d", code);
    mbox1 = lv_msgbox_create(NULL, title, tbuf, 0, true);
    if (mbox1)
        lv_obj_center(mbox1);
    else
        return 1;
    return 0;
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
static lp_lvgl_page_input_cb_t input_cb = 0;
static char input_ole[128];
static char input_new[128];
static void *input_arg;
static lv_obj_t *input_src;
//  键盘输入
static void ta_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);
    lv_obj_t *kb = lv_event_get_user_data(e);
    lv_obj_t *obj;

    if (code == LV_EVENT_FOCUSED)
    {
        if (lv_indev_get_type(lv_indev_get_act()) != LV_INDEV_TYPE_KEYPAD)
        {
            lv_keyboard_set_textarea(kb, ta);
            lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
        }
    }
    else if (code == LV_EVENT_READY)
    {
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
        lv_obj_clear_state(ta, LV_STATE_FOCUSED);
        lv_indev_reset(NULL, ta); /*To forget the last clicked object to make it focusable again*/

        obj = lv_textarea_get_label(ta);
        strcpy(input_new, lv_label_get_text(obj));
        if (input_cb)
            input_cb(input_arg, input_ole, input_new);
        lv_obj_del(input_src);
    }
}

/// @brief 输入对话框
/// @param data 输入buf
/// @param max_len 输入buf 长度
/// @param cb 输入完成回调
/// @return 0:ok 1:err
int lp_lvgl_input_pinyin(void *arg, char *ole, lp_lvgl_page_input_cb_t cb)
{
    // lv_obj_t *pinyin_ime;

    // strcpy(input_ole, ole);
    // input_cb = cb;
    // input_arg = arg;
    // input_src = lv_obj_create(lv_scr_act());
    // lv_obj_set_pos(input_src, 0, 0);
    // lv_obj_set_size(input_src, 240, 280);

    // pinyin_ime = lv_ime_pinyin_create(input_src);
    // lv_obj_set_style_text_font(pinyin_ime, &lp_lvgl_font_icon, 0);

    // /* ta1 */
    // lv_obj_t *ta1 = lv_textarea_create(input_src);
    // lv_textarea_set_one_line(ta1, true);
    // lv_obj_set_style_text_font(ta1, &lp_lvgl_font_icon, 0);
    // lv_obj_set_pos(ta1, 0, 0);
    // lv_obj_set_size(ta1, 180, 40);

    // /*Create a keyboard and add it to ime_pinyin*/
    // lv_obj_t *kb = lv_keyboard_create(input_src);
    // lv_obj_set_style_text_font(kb, &lp_lvgl_font_icon, 0);
    // lv_obj_set_size(kb, 240, 180);
    // // lv_obj_set_height(kb, 180);
    // lv_keyboard_set_textarea(kb, ta1);
    // lv_ime_pinyin_set_keyboard(pinyin_ime, kb);
    // lv_ime_pinyin_set_mode(pinyin_ime, LV_IME_PINYIN_MODE_K9); // Set to 9-key input mode. Default: 26-key input(k26) mode.
    // lv_obj_add_event_cb(ta1, ta_event_cb, LV_EVENT_ALL, kb);

    // /*Get the cand_panel, and adjust its size and position*/
    // lv_obj_t *cand_panel = lv_ime_pinyin_get_cand_panel(pinyin_ime);
    // lv_obj_set_size(cand_panel, LV_PCT(100), LV_PCT(10));
    // lv_obj_align_to(cand_panel, kb, LV_ALIGN_OUT_TOP_MID, 0, 0);

    // lv_textarea_set_text(ta1, ole);
    return 0;
}
static void en_ta_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *ta = lv_event_get_target(e);
    lv_obj_t *kb = lv_event_get_user_data(e);
    lv_obj_t *obj;

    if (code == LV_EVENT_FOCUSED)
    {
        lv_keyboard_set_textarea(kb, ta);
        lv_obj_clear_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
    else if (code == LV_EVENT_DEFOCUSED)
    {
        lv_keyboard_set_textarea(kb, NULL);
        lv_obj_add_flag(kb, LV_OBJ_FLAG_HIDDEN);
    }
    else if (code == LV_EVENT_READY)
    {
        obj = lv_textarea_get_label(ta);
        strcpy(input_new, lv_label_get_text(obj));
        if (input_cb)
            input_cb(input_arg, input_ole, input_new);
        lv_obj_del(input_src);
    }
    else if (code == LV_EVENT_CANCEL)
    {
#if 0
        obj = lv_textarea_get_label(ta);
        strcpy(input_new, lv_label_get_text(obj));
        if (input_cb)
            input_cb(input_arg, input_ole, input_new);
#endif
        lv_obj_del(input_src);
    }
}
/// @brief 输入对话框 - 英文
/// @param data 输入buf
/// @param max_len 输入buf 长度
/// @param cb 输入完成回调
/// @return 0:ok 1:err
int lp_lvgl_input_en(void *arg, char *ole, lp_lvgl_page_input_cb_t cb)
{
    lv_obj_t *kb, *ta;

    strcpy(input_ole, ole);
    input_cb = cb;
    input_arg = arg;
    input_src = lv_obj_create(lv_scr_act());
    lv_obj_remove_style_all(input_src);
    lv_obj_set_pos(input_src, 0, 0);
    lv_obj_set_size(input_src, 240, 280);

    kb = lv_keyboard_create(input_src);
    lv_obj_set_style_text_font(kb, &lv_font_montserrat_22, 0);
    lv_obj_set_size(kb, 240, 240);

    ta = lv_textarea_create(input_src);
    lv_obj_set_style_text_font(ta, &lv_font_montserrat_22, 0);
    lv_obj_set_pos(ta, 0, 0);
    lv_obj_set_size(ta, 240, 45);
    lv_obj_add_event_cb(ta, en_ta_event_cb, LV_EVENT_ALL, kb);
    lv_textarea_set_text(ta, ole);

    lv_keyboard_set_textarea(kb, ta);
    return 0;
}
int lp_lvgl_input(void *arg, char *ole, lp_lvgl_page_input_cb_t cb)
{
    return 0;
}
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////
#define TOP_MSG_NUM 16
static int top_view_off_y = 0;
static int top_view_off_off = 0;
static int obj_top_msg_num = 0;
static lv_obj_t *obj_top_clr;
static lv_obj_t *obj_top_src;
static lv_obj_t *obj_top_msg[TOP_MSG_NUM] = {0};
static const char *TOP_TEXT[] = {
    {LV_SYMBOL_WIFI},
    {LV_SYMBOL_BATTERY_EMPTY},
    {LV_SYMBOL_GPS},
    {LV_SYMBOL_BELL},
};
static void view_page_close_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (LV_EVENT_DELETE == code)
    {
        obj_top_msg_num = 0;
    }
}
static int lp_lvgl_page_view_top_show(lv_obj_t *src_obj)
{
    lv_obj_t *label, *bottom;
    int i = 0;

    static lv_style_t font_style; // 定义LV_STYLE
    lv_style_init(&font_style);   // 初始化LV_STYLE
    // 下面的部分就是用来修饰你的style的

    // lv_style_set_text_font(&font_style, &lp_font_icon);                    // 设置字体大小为48
    // lv_style_set_text_color(&font_style, lv_color_make(0x00, 0xff, 0x00)); // 设置字体颜色为绿色
    lv_style_set_text_align(&font_style, LV_TEXT_ALIGN_CENTER);

    lv_obj_add_event_cb(src_obj, view_page_close_cb, LV_EVENT_DELETE, 0);

    obj_top_src = lv_obj_create(src_obj);
    lv_obj_remove_style_all(obj_top_src);
    lv_obj_set_pos(obj_top_src, 0, 0);
    lv_obj_set_size(obj_top_src, 240, 270);
    for (i = 0; i < ARR_SIZE(TOP_TEXT); i++)
    {
        bottom = lv_btn_create(obj_top_src);
        bottom->user_data = (void *)i;
        lv_obj_set_pos(bottom, i % 3 * 80 + 5, 5 + i / 3 * 65);
        lv_obj_set_size(bottom, 70, 60);
        lv_obj_set_style_radius(bottom, LV_VER_RES * 0.287, 0);

        lv_obj_add_event_cb(bottom, top_ico_btn_event_handler, LV_EVENT_VALUE_CHANGED, NULL);
        lv_obj_add_flag(bottom, LV_OBJ_FLAG_CHECKABLE);

        switch (i)
        {
        case 0:
            if (lpbsp_wifi_event(1))
                lv_obj_add_state(bottom, LV_STATE_CHECKED);
            break;
        case 1:
            break;
        case 2:
            if (lp_gps_state())
                lv_obj_add_state(bottom, LV_STATE_CHECKED);
            break;
        case 3:
            break;
        }

        label = lv_label_create(bottom);
        lp_lvgl_label_set_text(label, TOP_TEXT[i]);
        lv_obj_center(label);

        lv_obj_add_style(label, &font_style, LV_PART_ANY);      // 给label的所有部分
        lv_obj_add_style(label, &font_style, LV_STATE_DEFAULT); // 给label的默认状态
    }
    top_view_off_y = 5 + (i + 2) / 3 * 65;
    top_view_off_off = top_view_off_y;

#if 0
    lp_lvgl_page_notify("Alarm", "wake up");
    lp_lvgl_page_notify("Sport", "today 17:50");
    lp_lvgl_page_notify("Weather", "rain and wind");
#endif
    return 0;
}
static void top_ico_btn_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_current_target(e);
    uint8_t checked = lv_obj_get_state(obj) & 0xfd;
    int index = 0;

    index = obj->user_data;
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        switch (index)
        {
        case 2:
            lp_gps_init(checked);
            if (0 == checked)
                bar_ico_flg &= 0xfc;
            else
                bar_ico_flg &= 0xfD;
            bar_ico_flg |= checked;
            break;

        default:
            lp_lvgl_page_notify("Alarm", "wake up");
            break;
        }
    }
    LP_LOGD("ico_btn index %d checked %d bar_ico_flg  %x\r\n", index, checked, bar_ico_flg);
}

static void top_msg_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_current_target(e);
    static int last_x = 0;
    int temp;

    lv_indev_t *indev = lv_indev_get_act();
    if (indev == NULL)
        return;
    lv_point_t p;
    lv_indev_get_point(indev, &p);

    if (code == LV_EVENT_PRESSED)
    {
        last_x = p.x;
    }
    else if (code == LV_EVENT_PRESSING)
    {
        temp = lv_obj_get_x(obj) + p.x - last_x;
        if (temp > -60 && 10 > temp)
            lv_obj_set_x(obj, temp);
    }
    else if (code == LV_EVENT_RELEASED)
    {
        if (lv_obj_get_x(obj) > -5)
        {
            lv_obj_set_x(obj, 10);
        }
        else
        {
            lv_obj_set_x(obj, -60);
        }
    }
}

/// @brief 清空一条msg
/// @param e
static void top_msg_clr_event_handler(lv_event_t *e)
{
    int y = 0, tempi, i;
    lv_obj_t *obj = lv_event_get_user_data(e);

    y = lv_obj_get_y(obj);
    tempi = (y - top_view_off_off) / 70 + 1;
    lv_obj_del(obj);

    top_view_off_y -= 70;
    for (i = tempi; i < obj_top_msg_num; i++)
    {
        if (obj_top_msg[i])
        {
            obj_top_msg[i - 1] = obj_top_msg[i];
            lv_obj_set_y(obj_top_msg[i], top_view_off_off + i * 70 - 70);
        }
        else
            break;
    }

    obj_top_msg_num--;
    if (obj_top_msg_num) // 还有msg
    {
        tempi = lv_obj_get_y(obj_top_clr);
        lv_obj_set_y(obj_top_clr, tempi - 70);
    }
    else // 完全没msg了
    {
        lv_obj_del(obj_top_clr);
    }
}

/// @brief 清空所有的msg
/// @param e
static void top_msg_clr_all_event_handler(lv_event_t *e)
{
    int i = 0;
    for (i = 0; i < obj_top_msg_num; i++)
    {
        lv_obj_del(obj_top_msg[i]);
    }
    lv_obj_del(obj_top_clr);
    obj_top_msg_num = 0;
    top_view_off_y = top_view_off_off; //
}

/// @brief 消息通知
/// @param title 标题
/// @param msg 内容
/// @return
int lp_lvgl_page_notify(char *title, char *msg)
{
    lv_obj_t *obj, *btn, *lab;
    int temp = 0;

    if (TOP_MSG_NUM == obj_top_msg_num) // 已经没位置了
        return 1;

    obj = lv_obj_create(obj_top_src);
    obj_top_msg[obj_top_msg_num++] = obj;
    lv_obj_set_style_pad_all(obj, 0, 0);
    lv_obj_set_style_pad_right(obj, 5, 0);
    lv_obj_set_style_bg_color(obj, lv_color_make(0x62, 0x71, 0x7f), 0);
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_DEFAULT);
    lv_obj_set_style_bg_opa(obj, LV_OPA_0, LV_PART_SCROLLBAR | LV_STATE_SCROLLED);
    lv_obj_set_pos(obj, 5, top_view_off_y);
    lv_obj_set_size(obj, 230, 65);

    lab = lv_label_create(obj);
    lv_obj_set_pos(lab, 5, 0);
    lp_lvgl_label_set_text(lab, title);

    lab = lv_label_create(obj);
    lv_obj_set_pos(lab, 5, 30);
    lp_lvgl_label_set_text(lab, msg);

    btn = lv_btn_create(obj);
    lv_obj_set_pos(btn, 230, 2);
    lv_obj_set_size(btn, 65, 55);
    lab = lv_label_create(btn);
    lv_obj_center(lab);
    lp_lvgl_label_set_text(lab, lp_i18n_string("Clr"));
    lv_obj_add_event_cb(btn, top_msg_clr_event_handler, LV_EVENT_CLICKED, obj);

    top_view_off_y += 70;

    if (1 == obj_top_msg_num) // 第一个有消息
    {
        obj_top_clr = lv_btn_create(obj_top_src);
        lv_obj_set_pos(obj_top_clr, 10, top_view_off_y);
        lv_obj_set_size(obj_top_clr, 220, 55);
        lab = lv_label_create(obj_top_clr);
        lv_obj_center(lab);
        lp_lvgl_label_set_text(lab, lp_i18n_string("Clr"));
        lv_obj_add_event_cb(obj_top_clr, top_msg_clr_all_event_handler, LV_EVENT_CLICKED, obj);

        lv_obj_update_layout(obj_top_clr);
        temp = lv_obj_get_y(obj_top_clr);
    }
    else // 多个消息
    {
        lv_obj_update_layout(obj_top_clr);
        temp = lv_obj_get_y(obj_top_clr);
        lv_obj_set_y(obj_top_clr, temp + 70);
    }
    return 0;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
static lv_obj_t *main_app;
/// @brief app 列表被点击了
/// @param e
static void main_page_btn_callback(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *btn = lv_event_get_target(e);
    int page_id = (uint32_t)lv_obj_get_user_data(btn);
    // 动画完成自动删除
    if (code == LV_EVENT_SHORT_CLICKED)
    {
        lp_lvgl_page_app_del(main_app); // 删除本main app

        lv_obj_t *tile1 = lp_lvgl_page_app_create(lv_scr_act(), 1);
        lpbsp_main_api->api[page_id](tile1);
        lp_lvgl_page_sysmenu_text_set(lpbsp_main_api->name[page_id]); // app 的左上角设置为ico
    }
}

/// @brief app列表
/// @param main_cont_obj
/// @return
static int view_page_main(lv_obj_t *main_cont_obj)
{
    int i = 0;
    lv_obj_t *btn_obj, *lab_obj;

    lp_lvgl_page_title_for_main_init(main_cont_obj, 0); // 总的title
    for (i = 0; i < lpbsp_main_api->num; i++)
    {
        btn_obj = lv_btn_create(main_cont_obj);
        lv_obj_set_user_data(btn_obj, (void *)i);
        lv_obj_remove_style_all(btn_obj);
        lv_obj_set_pos(btn_obj, 20, 25 + i * TFT_OBJ_HEIGHT_GAP);
        lv_obj_set_size(btn_obj, 200, TFT_OBJ_HEIGHT);
        lv_obj_set_style_radius(btn_obj, 10, 0);
        lv_obj_set_style_bg_color(btn_obj, lv_color_hex(0xff), 0);
        lv_obj_set_style_bg_opa(btn_obj, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(btn_obj, lv_color_hex(0xff), LV_STATE_FOCUSED);
        lv_obj_set_style_bg_opa(btn_obj, LV_OPA_80, LV_STATE_FOCUSED);
        lv_obj_set_style_bg_opa(btn_obj, LV_OPA_60, LV_STATE_PRESSED);
        lv_obj_add_flag(btn_obj, LV_OBJ_FLAG_EVENT_BUBBLE);

        lab_obj = lv_label_create(btn_obj);
        lv_label_set_text(lab_obj, lpbsp_main_api->name[i]);
        lv_obj_set_style_text_color(lab_obj, lv_color_white(), 0);
        lv_obj_set_align(lab_obj, LV_ALIGN_LEFT_MID);

        lv_obj_add_event_cb(btn_obj, main_page_btn_callback, LV_EVENT_SHORT_CLICKED, NULL);
    }
    return 0;
}
/// @brief app列表
/// @param main_cont_obj
/// @return
static int view_page_main_showup(lv_obj_t *main_cont_obj)
{
    int i = 0;
    lv_obj_t *btn_obj, *lab_obj;

    lp_lvgl_page_title_for_main_init(main_cont_obj, 5); // 总的title
    for (i = 0; i < lpbsp_main_api->num; i++)
    {
        btn_obj = lv_btn_create(main_cont_obj);
        lv_obj_set_user_data(btn_obj, (void *)i);
        lv_obj_remove_style_all(btn_obj);
        lv_obj_set_pos(btn_obj, 20, 25 + 5 + i * TFT_OBJ_HEIGHT_GAP);
        lv_obj_set_size(btn_obj, 200, TFT_OBJ_HEIGHT);
        lv_obj_set_style_radius(btn_obj, 10, 0);
        lv_obj_set_style_bg_color(btn_obj, lv_color_hex(0xff), 0);
        lv_obj_set_style_bg_opa(btn_obj, LV_OPA_COVER, 0);
        lv_obj_set_style_bg_color(btn_obj, lv_color_hex(0xff), LV_STATE_FOCUSED);
        lv_obj_set_style_bg_opa(btn_obj, LV_OPA_80, LV_STATE_FOCUSED);
        lv_obj_set_style_bg_opa(btn_obj, LV_OPA_60, LV_STATE_PRESSED);
        lv_obj_add_flag(btn_obj, LV_OBJ_FLAG_EVENT_BUBBLE);

        lab_obj = lv_label_create(btn_obj);
        lv_label_set_text(lab_obj, lpbsp_main_api->name[i]);
        lv_obj_set_style_text_color(lab_obj, lv_color_white(), 0);
        lv_obj_set_align(lab_obj, LV_ALIGN_LEFT_MID);

        lv_obj_add_event_cb(btn_obj, main_page_btn_callback, LV_EVENT_CLICKED, NULL);
    }
    return 0;
}
/// <summary>
/// app列表界面
/// </summary>
/// <param name="obj"></param>
/// <returns></returns>
static int view_page_main_app(lv_obj_t *obj)
{
    main_app = lp_lvgl_page_app_create(obj, 0);

    view_page_main(main_app);
    return 0;
}
/////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////
int lp_lvgl_ui_notify_run(void)
{
    lp_lvgl_ui_msg_t lp_ui_msg;

    if (lpbsp_queue_receive(lp_queue_ui_notify, &lp_ui_msg, 0))
    {
        lp_ui_msg.cb(lp_ui_msg.arg);
    }
    return 0;
}
int lp_lvgl_ui_notify_send(lp_lvgl_ui_notify_t cb, void *arg)
{
    lp_lvgl_ui_msg_t lp_ui_msg;

    lp_ui_msg.cb = cb;
    lp_ui_msg.arg = arg;
    lpbsp_queue_send(lp_queue_ui_notify, &lp_ui_msg, 0xffff);
    return 0;
}
////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////i18N///////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
static uint32_t location_index = 0;
static lp_i18n_t *lp_i18n = 0;
/// @brief 设置位置
/// @param loction country code -en,zh-cn,jp
/// @return
int lp_i18n_location_set(char *loction)
{
    int i = 0;

    if (0 == loction)
        return LF_ERR_PARAMETER_NULL;
    if (0 == lp_i18n)
        return LF_ERR;

    for (i = 0; i < lp_i18n->lang_num; i++)
    {
        if (0 == strcmp(loction, lp_i18n->lang[i]))
        {
            location_index = i;
            return 0;
        }
    }

    return LF_ERR_PARAMETER;
}

/// @brief 获取已经设置的语言
/// @param loction buf mini 8byte
/// @return
int lp_i18n_location_get(char *loction)
{
    int i = 0;

    if (0 == loction)
        return LF_ERR_PARAMETER_NULL;
    if (0 == lp_i18n)
        return LF_ERR;

    strcpy(loction, (const char *)lp_i18n->lang[location_index]);
    return 0;
}

/*
id,en,cn,jp

*/

/// @brief 语言文本
/// @param profile - 路径
/// @return
int lp_i18n_profile_set(char *profile)
{
    return 0;
}

/// @brief 获取存在的语言
/// @param index
/// @return
char *lp_i18n_lang_get(uint32_t index)
{
    if (0 == lp_i18n)
        return 0;

    if (index >= lp_i18n->lang_num)
        return 0;

    return lp_i18n->lang[index];
}
/// @brief 返回语言数量
/// @param
/// @return 数量
int lp_i18n_lang_num_get(void)
{
    if (0 == lp_i18n)
        return 0;

    return lp_i18n->lang_num;
}

/// @brief 语言数据设置
/// @param profile
/// @return
int lp_i18n_set(lp_i18n_t *p)
{
    if (0 == p)
    {
        return LF_ERR_PARAMETER_NULL;
    }

    lp_i18n = p;
    return 0;
}

/// @brief 用英语的获取设置的语言
/// @param en_str
/// @return
char *lp_i18n_string(char *en_str)
{
    int i = 0;
    char **temp;
    unsigned int *crc, crc_str = 0;

    if (0 == location_index || 0 == en_str || 0 == lp_i18n)
        return en_str;

#if 1
    crc = lp_i18n->crc;
    // crc_str = lp_crc32((uint8_t *)en_str, strlen(en_str));
    for (i = 0; i < lp_i18n->data_num; i++)
    {
        if (crc_str == crc[i])
        {
            temp = lp_i18n->data[location_index];
            return temp[i];
        }
    }
#else
    temp = lp_i18n->data[0];
    for (i = 0; i < lp_i18n->data_num; i++)
    {
        if (0 == strcmp(en_str, temp[i]))
        {
            temp = lp_i18n->data[location_index];
            return temp[i];
        }
    }
#endif
    return en_str;
}
/// @brief 用英语的获取设置的语言-带ico
/// @param en_str
/// @return
char *lp_i18n_string_ico(char *ico, char *en_str)
{
    static char tbuf[64] = {0};
    int len = 0;

    if (0 == ico)
        return lp_i18n_string(en_str);

    len = strlen(ico);
    memcpy(tbuf, ico, len);
    strcpy(&tbuf[len], lp_i18n_string(en_str));
    return tbuf;
}

////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////// fifo //////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////

/**
 * @brief  初始化FIFO
 * @param  p_fifo I 初始化FIFO的结构体
 * @param  p_buf  I FIFO的buf
 * @param  buf_size  I FIFO的buf的size
 * @retval 0 成功入
 */
int lp_fifo_init(lp_fifo_t *p_fifo, uint8_t *p_buf, uint16_t buf_size)
{
    memset(p_fifo, 0, sizeof(lp_fifo_t));
    p_fifo->fifo_len = buf_size;
    p_fifo->fifo_buff = p_buf;
    return 0;
}

/**
 * @brief  初始化FIFO
 * @param  p_fifo I 初始化FIFO的结构体
 * @param  buf_size  I FIFO的buf的size
 * @retval 0 成功,1 malloc err
 */
int lp_fifo_malloc_init(lp_fifo_t *p_fifo, uint16_t buf_size)
{
    memset(p_fifo, 0, sizeof(lp_fifo_t));
    p_fifo->fifo_len = buf_size;
    p_fifo->fifo_buff = lpbsp_malloc(buf_size);
    if (0 == p_fifo->fifo_buff)
        return 1;
    return 0;
}

/**
 * @brief  反初始化FIFO
 * @param  p_fifo I 初始化FIFO的结构体
 * @retval 0 成功,1 malloc err
 */
int lp_fifo_malloc_deinit(lp_fifo_t *p_fifo)
{
    if (p_fifo && p_fifo->fifo_buff)
    {
        lpbsp_free(p_fifo->fifo_buff);
    }
    return 0;
}

/**
 * @brief  将数据压入FIFO
 * @param  p_fifo I FIFO的结构体
 * @param  data I 待入FIFO的数据
 * @param  len  I 待入FIFO的数据量
 * @retval 1 成功入fifo 0 fifo空间不足
 */
int lp_fifo_write(lp_fifo_t *p_fifo, uint8_t *data, int len)
{
    int i;

    if (len > (p_fifo->fifo_len - p_fifo->fifo_size)) // 判断FIFO是否还有空间
        return 1;
    for (i = 0; i < len; i++)
    {
        p_fifo->fifo_buff[p_fifo->fifo_top] = data[i];
        // 计算下一个地址
        p_fifo->fifo_top = (p_fifo->fifo_top + 1) % p_fifo->fifo_len;
    }
    p_fifo->fifo_size += len;
    return 0;
}
/**
 * \brief  获取数据，并弹出FIFO
 * @param  p_fifo I FIFO的结构体
 * @param  buf O 数据输出地址
 * @param  size I 要出FIFO的个数
 * @retval 1成功 0失败
 */
int lp_fifo_read(lp_fifo_t *p_fifo, uint8_t *buf, int size)
{
    int i;
    if (p_fifo->fifo_size < size)
        return 1; // FIFO剩余数据量不足
    for (i = 0; i < size; i++)
    {
        buf[i] = p_fifo->fifo_buff[p_fifo->fifo_bot];
        p_fifo->fifo_bot = (p_fifo->fifo_bot + 1) % p_fifo->fifo_len;
    }
    p_fifo->fifo_size -= size;
    return 0;
}

/**
 * \brief  尝试读一个字节出来，并把长度发回
 * @param  p_fifo I FIFO的结构体
 * @param  buf O 数据输出
 * @retval 长度
 */
int lp_fifo_peek(lp_fifo_t *p_fifo, uint8_t *buf)
{
    if (0 == p_fifo->fifo_size)
    {
        buf[0] = 0;
        return 0;
    }
    buf[0] = p_fifo->fifo_buff[p_fifo->fifo_bot];
    return p_fifo->fifo_size;
}
/**
 * @brief  将数据压入FIFO (写len 带长度的 byte)
 * @param  p_fifo I FIFO的结构体
 * @param  data I 待入FIFO的数据
 * @param  len  I 待入FIFO的数据量
 * @retval 1 成功入fifo 0 fifo空间不足
 */
int lp_fifo_msg_set(lp_fifo_t *p_fifo, uint8_t *data, int len)
{
    uint8_t tbuf[1];

    tbuf[0] = len;
    lp_fifo_write(p_fifo, tbuf, 1);
    return lp_fifo_write(p_fifo, data, len); // fifo full
}
//

/**
 * \brief  获取数据(第一个字节是后面数据的长度)
 * @param  p_fifo I FIFO的结构体
 * @param  out O 数据输出地址
 * @param  max_out I 要出FIFO的个数
 * @retval 1成功 0失败
 */
int lp_fifo_msg_get(lp_fifo_t *p_fifo, uint8_t *out, int max_out)
{
    uint8_t len;
    int dlen;

    dlen = lp_fifo_peek(p_fifo, &len);
    if (dlen > len)
    {
        lp_fifo_read(p_fifo, out, 1);
        return lp_fifo_read(p_fifo, out, len);
    }
    return 1;
}
/**
 * \brief  清空fifo
 * @param  p_fifo I FIFO的结构体
 * @retval 1成功 0失败
 */
int lp_fifo_clear(lp_fifo_t *p_fifo)
{
    p_fifo->fifo_bot = 0;
    p_fifo->fifo_top = 0;
    p_fifo->fifo_size = p_fifo->fifo_len;
    return 0;
}
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////// time //////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////
/**
 * 判断一年是否为闰年
 * @param year a year
 * @return 0: not leap year; 1: leap year
 */
uint8_t lp_time_is_leap_year(uint32_t year)
{
    return (year % 4) || ((year % 100 == 0) && (year % 400)) ? 0 : 1;
}

/**
 * Get the number of days in a month
 * @param year a year
 * @param month a month. The range is basically [1..12] but [-11..0] or [13..24] is also
 *              supported to handle next/prev. year
 * @return [28..31]
 */
uint8_t lp_time_month_length_get(int32_t year, int32_t month)
{
    month--;
    if (month < 0)
    {
        year--;             /*Already in the previous year (won't be less then -12 to skip a whole year)*/
        month = 12 + month; /*`month` is negative, the result will be < 12*/
    }
    if (month >= 12)
    {
        year++;
        month -= 12;
    }

    /*month == 1 is february*/
    return (month == 1) ? (28 + lp_time_is_leap_year(year)) : 31 - month % 7 % 2;
}

/**
 * 这天是星期几
 * @param year a year
 * @param month a  month [1..12]
 * @param day a day [1..32]
 * @return [0..6] which means [Sun..Sat] or [Mon..Sun] depending on 日历周从周一开始
 */
uint8_t lp_time_day_of_week_get(uint32_t year, uint32_t month, uint32_t day)
{
    uint32_t a = month < 3 ? 1 : 0;
    uint32_t b = year - a;

#if 0 // 日历周从周一开始
    uint32_t day_of_week = (day + (31 * (month - 2 + 12 * a) / 12) + b + (b / 4) - (b / 100) + (b / 400) - 1) % 7;
#else
    uint32_t day_of_week = (day + (31 * (month - 2 + 12 * a) / 12) + b + (b / 4) - (b / 100) + (b / 400)) % 7;
#endif

    return day_of_week;
}
#define RTC_TIME_START_YEAR 1970
const uint8_t mon_table[12] = {31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31};
/**
 * utc转time时间
 * @param rtc I  utc时间
 * @param tm O time时间
 */
void lp_rtc_to_time(unsigned int rtc, datetime_t *tm)
{
    uint64_t temp = 0;
    uint16_t temp1 = 0;

    temp = rtc / 86400; // 得到天数(秒钟数对应的)

    temp1 = RTC_TIME_START_YEAR; // 1970年开始
    while (temp >= 365)
    {
        if (lp_time_is_leap_year(temp1)) // 是闰年
        {
            if (temp >= 366)
                temp -= 366; // 闰年的秒钟数
            else
            {
                temp1++;
                break;
            }
        }
        else
            temp -= 365; // 平年
        temp1++;
        if (temp1 > 2099)
            return; // 大于2099年，2022年11月20日  bug
    }
    tm->tm_year = temp1; // 得到年份
    temp1 = 0;
    while (temp >= 28) // 超过了一个月
    {
        if (lp_time_is_leap_year(tm->tm_year) && temp1 == 1) // 当年是不是闰�?/2月份
        {
            if (temp >= 29)
                temp -= 29; // 闰年的秒钟数
            else
                break;
        }
        else
        {
            if (temp >= mon_table[temp1])
                temp -= mon_table[temp1]; // 平年
            else
                break;
        }
        temp1++;
    }
    tm->tm_mon = temp1 + 1;          // 得到月份
    tm->tm_mday = temp + 1;          // 得到日期
    temp = rtc % 86400;              // 得到秒钟�?
    tm->tm_hour = temp / 3600;       // 小时
    tm->tm_min = (temp % 3600) / 60; // 分钟
    tm->tm_sec = (temp % 3600) % 60; // 秒钟
}
/**
 *  time时间转utc
 * @param tm I time时间
 * @return 0: utc时间
 */
unsigned int lp_time_to_rtc(datetime_t *tm)
{
    uint32_t seccount = 0; // 一定要初始
    uint16_t t;

    if (tm->tm_year < RTC_TIME_START_YEAR || tm->tm_year > 2099)
        return 0;

    for (t = RTC_TIME_START_YEAR; t < tm->tm_year; t++)
    {
        if (lp_time_is_leap_year(t))
            seccount += 31622400;
        else
            seccount += 31536000;
    }
    tm->tm_mon -= 1;
    for (t = 0; t < tm->tm_mon; t++)
    {
        seccount += (uint32_t)mon_table[t] * 86400;
        if (lp_time_is_leap_year(tm->tm_year) && t == 1)
            seccount += 86400;
    }
    seccount += (uint32_t)(tm->tm_mday - 1) * 86400;
    seccount += (uint32_t)tm->tm_hour * 3600;
    seccount += (uint32_t)tm->tm_min * 60; // 分钟秒钟�?
    seccount += tm->tm_sec;
    return seccount;
}
////////////////////////////////////////////////////////////////
uint32_t lp_time_utc_get(void)
{
    return lpbsp_rtc_get();
}
int lp_time_utc_set(uint32_t utc)
{
    if (utc > 1678723200)
        lpbsp_rtc_set(utc);
    return 0;
}
int lp_time_get(datetime_t *dt)
{
    time_t rawtime = lp_time_utc_get();

    lp_rtc_to_time(rawtime, dt);
    return 0;
}
int lp_time_set(datetime_t *dt)
{
    uint32_t utc = lp_time_to_rtc((struct tm_t *)dt);
    lp_time_utc_set(utc);
    return 0;
}
// utc 2 2023-10-28 12-12-12   data-must 20byte
void lp_time_utc2string(uint32_t utc, char *data, int type)
{
    datetime_t dt;

    lp_rtc_to_time(utc, &dt);
    if (0 == type) //   2023-10-28 12:12:12
        sprintf(data, "%04d-%02d-%02d %02d:%02d:%02d", dt.tm_year, dt.tm_mon, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
    else if (1 == type) //   2023-10-28 121212
        sprintf(data, "%04d-%02d-%02d %02d:%02d:%02d", dt.tm_year, dt.tm_mon, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
    else if (2 == type) // 20231028121212
        sprintf(data, "%04d%02d%02d%02d%02d%02d", dt.tm_year, dt.tm_mon, dt.tm_mday, dt.tm_hour, dt.tm_min, dt.tm_sec);
}
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
int lp_bat_value_get(void)
{
    static int last_value = 0;
    int value;
#if 0
    return lpbsp_bat_get_mv();
#else
    value = lpbsp_bat_get_mv(); // 4150-3550=600;
    if (0 == last_value)
        last_value = value;
    // LP_LOGD("bat mv=%dmv\r\n", value);
    if (lpbsp_charge_get())
    {
        if (value > last_value)
            last_value = (last_value + value) / 2;
    }
    else
    {
        if (value > last_value)
        {
        }
        else if ((value + 20) < last_value)
        {
            last_value = (last_value + value) / 2;
        }
        else
        {
            last_value = last_value - 10;
        }
    }

    if (last_value > 4100)
        return 100;
    if (last_value < 3600)
        return 0;
    return (last_value - 3600) / 5;
#endif
}

int32_t lplib_qma6100_writereg(uint8_t reg_add, uint8_t reg_dat)
{
    uint8_t tbuf[3];
    tbuf[0] = 0x12;
    tbuf[1] = reg_add;
    tbuf[2] = reg_dat;
    lpbsp_iic_write(0, tbuf, 3);
    return 0;
}
int32_t lplib_qma6100_readreg(uint8_t reg_add, uint8_t *bufs, int num)
{
    uint8_t tbuf[24];
    int ret = 0;
    tbuf[0] = 0x12;
    tbuf[1] = reg_add;
    ret = lpbsp_iic_read(0, tbuf, num);
    memcpy(bufs, tbuf, num);
    return ret;
}
void lplib_qma6100_clear_step(void)
{
    lplib_qma6100_writereg(0x13, 0x80); // clear step
    lpbsp_delayms(10);
    lplib_qma6100_writereg(0x13, 0x80); // clear step
    lpbsp_delayms(10);
    lplib_qma6100_writereg(0x13, 0x80); // clear step
    lpbsp_delayms(10);
    lplib_qma6100_writereg(0x13, 0x80); // clear step
    lplib_qma6100_writereg(0x13, 0x7f); // clear step
}
uint32_t lplib_qma6100_read_stepcounter(void)
{
    uint8_t data[4] = {0}, ret;
    uint32_t step_num;

    ret = lplib_qma6100_readreg(0x07, data, 2);
    if (ret != 0)
    {
        return 0;
    }

    step_num = (uint32_t)(((uint32_t)data[1] << 8) | data[0]);

    return step_num;
}
/// 读取步数
int lp_sensor_accelerometer_step_read(void)
{
    static uint32_t step = 0;
    static uint32_t last_tick = 0;
    uint32_t temp = 0, dt = 0;
    uint8_t tbuf[12];

    dt = lpbsp_rtc_get();
    temp = lplib_qma6100_read_stepcounter();
    if (dt > (last_tick + 60))
    {
        tbuf[0] = 'S';
        tbuf[1] = temp;
        tbuf[2] = temp >> 8;
        lplib_qma6100_clear_step();
        last_tick = dt;
        step += temp;
        lplib_rtc_ram_add(tbuf, 3);
        return step;
    }

    last_tick = dt;
    return temp + step;
}
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////FAT//////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////////////////////////////
static lp_fat_drive_t *lp_fat_drives[LF_DRIVER_NUM] = {0};
static int lp_fat_drives_num = 0; // 注册了多少个驱动
// 驱动信息
void lp_fat_driver_info(void)
{
    int i = 0;
    for (i = 0; i < lp_fat_drives_num; i++)
    {
        if (lp_fat_drives[i]->letter != 0)
        {
            LP_LOGD("%02d,lf[%c]\r\n", i, lp_fat_drives[i]->letter);
        }
    }
}

static int lp_mount_auto(int num, const char *driver_name)
{
    int ret = 0;

    LP_LOGD("auto mount %d %s\r\n", num, driver_name);
    lp_fat_drives[num]->is_mount = 1;
    if (0 == lp_fat_drives[num]->mount_cb)
    {
        ret = lpbsp_fs_mount(driver_name);
        if (0 != ret)
            return -2;
    }
    else
    {
        ret = lp_fat_drives[num]->mount_cb(driver_name);
        if (0 != ret)
            return -3;
    }

    return 0;
}
// 判断是那个盘的
int lp_fat_driver_num_get(const char *filename)
{
    int i = 0;
    int len = 0;

    for (i = 0; i < lp_fat_drives_num; i++)
    {
        if (lp_fat_drives[i]->letter == filename[1])
        {
            if (0 == lp_fat_drives[i]->is_mount)
            {
                len = lp_mount_auto(i, filename);
                if (0 != len)
                    return -2;
            }
            return i;
        }
        else if (0 == lp_fat_drives[i]->letter) // 这是非文件类的
        {
            len = strlen(lp_fat_drives[i]->name);
            if (0 == memcmp(lp_fat_drives[i]->name, filename, len))
            {
                if (0 == lp_fat_drives[i]->is_mount)
                {
                    len = lp_mount_auto(i, filename);
                    if (0 != len)
                        return -3;
                }
                return i;
            }
        }
    }
    LP_LOGD("lp_fat_driver_num_get find no [%s]\r\n", filename);
    return -1;
}
// 多小个盘
int lp_fat_driver_nums(void)
{
    return lp_fat_drives_num;
}

// 自定义数据
void *lp_fat_driver_user_data(uint32_t drive_num, int num)
{
    if (LF_DRIVER_NUM < (1 + drive_num))
        return 0;
    if (1 == num)
        return lp_fat_drives[drive_num]->user_data1;
    else if (2 == num)
        return lp_fat_drives[drive_num]->user_data2;
    return 0;
}

/// @brief 注册驱动
/// @param lp_fat_drive 驱动
/// @param lp_fat_drive_size 驱动大小
/// @return 返回码
int lp_fat_driver_init(lp_fat_drive_t *lp_fat_drive, int lp_fat_drive_size)
{
    int i = 0;

    if (0 == lp_fat_drive || 0 == lp_fat_drive_size)
        return LF_FILE_NO_DRIVER_NUM;

    for (i = 0; i < LF_DRIVER_NUM; i++)
    {
        if (0 == lp_fat_drives[i]) // 找到空闲的
        {
            lp_fat_drives[i] = lpbsp_malloc(lp_fat_drive_size);
            if (0 == lp_fat_drives[i])
            {
                return LF_ERR_MALLOC;
            }

            lp_fat_drives_num++;
            memcpy(lp_fat_drives[i], lp_fat_drive, lp_fat_drive_size);
            return 0;
        }
    }
    return 1; // 没有位置了
}

// 格式化
int lp_format(const char *driver_name)
{
    int num = lp_fat_driver_num_get(driver_name);
    if (0 > num)
    {
        return LF_FILE_NO_DRIVER_NUM;
    }
    if (0 == lp_fat_drives[num]->format_cb)
        return lpbsp_fs_format(driver_name);

    return lp_fat_drives[num]->format_cb(driver_name);
}

int lp_mount(const char *driver_name)
{
    int num = lp_fat_driver_num_get(driver_name);
    if (0 > num)
    {
        return LF_FILE_NO_DRIVER_NUM;
    }
    return 0;
}
int lp_unmount(const char *driver_name)
{
    int num = lp_fat_driver_num_get(driver_name);
    if (0 > num)
    {
        return LF_FILE_NO_DRIVER_NUM;
    }
    if (0 == lp_fat_drives[num]->is_mount)
        return 0;
    if (0 == lp_fat_drives[num]->unmount_cb)
        return lpbsp_fs_unmount(driver_name);
    return lp_fat_drives[num]->unmount_cb(driver_name);
}

int lp_ferror(lp_file_t *fp)
{
    return fp->err_code;
}
// 盘符区分,格式/A/path//
lp_file_t *lp_open(const char *filename, const char *mode)
{
    char tbuf[64];
    lp_file_t *p;
    int num = 0;

    if (0 == filename)
        return 0;
    num = lp_fat_driver_num_get(filename);
    if (0 > num)
    {
        return 0;
    }
    if (0 == lp_fat_drives[num]->open_cb)
        return 0;
    strcpy(tbuf, filename);
    lp_string_replace_char(tbuf, '\\', '/');
    p = lp_fat_drives[num]->open_cb(tbuf, mode);
    if (p) // 这地址一定得大于1000
    {
        lp_file_t *fd = lpbsp_malloc(sizeof(lp_file_t));
        if (fd)
        {
            fd->file = p;
            fd->driver_num = num;
            return fd;
        }
        else
        {
            lpbsp_free(p);
            return 0;
        }
    }
    else
    {
        LP_LOGD("open err [%s]\r\n", filename);
        return 0;
    }
    return p; // 返回少于1000的 没有lp_read 和 lp_write lp_close 函数的
}
int lp_read(void *ptr, int size, int nmemb, lp_file_t *stream)
{
    if (0 == stream)
        return 0;
    if (0 == lp_fat_drives[stream->driver_num]->read_cb)
    {
        stream->err_code = LF_FILE_NO_FUNC_CB;
        return 0;
    }
    return lp_fat_drives[stream->driver_num]->read_cb(ptr, size, nmemb, stream->file);
}
int lp_write(const void *ptr, int size, int nmemb, lp_file_t *stream)
{
    if (0 == stream)
        return 0;
    if (0 == lp_fat_drives[stream->driver_num]->write_cb)
    {
        stream->err_code = LF_FILE_NO_FUNC_CB;
        return 0;
    }
    return lp_fat_drives[stream->driver_num]->write_cb(ptr, size, nmemb, stream->file);
}
int lp_close(lp_file_t *stream)
{
    if (0 == stream)
        return 0;
    if (0 == lp_fat_drives[stream->driver_num]->close_cb)
        return LF_FILE_NO_FUNC_CB;
    int ret = lp_fat_drives[stream->driver_num]->close_cb(stream->file);
    lpbsp_free(stream);
    return ret;
}

int lp_append(const char *filename, const void *ptr, int size)
{
    int len = 0, ret = 0;
    lp_file_t *fd = lp_open(filename, "ab");
    if (0 == fd)
        return LF_ERR_OPEN_FILE;
    len = lp_write(ptr, 1, size, fd);

    if (size != len)
    {
        LP_LOGD("append err[%d\r\n]", len);
        lp_close(fd);
        return LF_ERR_OPEN_FILE;
    }

    ret = lp_close(fd);
    if (ret != 0)
        return 0;
    return len;
}
int lp_unlink(const char *filename)
{
    int num = 0;
    if (0 == filename)
        return -9;
    num = lp_fat_driver_num_get(filename); // 找出是哪个驱动
    if (0 > num)
    {
        return LF_ERR_PARAMETER;
    }
    if (lp_fat_drives[num]->unlink_cb)
        return lp_fat_drives[num]->unlink_cb(filename);
    else
        return LF_FILE_NO_FUNC_CB;
}
int lp_fsize(lp_file_t *fp)
{
    int size, temp = 0;

    temp = lp_tell(fp);
    lp_seek(fp, 0L, SEEK_END);
    size = lp_tell(fp);
    lp_seek(fp, temp, SEEK_SET);
    return size;
}
int lp_tell(lp_file_t *stream)
{
    if (0 == stream)
        return -9;
    if (lp_fat_drives[stream->driver_num]->tell_cb)
        return lp_fat_drives[stream->driver_num]->tell_cb(stream->file);
    else
        return LF_FILE_NO_FUNC_CB;
}

int lp_seek(lp_file_t *stream, int offset, int fromwhere)
{
    if (0 == stream)
        return -9;
    if (lp_fat_drives[stream->driver_num]->seek_cb)
        return lp_fat_drives[stream->driver_num]->seek_cb(stream->file, offset, fromwhere);
    else
        return LF_FILE_NO_FUNC_CB;
}
int lp_rewind(lp_file_t *stream)
{
    return lp_seek(stream, 0L, SEEK_SET);
}
int lp_rename(const char *oldpath, const char *newpath)
{
    int num = 0;
    if (0 == oldpath || 0 == newpath)
        return -9;

    num = lp_fat_driver_num_get(oldpath); // 找出是哪个驱动
    if (0 > num)
    {
        return 0;
    }
    return lp_fat_drives[num]->rename_cb(oldpath, newpath);
}
int lp_stat(const char *path, lp_info_t *info)
{
    int driver_num = 0;

    if (0 == path || 0 == info)
        return LF_ERR_PARAMETER_NULL;

    driver_num = lp_fat_driver_num_get(path); // 找出是哪个驱动
    if (0 > driver_num)
        return LF_FILE_NO_DRIVER_NUM;

    return lp_fat_drives[driver_num]->stat_cb(path, info);
}

int lp_file_exist(const char *path)
{
    lp_file_t *fd;

    fd = lp_open(path, "r");
    if (fd)
    {
        lp_close(fd);
        return 0;
    }
    return 1;
}
int lp_mkdir(const char *newdir)
{
    char tbuf[128];
    int driver_num = 0;

    if (0 == newdir)
        return -9;

    driver_num = lp_fat_driver_num_get(newdir); // 找出是哪个驱动
    if (0 > driver_num)
    {
        return 0;
    }
    strcpy(tbuf, newdir);
    lp_string_replace_char(tbuf, '\\', '/');
    return lp_fat_drives[driver_num]->mkdir_cb(tbuf);
}
int lp_rmdir(const char *newdir)
{

    return 0;
}
int lp_filesize(const char *path)
{
    int ret = 0, size = 0;
    lp_file_t *file;
    file = lp_open(path, "rb");
    if (0 == file)
        return 0;

    ret = lp_seek(file, 0L, SEEK_END);
    if (0 != ret)
    {
        LP_LOGD("lp_seek file error %d\r\n", ret);
    }
    size = lp_tell(file);

    ret = lp_close(file);
    if (0 != ret)
    {
        LP_LOGD("close file error\r\n");
    }

    return size;
}
// 写入一行内容到文件

int lp_puts(uint8_t *buf, lp_file_t *stream)
{
    if (0 == stream || 0 == buf)
        return -9;
    return lp_write(buf, 1, strlen((const char *)buf), stream);
}
// 读取一行文

int lp_gets(lp_file_t *stream, uint8_t *buf, uint32_t size)
{
    int ret;
    if (0 == stream || 0 == buf || 0 == size)
        return -9;
    size--;
    buf[size] = 0;
    for (int i = 0; i < size; i++)
    {
        ret = lp_read(&buf[i], 1, 1, stream);
        if (ret != 1)
        {
            buf[i] = 0;
            return 0;
        }
        if (buf[i] == '\n')
        {
            buf[i] = 0;
            return i;
        }
    }
    return 1;
}
int lp_read_uint16(lp_file_t *stream, uint16_t *i)
{
    uint8_t data[2];
    if (0 == stream || 0 == i)
        return -9;
    int ret = lp_read(data, 1, 2, stream);
    i[0] = (uint16_t)(data[0] | (data[1] << 8));
    return ret;
}
int lp_write_uint16(lp_file_t *stream, uint16_t i)
{
    uint8_t data[2] = {0};
    if (0 == stream)
        return 9;
    data[0] = (uint8_t)i;
    data[1] = i >> 8;
    return lp_write(data, 1, 2, stream);
}

lfdir_t *lp_opendir(const char *path)
{
    lfdir_t *dir = 0;
    void *p;
    int num = 0;

    if (0 == path)
    {
        LP_LOGD("dir null\r\n");
        return 0;
    }
    num = lp_fat_driver_num_get(path); // 找出是哪个驱动
    if (0 > num)
        return 0;
    if (0 == lp_fat_drives[num]->dir_open_cb)
        return 0;
    p = lp_fat_drives[num]->dir_open_cb(path);
    if (p)
    {
        dir = lpbsp_malloc(sizeof(lfdir_t));
        if (dir)
        {
            dir->driver_num = num;
            dir->dir = p;
            return dir;
        }
        return 0;
    }
    return 0;
}
lp_dir_t *lp_readdir(lfdir_t *dir)
{
    if (0 == dir)
    {
        LP_LOGD("dir null\r\n");
        return 0;
    }
    if (0 == lp_fat_drives[dir->driver_num]->dir_read_cb)
        return 0;
#if 0
    return lp_fat_drives[dir->driver_num]->dir_read_cb(dir);
#else
    lp_dir_t *tdir = 0;

    tdir = lp_fat_drives[dir->driver_num]->dir_read_cb(dir);
    if (tdir && tdir->type == LF_TYPE_DIR && 0 == strcmp("LosHide", tdir->name))
    {
        return lp_fat_drives[dir->driver_num]->dir_read_cb(dir);
    }
    return tdir;
#endif
}
int lp_closedir(lfdir_t *dir)
{
    int ret = 0;

    if (0 == dir)
        return LF_ERR_PARAMETER_NULL;
    if (0 == lp_fat_drives[dir->driver_num]->dir_close_cb)
        return 0;
    ret = lp_fat_drives[dir->driver_num]->dir_close_cb(dir->dir);
    lpbsp_free(dir);
    return ret;
}
// 容量
int lp_fs_volume(char *driver, int *total, int *free_bs)
{
    int num = 0;

    if (0 == driver || 0 == total || 0 == free_bs)
        return LF_ERR_PARAMETER_NULL;

    *total = 0;
    *free_bs = 0;

    num = lp_fat_driver_num_get(driver); // 找出是哪个驱动
    if (0 > num)
        return LF_FILE_NO_DRIVER_NUM;
    if (0 == lp_fat_drives[num]->volume_cb)
        return lpbsp_fs_volume(driver, total, free_bs);

    return lp_fat_drives[num]->volume_cb(driver, total, free_bs);
}
/// @brief lp_filelist的下级函数
/// @param root_len 根路径长度
/// @param panme 待遍历路径
/// @param lp_file_list_func_cb 处理回调函数
/// @param pOpaque 自定义参数
/// @return
static int lp_filelist2(int root_len, char *panme, int (*lp_file_list_func)(void *pOpaque, int root_len, const char *pFilename, int type), void *pOpaque)
{
    lp_dir_t *dirp;
    lfdir_t *dir;
    char file_path[128] = {0};

    dir = lp_opendir(panme);
    if (0 == dir)
    {
        LP_LOGD("open dir=%s error\r\n", panme);
        return 1;
    }
    while (1)
    {
        dirp = lp_readdir(dir);
        if (0 == dirp)
            break;

        if (LF_TYPE_NO == dirp->type || dirp->name[0] == '.')
            continue;

        snprintf((char *)file_path, 127, (char const *)"%s/%s", panme, dirp->name);
        if (dirp->type == LF_TYPE_DIR)
        {
            lp_file_list_func(pOpaque, root_len, file_path, LF_TYPE_DIR);
            lp_filelist2(root_len, file_path, lp_file_list_func, pOpaque);
        }
        else
        {
            lp_file_list_func(pOpaque, root_len, file_path, LF_TYPE_FILE);
        }
    }
    lp_closedir(dir);
    return 0;
}

/// @brief 遍历文件
/// @param panme  文件夹路径
/// @param lp_file_list_func_cb 回调处理函数
/// @param pOpaque 自定义参数
/// @return
int lp_filelist(char *panme, int (*lp_file_list_func)(void *pOpaque, int root_len, const char *pFilename, int type), void *pOpaque)
{
    lp_dir_t *dirp;
    lfdir_t *dir;
    char file_path[128] = {0};
    int root_len;

    if (0 == lp_file_list_func)
    {
        return 1;
    }

    root_len = strlen(panme);
    dir = lp_opendir(panme);
    if (0 == dir)
    {
        LP_LOGD("open dir=%s error\r\n", panme);
        return 1;
    }
    while (1)
    {
        dirp = lp_readdir(dir);
        if (0 == dirp)
            break;

        if (LF_TYPE_NO == dirp->type || dirp->name[0] == '.')
            continue;

        snprintf((char *)file_path, 127, (char const *)"%s/%s", panme, dirp->name);
        if (dirp->type == LF_TYPE_DIR)
        {
            lp_file_list_func(pOpaque, root_len, file_path, LF_TYPE_DIR);
            lp_filelist2(root_len, file_path, lp_file_list_func, pOpaque);
        }
        else
        {
            lp_file_list_func(pOpaque, root_len, file_path, LF_TYPE_FILE);
        }
    }
    lp_closedir(dir);
    return 0;
}

// 遍历文件
int lp_filelist_printf(char *panme)
{
    lp_dir_t *dirp;
    lfdir_t *dir;
    char file_path[128] = {0};

    dir = lp_opendir(panme);
    if (0 == dir)
    {
        LP_LOGD("open dir=%s error\r\n", panme);
        return 1;
    }
    LP_LOGD("open dir=%s\r\n", panme);
    while (1)
    {
        dirp = lp_readdir(dir);
        if (0 == dirp)
            break;

        if (LF_TYPE_NO == dirp->type || dirp->name[0] == '.')
            continue;
        snprintf((char *)file_path, 127, (char const *)"%s/%s", panme, dirp->name);
        if (dirp->type == LF_TYPE_DIR)
        {
            lp_filelist_printf(file_path);
        }
        else
        {
            LP_LOGD("\tfile[%s] size[%d]\r\n", file_path, lp_filesize(file_path));
        }
    }
    lp_closedir(dir);
    LP_LOGD("close dir\n\n");
    return 0;
}

int lp_log(lp_file_t *fd, const char *fmt, ...)
{
    va_list paramList;
    char tbuf[64], tlen;
    if (0 == fd)
        return -9;
    va_start(paramList, fmt);
    vsprintf(tbuf, fmt, paramList);
    va_end(paramList);
    tlen = strlen(tbuf);
    lp_write(tbuf, 1, tlen, fd);
    return tlen;
}

/// @brief 先判断有没有路径，没有就新建
/// @param dir 路径
/// @return 0 ok
int lp_mkdir_try(char *dir)
{
    int ret;
    lp_info_t lp_info;

    ret = lp_stat(dir, &lp_info);
    LP_LOGD("lp_stat ret [%s] %d\r\n", dir, ret);
    if (LF_OK != ret)
    {
        ret = lp_mkdir(dir);
        return ret;
    }
    return 0;
}
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////

// 读取文件
//  buf_path：路径
//  len：长度
// 返回：数据
uint8_t *lp_readall(char *buf_path, uint32_t *len)
{
    uint32_t rlen = 0;
    uint8_t *buf = 0;
    lp_file_t *file;
    int ret = 0;

    if (0 == buf_path)
        return 0;

    file = lp_open(buf_path, "rb");
    if (0 == file)
    {
        LP_LOGD("open err %s\r\n", buf_path);
        return NULL; // err
    }
    rlen = lp_fsize(file); // 文件长度
    if (0 == rlen)
    {
        LP_LOGD("size zero\r\n");
        lp_close(file); // 关闭
        return NULL;
    }
    buf = lpbsp_malloc(rlen + 1);
    if (0 == buf)
    {
        LP_LOGD("mallor err\r\n");
        lp_close(file); // 关闭
        return NULL;    // err
    }

    buf[rlen] = 0;

    ret = lp_read(buf, 1, rlen, file); // 读取
    if (ret != rlen)
    {
        LP_LOGD("ret %d len %d\r\n", ret, rlen);
        lpbsp_free(buf);
        lp_close(file); // 关闭
        return NULL;    // err
    }
    if (len)
        *len = rlen;

    lp_close(file); // 关闭
    return buf;
}
// 写文件
//  path：路径
//  data:数据
//  len：长度
// 返回：0 OK
uint32_t lp_save(char *path, uint8_t *data, uint32_t len)
{
    int32_t ret;
    lp_file_t *file;

    LP_LOGD("lp_save %s\r\n", path);
    file = lp_open(path, "wb+");
    if (0 == file)
    {
        LP_LOGD("lp_open  fail %s\r\n", path);
        return -1;
    }
    ret = lp_write(data, 1, len, file);
    if (ret != len)
    {
        LP_LOGD("lp_write err [%d]\r\n", ret);
        lp_close(file);
        return -2;
    }
    ret = lp_close(file);
    LP_LOGD("lp_close %d\r\n", ret);
    return ret;
}

// 复制文件
// 返回：0 OK
int lp_copy(char *src, char *dec)
{
    int ret = 0;
    lp_file_t *fsrc, *fdec;
    uint8_t temp[4096];
    fsrc = lp_open(src, "rb+");
    if (0 == fsrc)
        return -1;
    fdec = lp_open(dec, "wb+");
    if (0 == fdec)
    {
        lp_close(fsrc);
        return -2;
    }
    while (1)
    {
        ret = lp_read(temp, 1, 4096, fsrc);
        ret = lp_write(temp, 1, ret, fdec);
        if (ret != 4096)
            break;
    }
    ret = lp_close(fsrc);
    ret |= lp_close(fdec);
    return ret;
}

/// @brief 复制文件到文件夹
/// @param src 文件
/// @param dec 文件夹
/// @return 0 ok
int lp_copy_file_2_dir(char *src, char *dec)
{
    char tbuf[64];
    int ret = 1, slen, dlen = 0, i = 0;

    slen = strlen(src);
    dlen = strlen(dec);
    strcpy(tbuf, dec);
    for (i = slen; i > 1; i--)
    {
        if ('/' == src[i])
        {
            strcpy(&tbuf[dlen], &src[i]);
            // LP_LOGD("src[%s] dec[%s] new[%s]\r\n", src, dec, tbuf);
            ret = lp_copy(src, tbuf);
            return ret;
        }
    }
    return ret;
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/**
 * Open a file
 * @param drv pointer to a driver where this function belongs
 * @param path path to the file beginning with the driver letter (e.g. S:/folder/file.txt)
 * @param mode read: FS_MODE_RD, write: FS_MODE_WR, both: FS_MODE_RD | FS_MODE_WR
 * @return pointer to FIL struct or NULL in case of fail
 */
static void *fs_open(lv_fs_drv_t *drv, const char *path, lv_fs_mode_t mode)
{
    LV_UNUSED(drv);

    const char *flags = "";

    if (mode == LV_FS_MODE_WR)
        flags = "wb";
    else if (mode == LV_FS_MODE_RD)
        flags = "rb";
    else if (mode == (LV_FS_MODE_WR | LV_FS_MODE_RD))
        flags = "rb+";

    return lp_open(path, flags);
}

/**
 * Close an opened file
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to a FIL variable. (opened with fs_open)
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_close(lv_fs_drv_t *drv, void *file_p)
{
    LV_UNUSED(drv);
    lp_close(file_p);
    return LV_FS_RES_OK;
}

/**
 * Read data from an opened file
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to a FIL variable.
 * @param buf pointer to a memory block where to store the read data
 * @param btr number of Bytes To Read
 * @param br the real number of read bytes (Byte Read)
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_read(lv_fs_drv_t *drv, void *file_p, void *buf, uint32_t btr, uint32_t *br)
{
    LV_UNUSED(drv);
    int rlen;
    rlen = lp_read(buf, 1, btr, file_p);

    if (rlen > 0)
    {
        *br = rlen;
        return LV_FS_RES_OK;
    }
    return LV_FS_RES_UNKNOWN;
}

/**
 * Write into a file
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to a FIL variable
 * @param buf pointer to a buffer with the bytes to write
 * @param btw Bytes To Write
 * @param bw the number of real written bytes (Bytes Written). NULL if unused.
 * @return LV_FS_RES_OK or any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_write(lv_fs_drv_t *drv, void *file_p, const void *buf, uint32_t btw, uint32_t *bw)
{
    LV_UNUSED(drv);
    int rlen;
    rlen = lp_write(buf, 1, btw, file_p);
    if (rlen > 0)
    {
        *bw = rlen;
        return LV_FS_RES_OK;
    }
    return LV_FS_RES_UNKNOWN;
}

/**
 * Set the read write pointer. Also expand the file size if necessary.
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to a FIL variable. (opened with fs_open )
 * @param pos the new position of read write pointer
 * @param whence only LV_SEEK_SET is supported
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_seek(lv_fs_drv_t *drv, void *file_p, uint32_t pos, lv_fs_whence_t whence)
{
    LV_UNUSED(drv);
    switch (whence)
    {
    case LV_FS_SEEK_SET:
        lp_seek(file_p, pos, 0);
        break;
    case LV_FS_SEEK_CUR:
        lp_seek(file_p, pos, 1);
        break;
    case LV_FS_SEEK_END:
        lp_seek(file_p, pos, 2);
        break;
    default:
        break;
    }
    return LV_FS_RES_OK;
}

/**
 * Give the position of the read write pointer
 * @param drv pointer to a driver where this function belongs
 * @param file_p pointer to a FIL variable.
 * @param pos_p pointer to to store the result
 * @return LV_FS_RES_OK: no error, the file is read
 *         any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_tell(lv_fs_drv_t *drv, void *file_p, uint32_t *pos_p)
{
    LV_UNUSED(drv);
    *pos_p = lp_tell(file_p);
    return LV_FS_RES_OK;
}

/**
 * Initialize a 'DIR' variable for directory reading
 * @param drv pointer to a driver where this function belongs
 * @param path path to a directory
 * @return pointer to an initialized 'DIR' variable
 */
static void *fs_dir_open(lv_fs_drv_t *drv, const char *path)
{
    LV_UNUSED(drv);

    return lp_opendir(path);
}

/**
 * Read the next filename from a directory.
 * The name of the directories will begin with '/'
 * @param drv pointer to a driver where this function belongs
 * @param dir_p pointer to an initialized 'DIR' variable
 * @param fn pointer to a buffer to store the filename
 * @return LV_FS_RES_OK or any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_dir_read(lv_fs_drv_t *drv, void *dir_p, char *fn)
{
    LV_UNUSED(drv);
    lp_dir_t *lp_dir;
    fn[0] = '\0';

    do
    {
        lp_dir = lp_readdir(dir_p);
        if (0 == lp_dir)
            return LV_FS_RES_UNKNOWN;

        if (2 == lp_dir->type)
        {
            fn[0] = '/';
            strcpy(&fn[1], lp_dir->name);
        }
        else
            strcpy(fn, lp_dir->name);

    } while (strcmp(fn, "/.") == 0 || strcmp(fn, "/..") == 0);

    return LV_FS_RES_OK;
}

/**
 * Close the directory reading
 * @param drv pointer to a driver where this function belongs
 * @param dir_p pointer to an initialized 'DIR' variable
 * @return LV_FS_RES_OK or any error from lv_fs_res_t enum
 */
static lv_fs_res_t fs_dir_close(lv_fs_drv_t *drv, void *dir_p)
{
    LV_UNUSED(drv);
    lp_closedir(dir_p);
    return LV_FS_RES_OK;
}

/**
 * Register a driver for the File system interface
 */
static void lp_lvgl_fs_init(void)
{
    /*---------------------------------------------------
     * Register the file system interface in LVGL
     *--------------------------------------------------*/

    /*Add a simple drive to open images*/
    static lv_fs_drv_t fs_drv; /*A driver descriptor*/
    lv_fs_drv_init(&fs_drv);

    /*
    lv_fs_drv_t * lv_fs_get_drv(char letter)
    {
        lv_fs_drv_t ** drv;

        _LV_LL_READ(&LV_GC_ROOT(_lv_fsdrv_ll), drv) {
            // if((*drv)->letter == letter) {
                return *drv;
            // }
        }

        return NULL;
    }

    static const char * lv_fs_get_real_path(const char * path)
    {
        //path++;
        //if(*path == ':') path++;

        return path;
    }
    */
    fs_drv.letter = 'C'; // 这设置无意义 需要在 lv_fs_get_drv 中直接返回 *drv
    fs_drv.cache_size = 0;

    fs_drv.open_cb = fs_open;
    fs_drv.close_cb = fs_close;
    fs_drv.read_cb = fs_read;
    fs_drv.write_cb = fs_write;
    fs_drv.seek_cb = fs_seek;
    fs_drv.tell_cb = fs_tell;

    fs_drv.dir_close_cb = fs_dir_close;
    fs_drv.dir_open_cb = fs_dir_open;
    fs_drv.dir_read_cb = fs_dir_read;

    lv_fs_drv_register(&fs_drv);
}
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////
/*  以下是文件系统的驱动 */
#if defined(CONFIG_LP_USE_FS_WIN32) || defined(WIN32)

#include <windows.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#define MAX_PATH_LEN 256

static bool is_dots_name(const char *name)
{
    return name[0] == '.' && (!name[1] || (name[1] == '.' && !name[2]));
}

static WCHAR *mz_utf8z_to_widechar(const char *str)
{
    int reqChars = MultiByteToWideChar(CP_UTF8, 0, str, -1, NULL, 0);
    WCHAR *wStr = (WCHAR *)malloc(reqChars * sizeof(WCHAR));
    MultiByteToWideChar(CP_UTF8, 0, str, -1, wStr, reqChars);
    return wStr;
}
lfdir_t *win32_opendir(const char *path)
{
    HANDLE *handle = 0;
    WIN32_FIND_DATAA fdata;
    char buf[256] = {0};

#ifdef CONFIG_LP_USE_FS_WIN32_EXE_PATH
    char cwd[256] = {0};
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        printf("getcwd error\r\n");
        exit(0);
    }
    strcat(cwd, "/Fat");
    snprintf(buf, sizeof(buf), "%s%s\\*", cwd, &path[2]);
#else
    snprintf(buf, sizeof(buf), "%s\\*", path);

#if CONFIG_LP_USE_FS_WIN32_LETTER
    buf[0] = CONFIG_LP_USE_FS_WIN32_LETTER;
#else
    buf[0] = buf[1];
#endif
    buf[1] = ':';
#endif

    handle = FindFirstFileA(buf, &fdata);
    return handle;
}
lp_dir_t *win32_readdir(lfdir_t *dir)
{
    lp_dir_t *dirent = &dir->dirent;
    WIN32_FIND_DATAA fdata;

    while (FindNextFileA(dir->dir, &fdata))
    {
        if (is_dots_name(fdata.cFileName))
        {
            continue;
        }
        else
        {
            strcpy(dirent->name, fdata.cFileName);
            dirent->type = 0;
            if (fdata.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)
            {
                dirent->type = LF_TYPE_DIR;
            }
            else
            {
                dirent->type = LF_TYPE_FILE;
            }
            return dirent;
        }
    }
    return 0;
}
int win32_closedir(lfdir_t *dir)
{
    FindClose(dir);
    return 0;
}

int win32_stat(const char *path, lp_info_t *info)
{
    char buf[256];
#ifdef CONFIG_LP_USE_FS_WIN32_EXE_PATH
    char cwd[256] = {0};
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        printf("getcwd error\r\n");
        exit(0);
    }
    strcat(cwd, "/Fat");
    snprintf(buf, sizeof(buf), "%s%s", cwd, &path[2]);
#else
    strcpy(buf, path);

#if CONFIG_LP_USE_FS_WIN32_LETTER
    buf[0] = CONFIG_LP_USE_FS_WIN32_LETTER;
#else
    buf[0] = buf[1];
#endif
    buf[1] = ':';
#endif
    LP_LOGD("win32_stat[%s]\r\n", buf);

    WCHAR *wPath = mz_utf8z_to_widechar(buf);
    struct _stat _Stat;
    int res = _wstat(wPath, &_Stat);
    if (_Stat.st_mode && _S_IFDIR)
        info->type = LF_TYPE_DIR;
    else
        info->type = LF_TYPE_FILE;
    info->fsize = _Stat.st_size;
    free(wPath);
    return res;
}

lp_file_t *win32_open(const char *filename, const char *mode)
{
    char buf[256];
#ifdef CONFIG_LP_USE_FS_WIN32_EXE_PATH
    char cwd[256] = {0};
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        printf("getcwd error\r\n");
        exit(0);
    }
    strcat(cwd, "/Fat");
    snprintf(buf, sizeof(buf), "%s%s", cwd, &filename[2]);
#else
    strcpy(buf, filename);

#if CONFIG_LP_USE_FS_WIN32_LETTER
    buf[0] = CONFIG_LP_USE_FS_WIN32_LETTER;
#else
    buf[0] = buf[1];
#endif
    buf[1] = ':';
#endif
    // LP_LOGD("win32 open[%s][%s]\r\n", buf, mode);

    return (lp_file_t *)fopen(buf, mode);
}

int win32_mkdir(const char *path)
{
    char buf[256];
#ifdef CONFIG_LP_USE_FS_WIN32_EXE_PATH
    char cwd[256] = {0};
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        printf("getcwd error\r\n");
        exit(0);
    }
    strcat(cwd, "/Fat");
    snprintf(buf, sizeof(buf), "%s%s", cwd, &path[2]);
#else
    strcpy(buf, path);
#if CONFIG_LP_USE_FS_WIN32_LETTER
    buf[0] = CONFIG_LP_USE_FS_WIN32_LETTER;
#else
    buf[0] = buf[1];
#endif
    buf[1] = ':';
#endif
    return mkdir(buf);
}

int win32_fclose(lp_file_t *file)
{
    return fclose((FILE *)file);
}
int win32_fread(void *ptr, int size, int nmemb, lp_file_t *stream)
{
    return fread(ptr, size, nmemb, (FILE *)stream);
}
int win32_fwrite(const void *ptr, int size, int nmemb, lp_file_t *stream)
{
    return fwrite(ptr, size, nmemb, (FILE *)stream);
}

int win32_fseek(lp_file_t *file, uint32_t pos, int whence)
{
    return fseek((FILE *)file, pos, whence);
}
int win32_ftell(lp_file_t *file) // 当前位置
{
    return ftell((FILE *)file);
}

int win32_rename(const char *oldpath, const char *newpath)
{
    return rename(oldpath, newpath);
}
int win32_unlink(const char *filename)
{
    char buf[256];
#ifdef CONFIG_LP_USE_FS_WIN32_EXE_PATH
    char cwd[256] = {0};
    if (getcwd(cwd, sizeof(cwd)) == NULL)
    {
        printf("getcwd error\r\n");
        exit(0);
    }
    strcat(cwd, "/Fat");
    snprintf(buf, sizeof(buf), "%s%s", cwd, &filename[2]);
#else
    strcpy(buf, filename);
#if CONFIG_LP_USE_FS_WIN32_LETTER
    buf[0] = CONFIG_LP_USE_FS_WIN32_LETTER;
#else
    buf[0] = buf[1];
#endif
    buf[1] = ':';
#endif
    LP_LOGD("win32_unlink[%s]\r\n", buf);
    WCHAR *wPath = mz_utf8z_to_widechar(buf);
    struct _stat _Stat;
    int res = _wstat(wPath, &_Stat);
    free(wPath);
    if (0 == res)
    {
        if (_Stat.st_mode && _S_IFDIR)
            return rmdir(buf);
        else
            return remove(buf);
    }
    return 10;
}
int win32_volume(char *driver, int *total, int *free_bs)
{
    *total = 128 * 1024 * 1024;
    *free_bs = 64 * 1024 * 1024;
    return 0;
}
// lfs的drive
const lp_fat_drive_t win32_drive = {
    .open_cb = win32_open,
    .close_cb = win32_fclose,
    .read_cb = win32_fread,
    .write_cb = win32_fwrite,
    .stat_cb = win32_stat,
    .unlink_cb = win32_unlink,
    .seek_cb = win32_fseek,
    .tell_cb = win32_ftell, // 当前位置
    .format_cb = 0,
    .mount_cb = 0,
    .rename_cb = win32_rename,
    .mkdir_cb = win32_mkdir,
    .dir_open_cb = win32_opendir,
    .dir_read_cb = win32_readdir,
    .dir_close_cb = win32_closedir,
    .volume_cb = win32_volume,
};

int win_fs_init(char letter)
{
    int err = 0;
    lp_fat_drive_t *lp_fat_drive;

    LP_LOGD("win32 driver init [%c]\r\n", letter);

    lp_fat_drive = lpbsp_malloc(sizeof(lp_fat_drive_t));
    memcpy(lp_fat_drive, &win32_drive, sizeof(lp_fat_drive_t));
    lp_fat_drive->letter = letter;

    err = lp_fat_driver_init(lp_fat_drive, (int)sizeof(lp_fat_drive_t));
    if (err)
    {
        LP_LOGD("win32_driver_init err=%d\r\n", err);
        return LF_ERR;
    }
    return LF_OK;
}
#endif
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
#ifdef CONFIG_LP_USE_FS_STDIO
#include <dirent.h>
#include <unistd.h>
#include "stdio.h"
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
lfdir_t *stdio_opendir(const char *path)
{
    return opendir(path);
}
lp_dir_t *stdio_readdir(lfdir_t *dir)
{
    struct dirent *entry;
    lp_dir_t *dirent = &dir->dirent;

    entry = readdir(dir->dir);
    if (entry)
    {
        strcpy(dirent->name, entry->d_name);
        if (entry->d_type == DT_DIR)
        {
            dirent->type = LF_TYPE_DIR;
        }
        else
        {
            dirent->type = LF_TYPE_FILE;
        }
        return dirent;
    }
    return 0;
}
int stdio_closedir(lfdir_t *dir)
{
    return closedir(dir);
}

int stdio_stat(const char *path, lp_info_t *info)
{
    struct stat tbuf;
    int ret;

    ret = stat(path, &tbuf);
    if (ret < 0)
    {
        LP_LOGD("stat Error[%d]path %s: %s\n", errno, path, strerror(errno));
        return 1;
    }
    info->fsize = tbuf.st_size;
    return 0;
}
int stdio_mkdir(const char *path)
{
    int ret;

    ret = mkdir(path, 0775);
    if (ret)
    {
        LP_LOGD("mkdir Error[%d]: %s\n", ret, strerror(errno));
    }
    return ret;
}

lp_file_t *stdio_open(const char *filename, const char *mode)
{
    char buf[256];
    strcpy(buf, filename);
    return (lp_file_t *)fopen(buf, mode);
}

int stdio_fclose(lp_file_t *file)
{
    return fclose((FILE *)file);
}
int stdio_fread(void *ptr, int size, int nmemb, lp_file_t *stream)
{
    return fread(ptr, size, nmemb, (FILE *)stream);
}
int stdio_fwrite(const void *ptr, int size, int nmemb, lp_file_t *stream)
{
    return fwrite(ptr, size, nmemb, (FILE *)stream);
}

int stdio_fseek(lp_file_t *file, uint32_t pos, int whence)
{
    return fseek((FILE *)file, pos, whence);
}
int stdio_ftell(lp_file_t *file) // 当前位置
{
    return ftell((FILE *)file);
}

int stdio_rename(const char *oldpath, const char *newpath)
{
    return rename(oldpath, newpath);
}
int stdio_unlink(const char *filename)
{
    return unlink(filename);
}

// lfs的drive
const lp_fat_drive_t stdio_drive = {
    .open_cb = stdio_open,
    .close_cb = stdio_fclose,
    .read_cb = stdio_fread,
    .write_cb = stdio_fwrite,
    .stat_cb = stdio_stat,
    .unlink_cb = stdio_unlink,
    .seek_cb = stdio_fseek,
    .tell_cb = stdio_ftell, // 当前位置
    .format_cb = 0,
    .mount_cb = 0,
    .rename_cb = stdio_rename,
    .mkdir_cb = stdio_mkdir,
    .dir_open_cb = stdio_opendir,
    .dir_read_cb = stdio_readdir,
    .dir_close_cb = stdio_closedir,
    .volume_cb = 0,
};

int stdio_fs_init(char letter)
{
    int err = 0;
    lp_fat_drive_t *lp_fat_drive;

    LP_LOGD("stdio driver init [%c]\r\n", letter);

    lp_fat_drive = lpbsp_malloc(sizeof(lp_fat_drive_t));
    memcpy(lp_fat_drive, &stdio_drive, sizeof(lp_fat_drive_t));
    lp_fat_drive->letter = letter;

    err = lp_fat_driver_init(lp_fat_drive, (int)sizeof(lp_fat_drive_t));
    if (err)
    {
        LP_LOGD("stdio_driver_init err=%d\r\n", err);
        return LF_ERR;
    }
    return LF_OK;
}
#endif

#ifdef CONFIG_LP_USE_FS_POSIX

lfdir_t *posix_opendir(const char *path)
{
    return 0;
}
lp_dir_t *posix_readdir(lfdir_t *dir)
{
    return 0;
}
int posix_closedir(lfdir_t *dir)
{
    return 0;
}

int posix_stat(const char *path, lp_info_t *info)
{
    return 0;
}
int posix_mkdir(const char *path)
{
    return 0; // f_mkdir(path);
}
lp_file_t *posix_open(const char *filename, const char *mode)
{
    char buf[256];
    strcpy(buf, filename);
    return (lp_file_t *)open(buf, mode);
}

int posix_fclose(lp_file_t *file)
{
    return close((FILE *)file);
}
int posix_fread(void *ptr, int size, int nmemb, lp_file_t *stream)
{
    return read((FILE *)stream, ptr, size * nmemb);
}
int posix_fwrite(const void *ptr, int size, int nmemb, lp_file_t *stream)
{
    return write((FILE *)stream, ptr, size, nmemb);
}

int posix_fseek(lp_file_t *file, uint32_t pos, int whence)
{
    return lseek((FILE *)file, pos, whence);
}
int posix_ftell(lp_file_t *file) // 当前位置
{
    return lseek((FILE *)file, 0, SEEK_CUR);
}

int posix_rename(const char *oldpath, const char *newpath)
{
    return rename(oldpath, newpath);
}
int posix_unlink(const char *filename)
{
    return 0; // f_unlink(filename);
}

// lfs的drive
const lp_fat_drive_t posix_drive = {
    .open_cb = posix_open,
    .close_cb = posix_fclose,
    .read_cb = posix_fread,
    .write_cb = posix_fwrite,
    .stat_cb = posix_stat,
    .unlink_cb = posix_unlink,
    .seek_cb = posix_fseek,
    .tell_cb = posix_ftell, // 当前位置
    .format_cb = 0,
    .mount_cb = 0,
    .rename_cb = posix_rename,
    .mkdir_cb = posix_mkdir,
    .dir_open_cb = posix_opendir,
    .dir_read_cb = posix_readdir,
    .dir_close_cb = posix_closedir,
    .volume_cb = 0,
};

int posix_fs_init(char letter)
{
    int err = 0;
    lp_fat_drive_t *lp_fat_drive;

    LP_LOGD("posix driver init [%c]\r\n", letter);

    lp_fat_drive = lpbsp_malloc(sizeof(lp_fat_drive_t));
    memcpy(lp_fat_drive, &posix_drive, sizeof(lp_fat_drive_t));
    lp_fat_drive->letter = letter;

    err = lp_fat_driver_init(lp_fat_drive, (int)sizeof(lp_fat_drive_t));
    if (err)
    {
        LP_LOGD("posix_driver_init err=%d\r\n", err);
        return LF_ERR;
    }
    return LF_OK;
}
#endif
///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
/////////////////////////fatfs///////////////////////////////////
//////////////////////////////////////////////////////////////
#ifdef CONFIG_LP_USE_FS_FATFS
#include "ff.h"

// 得到磁盘剩余容量
//  drv:磁盘编号("0:"/"1:")
//  total:总容量	 （单位KB）
//  free:剩余容量	 （单位KB）
// 返回值:0,正常.其他,错误代码
int fatfs_volume(char *driver, int *total, int *free_bs)
{
    FATFS *fs1;
    uint8_t res;
    uint32_t fre_clust = 0, fre_sect = 0, tot_sect = 0;
    char tbuf[3] = {0, ':', 0};

    tbuf[0] = driver[1] - LF_FILE_SYS_LETTER_START + '0';
    res = (uint32_t)f_getfree((const TCHAR *)tbuf, (DWORD *)&fre_clust, &fs1); // 得到磁盘信息及空闲簇数量
    if (res == 0)
    {
        tot_sect = (fs1->n_fatent - 2) * fs1->csize; // 得到总扇区数
        fre_sect = fre_clust * fs1->csize;           // 得到空闲扇区数
                                                     //  #if _MAX_SS != 512                                   //扇区大小不是512字节,则转换为512字节
                                                     //          tot_sect *= fs1->ssize / 512;
                                                     //          fre_sect *= fs1->ssize / 512;
                                                     //  #endif
        *total = tot_sect >> 1;                      // 单位为KB
        *free_bs = fre_sect >> 1;                    // 单位为KB
    }
    return res;
}
int char_string2unicode_string(char *des, char *src)
{
    int len = 0;
    if (0 != des)
    {
        while (*des)
        {
            *src++ = *des++;
            *src++ = 0;
            len += 2;
        }
    }
    *src++ = 0;
    *src++ = 0;
    len += 2;
    return len;
}
lp_file_t *fatfs_open(const char *fname, const char *mode)
{
    int ret = 0;
    FIL *fp = lpbsp_malloc(sizeof(FIL));
    char tbuf[128];
    uint8_t mode_value = 0;

    if (0 == strcmp(mode, "r"))
    {
        mode_value = FA_READ;
    }
    else if (0 == strcmp(mode, "rb+"))
    {
        mode_value = FA_READ | FA_WRITE;
    }
    else if (0 == strcmp(mode, "rb"))
    {
        mode_value = FA_READ;
    }
    else if (0 == strcmp(mode, "wb+"))
    {
        mode_value = FA_CREATE_ALWAYS | FA_READ | FA_WRITE;
    }
    else if (0 == strcmp(mode, "wb"))
    {
        mode_value = FA_CREATE_ALWAYS | FA_READ | FA_WRITE;
    }
    else if (0 == strcmp(mode, "r+"))
    {
        mode_value = FA_READ | FA_WRITE;
    }
    else if (0 == strcmp(mode, "w"))
    {
        mode_value = FA_CREATE_ALWAYS | FA_WRITE;
    }
    else if (0 == strcmp(mode, "w+"))
    {
        mode_value = FA_READ | FA_WRITE | FA_CREATE_ALWAYS;
    }
    else if (0 == strcmp(mode, "a"))
    {
        mode_value = FA_OPEN_APPEND | FA_WRITE;
    }
    else if (0 == strcmp(mode, "a+"))
    {
        mode_value = FA_OPEN_APPEND | FA_WRITE | FA_READ;
    }
    else if (0 == strcmp(mode, "w+x"))
    {
        mode_value = FA_CREATE_NEW | FA_WRITE | FA_READ;
    }
    else
    {
        LP_LOGD("fatfs open mode err %s\r\n", mode);
    }

    tbuf[0] = fname[1] - LF_FILE_SYS_LETTER_START + '0';
    tbuf[1] = ':';
    strcpy(&tbuf[2], &fname[3]);

    if (0 == fp)
        return 0;

    ret = f_open(fp, tbuf, mode_value);
    if (FR_OK == ret)
    {
        LP_LOGD("fatfs open [%s] %x\r\n", tbuf, mode_value);
        return fp;
    }
    LP_LOGD("fatfs open error [%s]  ret  %d  %x\r\n", tbuf, ret, mode_value);

    lpbsp_free(fp);
    return 0;
}
int fatfs_close(FIL *file)
{
    int ret = 0;
    if (0 == file)
        return 1;

    ret = f_close(file);
    lpbsp_free(file);

    return ret;
}

int fatfs_read(void *ptr, int size, int nmemb, FIL *stream)
{
    uint32_t dlen = 0;
    int ret = 0;
    ret = f_read(stream, ptr, size * nmemb, &dlen); /* Read data from the file */
    if (FR_OK == ret)
    {
        return dlen;
    }
    LP_LOGD("fatfs f_read error ret  %d\r\n", ret);
    return -ret;
}
int fatfs_write(const void *ptr, int size, int nmemb, FIL *stream)
{
    uint32_t dlen = 0;
    int ret = 0;
    ret = f_write(stream, ptr, size * nmemb, &dlen); /* Write data to the file */
    if (FR_OK == ret)
    {
        return dlen;
    }
    LP_LOGD("f_write len %d,rel wite %d,ret%d\r\n", nmemb, dlen, ret);
    return -ret;
}
int fatfs_stat(const char *path, lp_info_t *info)
{
    int ret;
    FILINFO fno;
    char tbuf[128];

    tbuf[0] = path[1] - LF_FILE_SYS_LETTER_START + '0';
    tbuf[1] = ':';
    strcpy(&tbuf[2], &path[3]);

    LP_LOGD("stat %s\r\n", tbuf);
    ret = f_stat(tbuf, &fno);
    if (0 == ret)
    {
        info->fsize = fno.fsize;
        return LF_OK;
    }
    return LF_ERR;
}

int fatfs_unlink(const char *filename)
{
    int ret = 0;
    char tbuf[128];
    tbuf[0] = filename[1] - LF_FILE_SYS_LETTER_START + '0';
    tbuf[1] = ':';
    strcpy(&tbuf[2], &filename[3]);

    ret = f_unlink(tbuf); // Remove a file or sub-directory
    return ret;
}

int fatfs_seek(FIL *file, uint32_t pos, int whence)
{
    int ret = 0;

    if (0 == whence)
    {
        ret = f_lseek(file, pos);
    }
    else if (1 == whence)
    {
        ret = f_lseek(file, f_tell(file) + pos);
    }
    else
    {
        ret = f_lseek(file, f_size(file) - pos);
    }
    return ret;
}
int fatfs_tell(FIL *file)
{
    int ret = 0;

    ret = f_tell(file);
    return ret;
}

int fatfs_rename(const char *oldpath, const char *newpath)
{
    int ret = 0;
    char otbuf[128];
    char ntbuf[128];

    otbuf[0] = oldpath[1] - LF_FILE_SYS_LETTER_START + '0';
    otbuf[1] = ':';
    strcpy(&otbuf[2], &oldpath[3]);

    ntbuf[0] = newpath[1] - LF_FILE_SYS_LETTER_START + '0';
    ntbuf[1] = ':';
    strcpy(&ntbuf[2], &newpath[3]);

    ret = f_rename(otbuf, ntbuf);
    return ret;
}
int fatfs_mkdir(const char *newdir)
{
    int ret = 0;
    char tbuf[128];
    tbuf[0] = newdir[1] - LF_FILE_SYS_LETTER_START + '0';
    tbuf[1] = ':';
    strcpy(&tbuf[2], &newdir[3]);

    ret = f_mkdir(tbuf);
    return ret;
}

lfdir_t *fatfs_dir_open(const char *path)
{
    FRESULT res = 0;
    DIR *dir = 0;
    char tbuf[128] = {0};

    tbuf[0] = path[1] - LF_FILE_SYS_LETTER_START + '0';
    tbuf[1] = ':';
    if (strlen(path) > 3) // /c  /c/a
    {
        strcpy(&tbuf[2], &path[3]);
    }

    dir = lpbsp_malloc(sizeof(DIR));
    if (0 == dir)
    {
        return 0;
    }

    LP_LOGD("open dir addr new[%s]ole[%s]\r\n", tbuf, path);

    res = f_opendir(dir, tbuf); /* Open the directory */
    if (0 == res)
        return dir;

    LP_LOGD("open dir addr %d[%s][%s]\r\n", res, tbuf, path);
    lpbsp_free(dir);
    return 0;
}
lp_dir_t *fatfs_dir_read(lfdir_t *dir)
{
    FRESULT res;
    FILINFO fno;

    res = f_readdir((DIR *)dir->dir, &fno); /* Read a directory item */
    if (res != FR_OK || fno.fname[0] == 0)
    {
        return 0;
    }
    else if (fno.fattrib & AM_DIR) /* It is a directory */
    {
        dir->dirent.type = LF_TYPE_DIR;
    }
    else /* It is a file. */
    {
        dir->dirent.type = LF_TYPE_FILE;
    }
    strcpy(dir->dirent.name, fno.fname);

    return &dir->dirent;
}
int fatfs_dir_close(lfdir_t *dir)
{
    int ret = 0;

    ret = f_closedir(dir);

    lpbsp_free(dir);
    return ret;
}
/**
 * @brief 格式化
 * @param path:磁盘路径,比如"0:","1:"
 * @param mode: FM_FAT, FM_FAT32, FM_EXFAT and FM_SFD
 * @param au:簇大小
 * @retval 执行结果
 */
uint8_t mf_fmkfs(char *path, uint8_t mode, uint16_t au)
{
    MKFS_PARM opt;
    opt.align = 0;
    opt.au_size = au; // 格式化时,要设置的簇大小,以字节为单位
    opt.fmt = mode;   // 格式化的类型 FM_FAT, FM_FAT32, FM_EXFAT and FM_SFD FM_ANY
    opt.n_fat = 0;
    opt.n_root = 0;
    return f_mkfs((const TCHAR *)path, &opt, 0, FF_MAX_SS); // 格式化,drv:盘符;mode:模式;au:簇大小
}

int fatfs_format(const char *driver_name) // 格式化文件系统
{
    char path[3] = {0, ':', 0};

    path[0] = driver_name[1] - LF_FILE_SYS_LETTER_START + '0';
    return mf_fmkfs(path, FM_FAT32, 512);
}
int fatfs_mount(const char *driver_name) // 格式化文件系统
{
    static FATFS *fs[FF_VOLUMES];
    static uint8_t fs_flg[FF_VOLUMES] = {0}, i = 0;
    char path[3] = {0, ':', 0};

    for (i = 0; i < FF_VOLUMES; i++)
    {
        if (0 == fs_flg[i])
        {
            break;
        }
    }
    if (i >= FF_VOLUMES)
        return LF_ERR;

    path[0] = driver_name[1] - LF_FILE_SYS_LETTER_START + '0';

    fs[i] = lpbsp_malloc(sizeof(FATFS));
    return f_mount(fs[i], path, 1); // 挂载SD卡
}

const lp_fat_drive_t fatfs_drive = {

    .open_cb = fatfs_open,
    .close_cb = fatfs_close,
    .read_cb = fatfs_read,
    .write_cb = fatfs_write,
    .stat_cb = fatfs_stat,
    .unlink_cb = fatfs_unlink,
    .seek_cb = fatfs_seek,
    .tell_cb = fatfs_tell, // 当前位置
    .format_cb = fatfs_format,
    .mount_cb = fatfs_mount,
    .rename_cb = fatfs_rename,
    .mkdir_cb = fatfs_mkdir,
    .dir_open_cb = fatfs_dir_open,
    .dir_read_cb = fatfs_dir_read,
    .dir_close_cb = fatfs_dir_close,
    .volume_cb = fatfs_volume,
};

/// @brief 初始化一个 fatfs
/// @param path 盘符 从0开始 "0:"  "1:"
/// @return 返回错误码
int fatfs_fs_init(char letter)
{
    int err = 0;
    int total = 0, free_bs = 0;
    lp_fat_drive_t *lfat_drive;
    char path[3] = {'/', 0, 0};

    path[1] = letter;
    lfat_drive = lpbsp_malloc(sizeof(lp_fat_drive_t));
    memcpy(lfat_drive, &fatfs_drive, sizeof(lp_fat_drive_t));
    lfat_drive->letter = letter;

    err = lp_fat_driver_init(lfat_drive, sizeof(lp_fat_drive_t));
    if (err)
    {
        LP_LOGD("fatfs_driver_init err=%d\r\n", err);
        return -1;
    }

    err = lp_mount(path); // 挂载SD卡
    if (err)
    {
        LP_LOGD("fatfs_f_mount err=%d\r\n", err);
        err = lp_format(path);
        LP_LOGD("f_mkfs err=%d\r\n", err);
        err = lp_mount(path); // 挂载SD卡
        if (err)
            return -1;
    }

    err = lp_fs_volume(path, &total, &free_bs);
    LP_LOGD(" -[%s] total=%dMB,free_bs=%dMB\r\n", path, total / 1024, free_bs / 1024);

    lp_filelist_printf(path); // 打印遍历文件

    return 0;
}
#endif
///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////
/////////////////////// lfs ////////////////////////////////////
///////////////////////////////////////////////////////////////////
#ifdef CONFIG_LP_USE_FS_LFS
int my_lfs_format(char *p)
{
    int num = lp_fat_driver_num_get(p);

    if (0 > num)
        return LF_FILE_NO_DRIVER_NUM;
    return lfs_format(lp_fat_driver_user_data(num, 1), lp_fat_driver_user_data(num, 2));
}
int my_lfs_mount(char *p)
{
    int num = lp_fat_driver_num_get(p);

    if (0 > num)
        return LF_FILE_NO_DRIVER_NUM;
    return lfs_mount(lp_fat_driver_user_data(num, 1), lp_fat_driver_user_data(num, 2));
}

int file_open_type(char *flags)
{
    int type = 0, len = 0, i = 0;
    len = strlen(flags);
    for (i = 0; i < len; i++)
    {
        switch (flags[i])
        {
        case 'r':
            type |= LFS_O_RDONLY;
            break;
        case 'w':
            type |= LFS_O_WRONLY;
            break;
        case 'b':
            type |= 0;
            break;
        case '+':
            type |= LFS_O_CREAT; // Fail if a file already exists   = 0x0100,    // Create a file if it does not exist
            break;
        default:
            break;
        }
    }
    // LP_LOGD("open type=%x\r\n", type);
    return type;
}
lfs_file_t *lfs_open(const char *path, char *flags)
{
    int num = 0, ret = 0;
    lfs_file_t *fd = lpbsp_malloc(sizeof(lfs_file_t));
    if (0 == fd)
        return 0;
    num = lp_fat_driver_num_get(path);

    if (0 > num)
        return LF_FILE_NO_DRIVER_NUM;
    fd->driver_num = num;
    ret = lfs_file_open(lp_fat_driver_user_data(num, 1), fd, (const char *)&path[3], file_open_type(flags));
    // LP_LOGD("open[%s] flgs[%s],ret[%d],%x,%d\r\n", &path[3], flags, ret, lp_fat_driver_user_data(num, 1), lp_fat_driver_user_data(num, 2));
    if (0 == ret)
        return fd;
    else
        lpbsp_free(fd);
    return 0;
}
int lfs_read(void *buffer, int size, int nmemb, lfs_file_t *file)
{
    return lfs_file_read((lfs_t *)lp_fat_driver_user_data(file->driver_num, 1), file, buffer, size * nmemb);
}
int lfs_write(void *buffer, int size, int nmemb, lfs_file_t *file)
{
    return lfs_file_write((lfs_t *)lp_fat_driver_user_data(file->driver_num, 1), file, buffer, size * nmemb);
}
int lfs_close(lfs_file_t *file)
{
    int ret = 0;

    ret = lfs_file_close((lfs_t *)lp_fat_driver_user_data(file->driver_num, 1), file);
    lpbsp_free(file);
    return ret;
}
int lfs_unlink(const char *filename)
{
    int ret = 0;
    int num = 0;

    num = lp_fat_driver_num_get(filename);
    if (0 > num)
        return LF_FILE_NO_DRIVER_NUM;
    ret = lfs_remove((lfs_t *)lp_fat_driver_user_data(num, 1), filename);
    return ret;
}
int lfs_tell(lfs_file_t *file)
{
    return lfs_file_tell((lfs_t *)lp_fat_driver_user_data(file->driver_num, 1), file);
}

int lfs_seek(lfs_file_t *file, int offset, int fromwhere)
{
    return lfs_file_seek((lfs_t *)lp_fat_driver_user_data(file->driver_num, 1), file, offset, fromwhere);
}

int lfs_renamemy(const char *oldpath, const char *newpath)
{
    int num = lp_fat_driver_num_get(oldpath);
    if (0 > num)
        return LF_FILE_NO_DRIVER_NUM;
    return lfs_rename((lfs_t *)lp_fat_driver_user_data(num, 1), oldpath, newpath);
}

int lfs_stat_api(const char *path, lp_info_t *info)
{
    int num, ret;
    struct lfs_info lfsinfo;
    num = lp_fat_driver_num_get(path);

    if (0 > num)
        return LF_FILE_NO_DRIVER_NUM;
    ret = lfs_stat((lfs_t *)lp_fat_driver_user_data(num, 1), &path[2], &lfsinfo);
    if (ret)
    {
        info->fsize = lfsinfo.size;
    }
    return ret;
}
int lfs_mk_dir(const char *path)
{
    int num, ret;

    num = lp_fat_driver_num_get(path);

    if (0 > num)
        return LF_FILE_NO_DRIVER_NUM;
    ret = lfs_mkdir(lp_fat_driver_user_data(num, 1), &path[2]);
    LP_LOGD("path %d %s\r\n", ret, path);

    return ret;
}

lfs_dir_t *lfs_opendir(const char *path)
{
    int ret = 0;
    int num = lp_fat_driver_num_get(path);

    if (0 > num)
        return LF_FILE_NO_DRIVER_NUM;
    lfs_dir_t *dir = lpbsp_malloc(sizeof(lfs_dir_t));
    if (0 == dir)
    {
        return 0;
    }
    dir->driver_num = num;

    if (0 == path[2])
    {
        ret = lfs_dir_open(lp_fat_driver_user_data(num, 1), dir, "/");
    }
    else
    {
        ret = lfs_dir_open(lp_fat_driver_user_data(num, 1), dir, &path[2]);
    }
    if (0 == ret)
        return dir;
    else
        lpbsp_free(dir);
    return 0;
}
lp_dir_t *lfs_readdir(lfdir_t *dir)
{
    lp_dir_t *dirent = 0;
    struct lfs_info info;
    int ret = 0;

    ret = lfs_dir_read(lp_fat_driver_user_data(dir->driver_num, 1), dir->dir, &info);

    if (ret > 0)
    {
        dirent = &dir->dirent;
        strcpy(dirent->name, info.name);
        dirent->type = 1;
        if (LFS_TYPE_DIR == info.type)
            dirent->type = 2;
        return dirent;
    }
    LP_LOGD("lfs_dir_read ret %d\r\n", ret);
    return 0;
}
int lfs_closedir(lfs_dir_t *dir)
{
    int ret = 0;
    ret = lfs_dir_close(lp_fat_driver_user_data(dir->driver_num, 1), dir);
    lpbsp_free(dir);
    return 0;
}
static int lfs_couut(void *p, lfs_block_t b)
{
    *(lfs_size_t *)p += 1;
    return 0;
}
int lfs_volume(char *driver, int *total, int *free_bs)
{
    int num = 0, err, driver_num = lp_fat_driver_num_get(driver);

    if (0 > num)
        return LF_FILE_NO_DRIVER_NUM;

    lfs_t *lfst = lp_fat_driver_user_data(driver_num, 1);
    err = lfs_fs_traverse(lfst, lfs_couut, &num);
    if (err)
        return 0;
    num = lfst->cfg->block_count - num;
    *free_bs = num * lfst->cfg->block_size;
    *total = lfst->cfg->block_count * lfst->cfg->block_size;
    return err;
}

static const lp_fat_drive_t lfs_drive_cfg = {

    .open_cb = lfs_open,
    .close_cb = lfs_close,
    .read_cb = lfs_read,
    .write_cb = lfs_write,
    .stat_cb = lfs_stat_api,
    .unlink_cb = lfs_unlink,
    .seek_cb = lfs_seek,
    .tell_cb = lfs_tell,        // 当前位置
    .format_cb = my_lfs_format, // 格式化F
    .mount_cb = my_lfs_mount,
    .rename_cb = lfs_renamemy,
    .mkdir_cb = lfs_mk_dir,
    .dir_open_cb = lfs_opendir,
    .dir_read_cb = lfs_readdir,
    .dir_close_cb = lfs_closedir,
    .volume_cb = lfs_volume,

    // .user_data1 = &lfs, /**< Custom file user1 data*/
    // .user_data2 = &cfg, /**< Custom file user2 data*/
};
/**
 * @description: 初始化lfs文件系统
 * @param {char} path  lfs挂载盘符
 * @param {int} block_count  多小个最少擦除块
 * @param {int} block_size  一个块的大小
 * @return {*}
 */
int lfs_fs_init(char letter, void *cfg)
{
    LP_LOGD("lfs driver init path [%c] \r\n", letter);
    int err = 0;
    char tbuf[3] = {'/', 0, 0};
    lp_fat_drive_t *lfs_drive;
    lfs_t *lfs;

    tbuf[1] = letter;
    lfs_drive = lpbsp_malloc(sizeof(lp_fat_drive_t));
    lfs = lpbsp_malloc(sizeof(lfs_t));
    memcpy(lfs_drive, &lfs_drive_cfg, sizeof(lp_fat_drive_t));

    lfs_drive->letter = letter;
    lfs_drive->user_data1 = lfs;
    lfs_drive->user_data2 = cfg;

    err = lp_fat_driver_init(lfs_drive, sizeof(lp_fat_drive_t));
    if (err)
    {
        LP_LOGD("lp_fat_driver_init err=%d\r\n", err);
        return 1;
    }

    err = lp_mount(tbuf);
    if (err)
    {
        LP_LOGD("lp_mount1 err %d\n", err);
        err = lp_format(tbuf);
        if (err)
        {
            LP_LOGD("lp_format err %d\n", err);
            return 2;
        }
        err = lp_mount(tbuf);
        if (err)
        {
            LP_LOGD("lp_mount2 err %d\n", err);
            return 3;
        }
    }
    // lp_filelist_printf(tbuf); // 打印遍历文件

    int total, free_bs;
    lp_fs_volume(tbuf, &total, &free_bs);
    LP_LOGD("[%s] total=%dKB,free_bs=%dKB\r\n", tbuf, total / 1024, free_bs / 1024);

    return 0;
}
#endif

////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

int lp_fs_init(void)
{
#if defined(CONFIG_LP_USE_FS_WIN32) || defined(WIN32)
    win_fs_init('C');
#endif

#ifdef CONFIG_LP_USE_FS_STDIO
    stdio_fs_init(CONFIG_LP_USE_FS_STDIO_LETTER);
#endif

#ifdef CONFIG_LP_USE_FS_POSIX
    posix_fs_init(CONFIG_LP_USE_FS_POSIX_LETTER);
#endif

#ifdef CONFIG_LP_USE_FS_FATFS
    fatfs_fs_init(CONFIG_LP_USE_FS_FATFS_LETTER);
#endif

#ifdef CONFIG_LP_USE_FS_LFS
    lfs_fs_init(CONFIG_LP_USE_FS_LFS_LETTER);
#endif
    return 0;
}

///////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////

int lp_http_content_length_get(char *heap)
{
    char *tbuf = 0, temp[32];
    int read_len = 0;

    tbuf = lp_string_next_chars((char *)heap, "Content-Length:");
    if (tbuf)
    {
        lp_string_split_index(tbuf, '\r', 0, temp);
        read_len = atoi((const char *)temp); //  这包数据多小
    }
    else
    {
        tbuf = lp_string_next_chars((char *)heap, "content-length:");
        if (tbuf)
        {
            lp_string_split_index(tbuf, '\r', 0, temp);
            read_len = atoi((const char *)temp); //  这包数据多小
        }
    }
    return read_len;
}

char *lp_http_content_range_get(char *heap)
{
    char *tbuf = 0;

    tbuf = lp_string_next_chars((char *)heap, "Content-Range:");
    if (tbuf)
        return tbuf;

    tbuf = lp_string_next_chars((char *)heap, "content-range:");

    return tbuf;
}
/*
url:127.0.0.1:8080/api_get
out ip:127.0.0.1
out port:8080
out api:api_get
*/
void lp_url_parser(char *url, char *ip, uint32_t *port, char *api)
{
    char *tempp, tbuf[8];
    uint8_t ip_flg = 0;

    if (0 == memcmp("http://", url, 7))
    {
        ip_flg = 1;
        url += 7;
    }
    else if (0 == memcmp("https://", url, 8))
    {
        ip_flg = 2;
        url += 8;
    }

    tempp = lp_string_next(url, ':');
    if (tempp)
    {
        if (ip)
            lp_string_split_index(url, ':', 0, ip);

        if (port)
        {
            lp_string_split_index(tempp, '/', 0, tbuf);
            *port = atoi(tbuf);
        }
    }
    else // 没有port的
    {
        if (port)
            *port = 80;

        if (ip)
            lp_string_split_index(url, '/', 0, ip);
    }

    tempp = lp_string_next(url, '/');
    if (api)
        strcpy(api, tempp - 1); // -1是把 / 也复制出去
}
/// @brief socket接受数据-已经收取到数据的时候-超出3ms就立马返回
/// @param fd socket 句柄
/// @param buf 接收buf
/// @param nbytes 期望接收大小
/// @param timeout_ms 超时时间单位ms
/// @return 真实接收大小
int lpbsp_socket_read_timeout(int fd, uint8_t *buf, int nbytes, int timeout_ms)
{
    int len = 0, num = 0, rx_error_num = 0;
    while (1)
    {
        lpbsp_delayms(5);
        len = lpbsp_socket_read(fd, &buf[num], nbytes - num);
        if (len > 0)
        {
            rx_error_num = 0;
            num += len;
            continue;
        }
        else
        {
#if defined(CONFIG_LP_USE_FS_WIN32) || defined(WIN32)
            if (num && rx_error_num > 15) // 有些网站回复很慢的
#else
            if (num && rx_error_num > 550)
#endif
            {
                break;
            }
            rx_error_num++;
        }

        timeout_ms -= 5;
        if (0 > timeout_ms)
            break;
    }
    return num;
}
int lpbsp_socket_read_http_timeout(int fd, uint8_t *buf, int nbytes, int timeout_ms)
{
    int len = 0, num = 0, read_len = 0, ret = 0;
    char *tbuf = 0, temp[32];

    while (1)
    {
        lpbsp_delayms(5);
        len = lpbsp_socket_read(fd, &buf[num], nbytes - num);
        if (len > 0)
        {
            num += len;

            if (0 == read_len)
            {
                tbuf = lp_string_next_chars((char *)buf, "Content-Length:");
                if (tbuf)
                {
                    lp_string_split_index((char *)tbuf, '\r', 0, temp);
                    read_len = atoi((const char *)temp); //  这包数据多小
                }
            }
            if (read_len)
            {
                tbuf = lp_string_next_chars((char *)buf, "\r\n\r\n");

                ret = num - ((uint32_t)tbuf - (uint32_t)buf); // all - info = data len
                if (ret == read_len)
                {
                    return num;
                }
            }
            continue;
        }

        timeout_ms -= 5;
        if (0 > timeout_ms)
            break;
    }
    return num;
}

/// @brief 读取http的返回
/// @param fd socket句柄
/// @param buf 读取数据buf
/// @param inout in，导入的buf大小，out，实体数据的大小
/// @param read_len out 读取到的全部数据大小
/// @return 0 没有读取出实体数据  1实体数据buf开始的地址
uint8_t *lpbsp_socket_read_http(int fd, uint8_t *buf, uint32_t *inout, uint32_t *read_len_out)
{
    int len = 0, num = 0, read_len = 0, ret = 0, nbytes = 0;
    char *tbuf = 0, temp[32];
    int timeout_ms = 5000;

    nbytes = *inout;
    *read_len_out = 0;
    while (1)
    {
        lpbsp_delayms(5);
        len = lpbsp_socket_read(fd, &buf[num], nbytes - num);
        if (len > 0)
        {
            num += len;

            if (0 == read_len)
            {
                read_len = lp_http_content_length_get((char *)buf);
            }
            if (read_len)
            {
                tbuf = lp_string_next_chars((char *)buf, "\r\n\r\n");

                ret = num - ((uint32_t)tbuf - (uint32_t)buf); // all - info = data len
                if (ret == read_len)
                {
                    *read_len_out = num; // 整一包的数据
                    *inout = read_len;   // 这包有效数据
                    return (uint8_t *)tbuf;
                }
            }
            continue;
        }

        timeout_ms -= 5;
        if (0 > timeout_ms)
            break;
    }
    *read_len_out = num;
    return 0;
}

/*
获取指定的webapi的boby数据 (少于4K)
baby_data:没有就发null
*/
#define NET_DAWN_FILE_BUF_LEN 1024 * 20

#define NET_DAWN_RECV_BUF_LEN (NET_DAWN_FILE_BUF_LEN + 512)
#define NET_DAWN_FILE_BUF_LEN_B (NET_DAWN_FILE_BUF_LEN - 1)
int lp_http_file_down(char *url, char *file_name, void (*prog_cb)(uint32_t prog, uint32_t fsize))
{
    lp_file_t *fd;
    int sock_num = 0, err = 0, ret = 0, fsize = 0, now_size = 0, try_num = 0;
    const char *request = "GET %s HTTP/1.1\r\nHost:%s:%d\r\nConnection:keep-alive\r\nRange:bytes=%d-%d\r\n\r\n";
    char web_api[64] = {0}, *send_temp = 0; //
    char *recv_buf, ip[20], *tbuf = 0, temp[32], *psize;
    uint32_t ip_port, file_len = 0, read_len = 0, data_len = 0;

    if (url == 0 || file_name == 0)
    {
        return LF_ERR_PARAMETER;
    }

    lp_url_parser(url, ip, &ip_port, web_api);
    LP_LOGD("ip %s,port %d,api %s\r\n", ip, ip_port, web_api);

    fd = lp_open(file_name, "wb+");
    if (0 == fd)
    {
        return LF_ERR_OPEN_FILE;
    }

    recv_buf = lpbsp_malloc(NET_DAWN_RECV_BUF_LEN);
    if (0 == recv_buf)
    {
        lp_close(fd);
        LP_LOGE("malloc error\r\n");
        return LF_ERR_MALLOC;
    }

    sock_num = lpbsp_socket_connect_url(ip, ip_port);
    if (0 >= sock_num)
    {
        err = LF_ERR_SOCKET_CONNECT;
        goto OVER;
    }
    send_temp = lpbsp_malloc(512);
    if (0 == send_temp)
        return LF_ERR_MALLOC;
    while (1)
    {
        if (0 == fsize)
            sprintf(send_temp, request, web_api, ip, ip_port, file_len, 4095);
        else
        {
            if (fsize - file_len >= NET_DAWN_FILE_BUF_LEN_B)
                sprintf(send_temp, request, web_api, ip, ip_port, file_len, file_len + NET_DAWN_FILE_BUF_LEN_B);
            else
                sprintf(send_temp, request, web_api, ip, ip_port, file_len, fsize - 1);
        }

        for (try_num = 0; try_num < 5; try_num++)
        {
            if (try_num) // bug 防止上一包的残留数据
                lpbsp_socket_read_timeout(sock_num, (uint8_t *)recv_buf, NET_DAWN_RECV_BUF_LEN, 500);

            ret = lpbsp_socket_write(sock_num, (uint8_t *)send_temp, strlen(send_temp));
            if (0 > ret)
            {
                lpbsp_socket_close(sock_num);
                err = LF_ERR_SOCKET_WRITE;
                break;
            }

            data_len = NET_DAWN_RECV_BUF_LEN;
            tbuf = (char *)lpbsp_socket_read_http(sock_num, (uint8_t *)recv_buf, &data_len, &read_len);
            if (0 == tbuf)
            {
                err = 8;
                continue;
            }
            if (0 == fsize)
            {
                psize = lp_http_content_range_get(recv_buf);
                psize = lp_string_split_index(psize, '/', 0, temp);
                psize = lp_string_split_index(psize, '\r', 0, temp);
                fsize = atoi(temp);
            }

            err = 0;
            now_size += data_len;
            file_len += data_len; // 读取成功了,才读取下一包啊
            if (prog_cb)
            {
                prog_cb(now_size, fsize);
            }
            break; // ok
        }

        if (err)
            break;

        ret = lp_write(tbuf, 1, data_len, fd);
        lpbsp_delayms(100);
        if (ret != data_len)
        {
            LP_LOGW("lp_write error %d  %d\r\n", data_len, ret);
            err = 10;
            break;
        }

        // LP_LOGD("read=%d,now=%d,fsize=%d\r\n", data_len, now_size, fsize);
        if (now_size == fsize)
        {
            break;
        }
    }
OVER:

    LP_LOGD("down ret core[%d]\r\n", err);
    lpbsp_socket_close(sock_num);
    lp_close(fd);
    lpbsp_free(recv_buf);
    if (send_temp)
        lpbsp_free(send_temp);
    return err;
}

//
int lp_http_file_down_data(char *url, int (*prog_cb)(uint8_t *data, uint32_t prog, uint32_t fsize), int (*prog_bar_cb)(uint32_t prog, uint32_t fsize))
{
    int sock_num = 0, err = 0, ret = 0, fsize = 0, now_size = 0, try_num = 0;
    const char *request = "GET %s HTTP/1.1\r\nHost:%s:%d\r\nConnection:keep-alive\r\nRange:bytes=%d-%d\r\n\r\n";
    char web_api[64] = {0}, send_temp[256] = {0}; //
    char *recv_buf, ip[20], *tbuf = 0, temp[32], *psize;
    uint32_t ip_port, file_len = 0, read_len = 0, data_len = 0;

    if (url == 0)
    {
        return LF_ERR_PARAMETER;
    }

    lp_url_parser(url, ip, &ip_port, web_api);
    LP_LOGD("ip %s,port %d,api %s\r\n", ip, ip_port, web_api);

    recv_buf = lpbsp_malloc(NET_DAWN_RECV_BUF_LEN);
    if (0 == recv_buf)
    {
        LP_LOGE("malloc error\r\n");
        return LF_ERR_MALLOC;
    }

    sock_num = lpbsp_socket_connect_url(ip, ip_port);
    if (0 >= sock_num)
    {
        err = LF_ERR_SOCKET_CONNECT;
        goto OVER;
    }
    while (1)
    {
        if (0 == fsize)
            sprintf(send_temp, request, web_api, ip, ip_port, file_len, 4095);
        else
        {
            if (fsize - file_len >= NET_DAWN_FILE_BUF_LEN_B)
                sprintf(send_temp, request, web_api, ip, ip_port, file_len, file_len + NET_DAWN_FILE_BUF_LEN_B);
            else
                sprintf(send_temp, request, web_api, ip, ip_port, file_len, fsize - 1);
        }

        for (try_num = 0; try_num < 5; try_num++)
        {
            if (try_num) // bug 防止上一包的残留数据
                lpbsp_socket_read_timeout(sock_num, (uint8_t *)recv_buf, NET_DAWN_RECV_BUF_LEN, 500);

            ret = lpbsp_socket_write(sock_num, (uint8_t *)send_temp, strlen(send_temp));
            if (0 > ret)
            {
                lpbsp_socket_close(sock_num);
                err = LF_ERR_SOCKET_WRITE;
                break;
            }

            data_len = NET_DAWN_RECV_BUF_LEN;
            tbuf = (char *)lpbsp_socket_read_http(sock_num, (uint8_t *)recv_buf, &data_len, &read_len);
            if (0 == tbuf)
            {
                err = 8;
                continue;
            }
            if (0 == fsize)
            {
                psize = lp_http_content_range_get(recv_buf);
                psize = lp_string_split_index(psize, '/', 0, temp);
                psize = lp_string_split_index(psize, '\r', 0, temp);
                fsize = atoi(temp);
                prog_cb(0, LP_FUNC_CB_START, fsize);
            }

            err = 0;
            now_size += data_len;
            file_len += data_len; // 读取成功了,才读取下一包啊
            if (prog_cb)
            {
                ret = prog_cb((uint8_t *)tbuf, LP_FUNC_CB_DATA, data_len);
                if (ret)
                {
                    LP_LOGW("prog_cb error %d  %d\r\n", data_len, ret);
                    err = 10;
                    break;
                }
            }
            if (prog_bar_cb)
                prog_bar_cb(now_size, fsize);
            break; // ok
        }

        if (err)
            break;

        // LP_LOGD("read=%d,now=%d,fsize=%d\r\n", data_len, now_size, fsize);
        if (now_size == fsize)
        {
            ret = prog_cb((uint8_t *)tbuf, LP_FUNC_CB_END, fsize);
            if (ret)
                err = ret + 20;
            break;
        }
    }
OVER:

    LP_LOGD("down ret core[%d]\r\n", err);
    lpbsp_socket_close(sock_num);
    lpbsp_free(recv_buf);
    return err;
}
int lp_http_file_up(char *url, char *file_name, void (*prog_cb)(uint32_t prog, uint32_t fsize))
{
    lp_file_t *fd;
    int sock_num = 0, err = 0, ret = 0;
    const char *request = "PUT %s HTTP/1.1\r\nHost:%s:%d\r\nContent-Type: text/plain\r\nContent-Length:%d\r\n\r\n";
    char web_api[64] = {0}, *send_temp = 0; //
    char *recv_buf, ip[20], temp[32];
    uint32_t ip_port, now_size = 0, file_len = 0;

    if (url == 0 || file_name == 0)
        return LF_ERR_PARAMETER;

    lp_url_parser(url, ip, &ip_port, web_api);
    LP_LOGD("ip %s,port %d,api %s\r\n", ip, ip_port, web_api);

    fd = lp_open(file_name, "rb");
    if (0 == fd)
        return LF_ERR_OPEN_FILE;

    file_len = lp_fsize(fd);

    recv_buf = lpbsp_malloc(NET_DAWN_RECV_BUF_LEN);
    if (0 == recv_buf)
        return LF_ERR_MALLOC;

    sock_num = lpbsp_socket_connect_url(ip, ip_port);
    if (0 >= sock_num)
    {
        err = LF_ERR_SOCKET_CONNECT;
        goto OVER;
    }
    send_temp = lpbsp_malloc(512);
    if (0 == send_temp)
        return LF_ERR_MALLOC;
    sprintf(send_temp, request, web_api, ip, ip_port, file_len);
    lpbsp_socket_write(sock_num, (uint8_t *)send_temp, strlen(send_temp));
    lpbsp_free(send_temp);
    while (1)
    {
        ret = lp_read(recv_buf, 1, NET_DAWN_RECV_BUF_LEN, fd);
        if (0 == ret)
        {
            ret = lpbsp_socket_write(sock_num, (const uint8_t *)"\r\n", 2);
            if (0 > ret)
                err = 12;
            break;
        }
        ret = lpbsp_socket_write(sock_num, (uint8_t *)recv_buf, ret);
        if (0 > ret)
        {
            lpbsp_socket_close(sock_num);
            err = LF_ERR_SOCKET_WRITE;
            break;
        }
        now_size += ret;
        if (prog_cb)
        {
            prog_cb(now_size, file_len);
        }
    }
    ret = lpbsp_socket_read(sock_num, (uint8_t *)recv_buf, NET_DAWN_RECV_BUF_LEN);
    if (ret > 0)
        recv_buf[ret] = 0;

OVER:
    LP_LOGD("up ret core[%d]\r\n", err);
    lpbsp_socket_close(sock_num);
    lp_close(fd);
    lpbsp_free(recv_buf);
    return err;
}
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////
int lp_fs_test(void)
{
    lp_file_t *fd;
    int ret = 0;

    fd = lp_open("/D/lftest.txt", "wb+");
    if (0 == fd)
    {
        LP_LOGD("open err\r\n");
        return -1;
    }
    ret = lp_write("test", 1, 4, fd);

    ret = lp_close(fd);

    ret = lp_unlink("/D/lftest.txt");
    LP_LOGD("lp_unlink err %d\r\n", ret);
    return 0;
}
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////        粘贴板
static int pasteboard_type = 0;
static uint8_t *pasteboard_data = 0;
static int pasteboard_len = 0;

/// @brief 设置粘贴板
/// @param type 类型
/// @param data 数据
/// @param len 长度
/// @return 0 ok 1 malloc err
int lp_pasteboard_set(int type, uint8_t *data, int len)
{
    if (pasteboard_data)
    {
        lpbsp_free(pasteboard_data);
    }

    pasteboard_data = lpbsp_malloc(len);
    if (pasteboard_data)
    {
        pasteboard_type = type;
        memcpy(pasteboard_data, data, len);
        pasteboard_len = len;
        return 0;
    }
    return 1;
}
/// @brief 获取粘贴板类型和长度
/// @param data_len_out 长度输出
/// @return 0 ok
int lp_pasteboard_type_get(int *data_len_out)
{
    if (data_len_out)
        *data_len_out = pasteboard_len;
    return pasteboard_type;
}
/// @brief 获取粘贴板内容
/// @param data_out 输出内容buf
/// @return 0 ok
int lp_pasteboard_get(uint8_t *data_out)
{
    memcpy(data_out, pasteboard_data, pasteboard_len);
    return 0;
}
/// @brief 清空粘贴板
/// @param nano
/// @return 0 ok
int lp_pasteboard_clear(void)
{
    if (pasteboard_data)
        lpbsp_free(pasteboard_data);
    return 0;
}
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
/// @brief 字符串相加
/// @param s1 源字符
/// @param s2 待相加字符
char *lp_string_add_string(char *s1, char *s2)
{
    int len = 0, len2, i = 0;

    if (*s2 == '\0')
        return ((char *)s1);
    len = strlen(s1);

    while (0 != s2[i])
    {
        s1[len + i] = s2[i];
        i++;
    }
    s1[len + i] = 0;
    return (NULL);
}
/**
 *  返回下一个字符的位置
 * @param in I 字符串
 * @param c I 字符
 * @return 下一个字符c的位置
 */
char *lp_string_next(char *in, char c)
{
    int i = 0;
    while (1)
    {
        if (in[i] == c)
            return &in[i + 1];
        else if (in[i++] == 0)
            return 0;
    }
    return 0;
}

/**
 *  删除c后面的字符 包括当前
 * @param in I 字符串
 * @param c I 字符
 * @return 0 ok
 */
int lp_string_cut(char *in, char c)
{
    int i = 0;
    while (1)
    {
        if (in[i] == c)
        {
            in[i] = 0;
            return 0;
        }
        else if (in[i++] == 0)
            return 1;
    }
    return 1;
}
/**
 *  返回下一个字符串的位置
 * @param in I 字符串
 * @param c I 字符串
 * @return 下一个字符串c的位置
 */
char *lp_string_next_chars(char *in, char *c)
{
    int i = 0, dlen = 0;

    dlen = strlen(c);
    while (1)
    {
        if (in[i] == c[0] && 0 == memcmp(&in[i], c, dlen))
        {
            return &in[i + dlen];
        }
        else if (in[i++] == 0)
            return 0;
    }
    return 0;
}

/// @brief string copy
/// @param src in:
/// @param out out:
/// @param max_num in:
/// @return
int lp_string_copy(char *src, char *out, int max_num)
{
    int i = 0;
    out[max_num - 1] = 0;
    for (i = 0; i < max_num - 1; i++)
    {
        out[i] = src[i];
        if (0 == out[i])
            break;
    }
    return i;
}

/// @brief 字符相加
/// @param src 源字符
/// @param in 待相加字符
/// @return 返回src字符结尾指针
char *lp_string_cat(char *src, char *in)
{
    int len = 0;
    int dlen = 0;

    len = strlen(src);
    len = strlen(in);
    strcpy(src + len, in);
    return src + len + dlen;
}
/**
 *  返回字符串中的ch的数量
 * @param in I 字符串
 * @param c I 字符
 * @return 字符串中的ch的数量
 */
int lp_string_char_num_get(char *src, char ch)
{
    int num = 0, i = 0;
    while (src[i])
    {
        if (ch == src[i])
            num++;
        i++;
    }
    return num;
}

/**
 *  返回字符串中的c 分割 第index个 的长度
 * @param src I 字符串
 * @param c I 字符
 * @param index I 第index个
 * @return 字符串长度
 */
int lp_string_split_index_size(char *src, char c, int index)
{
    int last = 0, i = 0, gnum = 1, k = 0;
    if (0 == index)
    {
        while (1)
        {
            if (src[i] == c || src[i] == 0)
                return i;
            i++;
        }
    }
    for (i = 0; i < strlen(src); i++)
    {
        if (src[i] == c)
        {
            if (gnum == index)
            {
                i++;
                for (; i < strlen(src); i++)
                {
                    if (src[i] == c)
                        return k;
                    k++;
                }
                return k;
            }
            else
            {
                gnum++;
                last = i + 1;
            }
        }
    }
    LP_LOGW("last %d\r\n", last);
    return 0;
}

/**
 *  返回字符串中的c 分割 第num个 的data
 * @param in I 字符串
 * @param c I 字符
 * @param num I 第num个
 * @param out O data
 * @return 下一份字符串的位置
 */
char *lp_string_split_index(char *in, char c, int num, char *out)
{
    int last = 0, i = 0, gnum = 1, k = 0;
    if (0 == num)
    {
        while (1)
        {
            if (in[i] == c)
            {
                out[i] = 0;
                return &in[i + 1];
            }
            else if (in[i] == 0)
            {
                out[i] = 0;
                return 0;
            }
            out[i] = in[i];
            i++;
        }
    }
    for (i = 0; i < strlen(in); i++)
    {
        if (in[i] == c)
        {
            if (gnum == num)
            {
                i++;
                for (; i < strlen(in); i++)
                {
                    if (in[i] == c)
                    {
                        out[k] = 0;
                        return &in[i + 1];
                    }
                    out[k++] = in[i];
                }
                out[k] = 0;
                return 0;
            }
            else
            {
                gnum++;
                last = i + 1;
            }
        }
    }
    LP_LOGD("last %d\r\n", last);

    return 0;
}
/// <summary>
/// 替代字符中的字符
/// </summary>
/// <param name="path">字符串</param>
/// <param name="ole">旧字符</param>
/// <param name="cnew">新字符</param>
void lp_string_replace_char(char *path, char ole, char cnew)
{
    uint16_t len = strlen(path), i = 0;
    for (i = 0; i < len; i++)
    {
        if (path[i] == ole)
            path[i] = cnew;
    }
}
/// @brief 返回文件名后缀
/// @param in 文件路径
/// @param out 后缀输出buf
/// @return
uint8_t lp_file_suffix(char *in, char *out)
{
    uint8_t flg = 1;
    int i, j = 0;
    for (i = 0; in[i] != 0; i++)
    {
        if (in[i] == '.')
        {
            flg = 0;
            j = i;
        }
    }
    if (0 == flg)
        strcpy(out, (const char *)&in[j + 1]);
    return 0;
}
// 判断文件名后缀
uint8_t lp_file_suffix_if(char *path, char *suffix)
{
    uint8_t flg = 1;
    int i, j = 0;
    for (i = 0; path[i] != 0; i++)
    {
        if (path[i] == '.')
        {
            flg = 0;
            j = i;
        }
    }
    if (0 == flg)
        if (0 == strcmp(suffix, &path[j + 1]))
            return 0;
    return 1;
}

/// <summary>
/// 全局路径加相对路径，输出malloc后数据
/// </summary>
/// <param name="rootpath_in">全局路径</param>
/// <param name="path_in">相对路径</param>
/// <returns>输出合成路径</returns>
char *lp_file_dir_add(char *rootpath_in, char *path_in)
{
    char losfpath[256], *apath;
    uint8_t i = 0, backs = 0, temp1 = 0, temp = 0;
    int8_t j = 0;
    strcpy(losfpath, rootpath_in);
    temp = strlen(path_in);
    for (i = 0; i < temp;)
    {
        if (0 == memcmp("../", &path_in[i], 3)) // skip
        {
            i += 3;
            backs++;
        }
        else if (0 == memcmp("./", &path_in[i], 2)) // skip
        {
            i += 2;
            break;
        }
        else if ('/' == path_in[i]) // skip
        {
            i += 1;
            break;
        }
        else
        {
            break;
        }
    }
    temp1 = strlen(losfpath);
    if ('/' != losfpath[temp1 - 1])
    {
        losfpath[temp1++] = '/';
    }
    if (backs && (temp1 > 1)) // 后退代码
    {
        for (j = temp1 - 2; j > 0; j--)
        {
            if ('/' == losfpath[j])
            {
                backs--;
                if (0 == backs)
                {
                    temp1 = j + 1;
                    break;
                }
            }
        }
    }
    apath = lpbsp_malloc(temp1 + temp - i);
    if (0 == apath)
        return 0;
    memcpy(apath, losfpath, temp1);
    strcpy(&apath[temp1], &path_in[i]);
    return apath;
}
/// <summary>
/// 全路径的文件提取路径 如 /C/A/B.zip -> /C/A
/// </summary>
/// <param name="path">文件全路径 输入</param>
/// <param name="name">路径 输出</param>
/// <returns></returns>
uint8_t lp_file_dirpath_get(char *path, char *name)
{
    int i, j = 0, len;
    len = strlen(path);
    for (i = 0; len > i; i++)
    {
        if (path[i] == '/' || path[i] == '\\')
            j = i;
    }
    memcpy(name, path, j);
    name[j] = 0;
    if (j)
        return 0;
    return 1;
}
/// <summary>
/// 提取文件名带后缀 如  /C/A/B.zip -> B.zip
/// </summary>
/// <param name="path">全路径，输入</param>
/// <param name="name">文件名 输出</param>
/// <returns>0 ok 1 err</returns>
uint8_t lp_file_filename_get(char *path, char *name)
{
    int i, j = 0;
    for (i = 0; path[i] != 0; i++)
        if (path[i] == '/' || path[i] == '\\')
            j = i;
    strcpy(name, &path[j + 1]);
    if (j)
        return 0;
    return 1;
}

/// <summary>
/// 把全路径去除点后缀，返回如  /C/A/B.zip -> /C/A/B
/// </summary>
/// <param name="path">全路径，输入</param>
/// <param name="name">有路径文件名，不带后缀</param>
/// <returns></returns>
uint8_t lp_file_dirname_get(char *path, char *name)
{
    uint8_t flg = 1;
    int i, j = 0, len;
    len = strlen(path);
    for (i = 0; len > i; i++)
    {
        if (path[i] == '.')
        {
            flg = 0;
            j = i;
        }
    }
    memcpy(name, path, j);
    name[j] = 0;
    return flg;
}

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
#ifdef CONFIG_LP_USE_MINIZ
static int lp_file_list_func_cb(void *pOpaque, int root_len, const char *pFilename, int type)
{
    lp_file_t *fd;
    char tbuf[128];
    mz_bool status;

    LP_LOGD("Filename: %s Is Dir: %d\n", pFilename, type);

    if (LF_TYPE_DIR == type)
    {
        sprintf(tbuf, "%s/", &pFilename[root_len + 1]);
        status = mz_zip_writer_add_mem(pOpaque, tbuf, 0, 0, 0);
        if (!status)
        {
            LP_LOGE("mz_zip_writer_add_mem %d %s\n", status, tbuf);
        }
    }
    else // file
    {
        status = mz_zip_writer_add_file(pOpaque, &pFilename[root_len + 1], pFilename, 0, 0, 9);
        if (!status)
        {
            LP_LOGE("mz_zip_writer_add_file %d %s\n", status, pFilename);
        }
    }

    return 0;
}
/// @brief 压缩文件或文件夹
/// @param zip_file 待压缩文件或文件夹
/// @param panem 输出路径
/// @return
int lp_zip_en(char *zip_file, char *panem)
{
    mz_zip_archive zip;
    mz_bool status;

    memset(&zip, 0, sizeof(zip));
    status = mz_zip_writer_init_file(&zip, panem, 0);
    if (!status)
    {
        LP_LOGD("mz_zip_writer_init_file %s\n", panem);
        return 1;
    }

    lp_filelist(zip_file, lp_file_list_func_cb, &zip);

    status = mz_zip_writer_finalize_archive(&zip);
    LP_LOGD("mz_zip_writer_finalize_archive %d\n", status);

    status = mz_zip_writer_end(&zip);
    LP_LOGD("mz_zip_writer_end %d\n", status);
    return 0;
}
static size_t mz_unfile_write_func_cb(void *pOpaque, mz_uint64 file_ofs, const void *pBuf, size_t n)
{
    int ret;

    ret = lp_write(pBuf, 1, n, (lp_file_t *)pOpaque);
    return ret;
}
/// @brief 解压zip_file
/// @param zip_file 路径
/// @param out_path 放在这,这个后缀不带/
/// @return
int lp_zip_un(char *zip_file, char *out_path)
{
    mz_zip_archive zip_archive;
    mz_bool status;
    int i;
    int ret = 0;
    char tbuf[256], *temp;
    lp_file_t *fd_out;

    LP_LOGE("zip_file [%s] 2 out [%s]\n", zip_file, out_path);

    memset(&zip_archive, 0, sizeof(zip_archive));

    lp_file_dirname_get(zip_file, tbuf);
    ret = lp_mkdir_try(tbuf); // 先新建一个当前的目录
    if (0 != ret)
        return 2;

    status = mz_zip_reader_init_file(&zip_archive, zip_file, 0);
    if (!status)
    {
        LP_LOGE("mz_zip_reader_init_file() failed=%s!\n", mz_zip_get_error_string(mz_zip_get_last_error(&zip_archive)));
        return EXIT_FAILURE;
    }

    for (i = 0; i < (int)mz_zip_reader_get_num_files(&zip_archive); i++)
    {
        mz_zip_archive_file_stat file_stat;
        if (!mz_zip_reader_file_stat(&zip_archive, i, &file_stat))
        {
            LP_LOGE("mz_zip_reader_file_stat() failed!\n");
            mz_zip_reader_end(&zip_archive);
            return EXIT_FAILURE;
        }

        LP_LOGD("Filename: \"%s\", Comment: \"%s\", Uncompressed size: %u, Compressed size: %u, Is Dir: %u\n", file_stat.m_filename, file_stat.m_comment, (unsigned int)file_stat.m_uncomp_size, (unsigned int)file_stat.m_comp_size, file_stat.m_is_directory);

        snprintf(tbuf, sizeof(tbuf), "%s/%s", out_path, file_stat.m_filename);
        if (1 == file_stat.m_is_directory) // dir
        {
            lp_mkdir(tbuf);
        }
        else // file
        {
            fd_out = lp_open(tbuf, "wb");
            if (0 == fd_out)
            {
                LP_LOGW("lf open err %s\r\n", tbuf);
            }
            mz_zip_reader_extract_to_callback(&zip_archive, i, mz_unfile_write_func_cb, fd_out, 0);
            lp_close(fd_out);
        }
    }
    mz_zip_reader_end(&zip_archive);
    return 0;
}

/// @brief 读取zip中文件的状态
/// @param pZip zip文件句柄
/// @param pFilename 文件名字
/// @param file_stat 返回状态
/// @return 0 ok
int lp_zip_file_stat(mz_zip_archive *pZip, const char *pFilename, mz_zip_archive_file_stat *file_stat)
{
    uint32_t file_index;
    mz_bool status;

    status = mz_zip_reader_locate_file_v2(pZip, pFilename, NULL, 0, &file_index);
    if (!status)
    {
        LP_LOGW("mz_zip_reader_locate_file_v2() failed=%d!\n", status);
        return 1;
    }

    status = mz_zip_reader_file_stat(pZip, file_index, file_stat);
    if (!status)
    {
        LP_LOGW("mz_zip_reader_file_stat() failed=%d!\n", status);
        return 1;
    }
    return 0;
}
#else
int lp_zip_en(char *zip_file, char *panem)
{
    printf("Error : lp_zip_en no isexit! %s\r\n ", zip_file);
    return 1;
}
int lp_zip_un(char *zip_file, char *out_path)
{
    printf("Error : lp_zip_un no isexit! %s\r\n ", zip_file);
    return 1;
}
#endif

/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////
struct _lp_list_t
{
    unsigned char num_all;  // 已经申请的个数
    unsigned char num_free; // 空闲的个数
    unsigned char num_user; // 已使用的个数
    char **data;
};

/// @brief list init
/// @param nano
/// @return >0 ok handle
int lp_list_init(void)
{
    struct _lp_list_t *_lp_list;

    _lp_list = lpbsp_malloc(sizeof(struct _lp_list_t));
    if (0 == _lp_list)
        return 0;

    _lp_list->data = lpbsp_malloc(4 * 25); //
    if (0 == _lp_list->data)
    {
        lpbsp_free(_lp_list);
        return 0;
    }
    _lp_list->num_all = 25;
    _lp_list->num_free = 25;
    _lp_list->num_user = 0;
    return (int)_lp_list;
}
/// @brief list长度
/// @param handle 句柄
/// @return 已存在长度
int lp_list_len(int handle)
{
    struct _lp_list_t *_lp_list = (struct _lp_list_t *)handle;

    if (0 == _lp_list)
        return 0;
    return _lp_list->num_user;
}
/// @brief list 添加
/// @param handle 句柄
/// @param str 字符串
/// @return 0 ok
int lp_list_add(int handle, char *str)
{
    struct _lp_list_t *_lp_list = (struct _lp_list_t *)handle;
    char **temp = 0;
    int i = 0;

    if (0 == _lp_list)
        return 1;

    if (0 == _lp_list->num_free)
    {
        temp = lpbsp_malloc(4 * (_lp_list->num_all + 25));
        if (0 == temp)
            return 2;

        for (i = 0; i < _lp_list->num_all; i++)
        {
            temp[i] = _lp_list->data[i];
        }
        lpbsp_free(_lp_list->data);

        _lp_list->data = temp;
        _lp_list->num_all += 25;
        _lp_list->num_free += 25;
    }
    _lp_list->data[_lp_list->num_user] = lpbsp_malloc(strlen(str) + 1);
    if (0 == _lp_list->data[_lp_list->num_user])
        return 2;

    strcpy(_lp_list->data[_lp_list->num_user], str);
    _lp_list->num_user++;
    _lp_list->num_free--;
    return 0;
}

/// @brief 获取list 依据index
/// @param handle 句柄
/// @param index 序号 0 start
/// @return 内容指针
char *lp_list_get(int handle, int index)
{
    struct _lp_list_t *_lp_list = (struct _lp_list_t *)handle;

    if (0 == _lp_list)
        return 0;

    if (_lp_list->num_user > index)
    {
        return _lp_list->data[index];
    }
    return 0;
}

/// @brief list 删除一个index
/// @param handle 句柄
/// @param index 序号
/// @return 0 ok
int lp_list_del_one(int handle, int index)
{
    struct _lp_list_t *_lp_list = (struct _lp_list_t *)handle;

    if (0 == _lp_list)
        return 1;

    return 1;
}

/// @brief 释放list
/// @param handle 句柄
/// @return 0 ok
int lp_list_free(int handle)
{
    int i = 0;
    struct _lp_list_t *_lp_list = (struct _lp_list_t *)handle;

    if (0 == _lp_list)
        return 1;

    for (i = 0; i < _lp_list->num_all; i++)
    {
        lpbsp_free(_lp_list->data[i]);
    }
    lpbsp_free(_lp_list->data);
    return 0;
}
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
#define ABS(x) (((x) > 0) ? (x) : -(x))

static const float PI_VALUE = 3.14159265358979324;
static const float a_value = 6378245.0;
static const float ee_value = 0.00669342162296594323;

static float transformLat(float x, float y)
{
    float ret = -100.0 + 2.0 * x + 3.0 * y + 0.2 * y * y + 0.1 * x * y + 0.2 * sqrt(ABS(x));

    ret += (20.0 * sin(6.0 * x * PI_VALUE) + 20.0 * sin(2.0 * x * PI_VALUE)) * 2.0 / 3.0;
    ret += (20.0 * sin(y * PI_VALUE) + 40.0 * sin(y / 3.0 * PI_VALUE)) * 2.0 / 3.0;
    ret += (160.0 * sin(y / 12.0 * PI_VALUE) + 320 * sin(y * PI_VALUE / 30.0)) * 2.0 / 3.0;
    return ret;
}

static float transformLon(float x, float y)
{
    float ret = 300.0 + x + 2.0 * y + 0.1 * x * x + 0.1 * x * y + 0.1 * sqrt(ABS(x));

    ret += (20.0 * sin(6.0 * x * PI_VALUE) + 20.0 * sin(2.0 * x * PI_VALUE)) * 2.0 / 3.0;
    ret += (20.0 * sin(x * PI_VALUE) + 40.0 * sin(x / 3.0 * PI_VALUE)) * 2.0 / 3.0;
    ret += (150.0 * sin(x / 12.0 * PI_VALUE) + 300.0 * sin(x / 30.0 * PI_VALUE)) * 2.0 / 3.0;
    return ret;
}

void lp_gps_transform(float wgLat, float wgLon, float *mgLat, float *mgLon)
{
    float sqrtMagic;
    float dLat = transformLat(wgLon - 105.0, wgLat - 35.0);
    float dLon = transformLon(wgLon - 105.0, wgLat - 35.0);
    float radLat = wgLat / 180.0 * PI_VALUE;
    float magic = sin(radLat);

    magic = 1 - ee_value * magic * magic;
    sqrtMagic = sqrt(magic);
    dLat = (dLat * 180.0) / ((a_value * (1 - ee_value)) / (magic * sqrtMagic) * PI_VALUE);
    dLon = (dLon * 180.0) / (a_value / sqrtMagic * cos(radLat) * PI_VALUE);
    *mgLat = wgLat + dLat;
    *mgLon = wgLon + dLon;
};
///////////////////////////////////////////////////////////////////////////////////////////
static char gps_uart_id = -1;
static int task_hand_gps = 0;
static struct lp_gps_info_t gps_info = {0};
static uint8_t gps_effective_time = 0; // 有效时间

int lp_gps_read(struct lp_gps_info_t *info)
{
    if (info)
        memcpy(info, &gps_info, sizeof(struct lp_gps_info_t));
    return gps_effective_time;
}
static void gps_init_only(void)
{
    uint8_t cmdbuf[] = "$PCAS03,0,0,0,0,1,0,0,0,0,0,,,0,0*03\r\n"; // RMC
    lpbsp_uart_write(gps_uart_id, cmdbuf, strlen((char *)cmdbuf));
}
static int gps_read_info(struct lp_gps_info_t *info)
{
    struct minmea_sentence_rmc frame;
    uint32_t rx_len = 0, temp;
    uint8_t gps_buf[128] = {0};

    rx_len = lpbsp_uart_read(1, gps_buf, 128);
    if (rx_len)
    {
        // LP_LOGD("[%d][%s", rx_len, gps_buf);
        if (minmea_parse_rmc(&frame, (const char *)gps_buf))
        {
            info->dt.tm_year = frame.date.year + 2000;
            info->dt.tm_mon = frame.date.month;
            info->dt.tm_mday = frame.date.day;
            info->dt.tm_hour = frame.time.hours;
            info->dt.tm_min = frame.time.minutes;
            info->dt.tm_sec = frame.time.seconds;

            if (2049 > info->dt.tm_year && info->dt.tm_year > 2022)
            {
                uint32_t utc = lp_time_to_rtc((struct tm_t *)&info->dt) + 8 * 3600;
                temp = lp_time_utc_get();
                if (abs(utc - temp) > 45)
                    lp_time_utc_set(utc);
            }

            if (0 == frame.valid)
            {
                return 1;
            }
            info->latitude = minmea_tocoord(&frame.latitude);
            info->longitude = minmea_tocoord(&frame.longitude);

            lp_gps_transform(info->latitude, info->longitude, &info->latitude, &info->longitude);

            info->speed = minmea_tofloat(&frame.speed);
            return 0;
        }
        else
        {
            // LP_LOGD("[%d][%s", rx_len, gps_buf);
        }
    }
    return 1;
}
static void gps_task(void *arg)
{
    int ret = 0;
    uint8_t tbuf[16];

    tbuf[0] = 'G';
    while (1)
    {
        ret = gps_read_info(&gps_info);
        if (0 == ret)
        {
            gps_effective_time = 63;

            memcpy(tbuf + 1, (const void *)&gps_info.latitude, 4);
            memcpy(tbuf + 5, (const void *)&gps_info.longitude, 4);
            memcpy(tbuf + 9, (const void *)&gps_info.speed, 4);
            lplib_rtc_ram_add(tbuf, 13);
        }
        else
        {
            if (gps_effective_time)
                gps_effective_time--;
        }
        lpbsp_delayms(950); // delay
        // LP_LOGD(" G ");
    }
}
int lp_gps_state(void)
{
    return task_hand_gps;
}
int lp_gps_init(uint8_t eanble)
{
    gps_uart_id = 1;
    if (0 > gps_uart_id)
    {
        return -1;
    }

    lpbsp_uart_init(gps_uart_id, eanble);
    if (eanble && 0 == task_hand_gps)
    {
        lpbsp_delayms(200);
        gps_init_only();

        task_hand_gps = lpbsp_task_create("gps_task", 0, gps_task, 3072, 3);
    }
    else if (0 == eanble && task_hand_gps)
    {
        lpbsp_task_delete((void *)task_hand_gps);
        task_hand_gps = 0;
    }

    return 0;
}
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
int lp_sensor_magnetic_angle_read(void)
{
    int angle;
    int32_t x, y, z;

    lpbsp_sensor_magnetic_xyz_read(&x, &y, &z);
    angle = atan2((double)y, (double)x) * (180 / 3.14159265) + 180;

    return angle;
}
////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////
/// @brief 创建一个链表
/// @param 无
/// @return 该函数的功能是创建链表 把链表的头指针赋给p_lsit所指的链表指针
lp_link_t *lp_link_new(void)
{
    lp_link_t *p_list = 0;
    p_list = (lp_link_t *)lpbsp_malloc(sizeof(lp_link_t));
    if (p_list == 0)
        return 0;
    p_list->next = (lp_link_t *)p_list; // 指针域指向它本身
    return p_list;
}

/// @brief 按数据查找一个链表的节点
/// @param phead 链表句柄
/// @param data 节点数据
/// @return 节点
lp_link_t *lp_link_find(lp_link_t *phead, void *data)
{
    lp_link_t *p = (lp_link_t *)phead->next; // p指向第一个结点
    while (p != phead)                       // p未到表头
    {
        if (p->data == data)
        {
            return p;
        }
        p = (lp_link_t *)p->next;
    }
    if (p->data == data)
    {
        return p;
    }
    return 0;
}

/// @brief 在带头节点的单链表L中的第i个位置之前插入元素e
/// @param L 链表句柄
/// @param data 节点数据
/// @return 新增的节点
lp_link_t *lp_link_add(lp_link_t *L, void *data)
{
    lp_link_t *s = 0; // p指向表头
    if (0 == L)
        return 0;
    s = (lp_link_t *)lpbsp_malloc(sizeof(lp_link_t));
    if (s == 0)
        return 0;
    s->data = data; // 复制线程名字
    s->next = L->next;
    L->next = (lp_link_t *)s;
    return s;
}

/// @brief 删除链表
/// @param phead  指向一个链表指针，此处传入表头地址
/// @return
int lp_link_del(lp_link_t *phead)
{
    lp_link_t *p; // p指向第一个结点
    lp_link_t *s = 0;
    if (0 == phead)
        return 1;

    p = (lp_link_t *)phead->next; // p指向第一个结点
    while (p != phead)
    {
        s = p;
        lpbsp_free(p->data);
        p = (lp_link_t *)p->next;
        lpbsp_free(s);
    } // p未到表头
    lpbsp_free(p->data);
    lpbsp_free(p);
    return 0;
}

/// @brief 该函数的功能是删除链表一个节点
/// @param phead 链表句柄
/// @param data 节点
/// @return
int lp_link_remove(lp_link_t *phead, void *data)
{
    lp_link_t *p1;     // p1保存当前需要检查的节点的地址
    lp_link_t *p2 = 0; // p2保存当前检查过的节点的地址
    if (phead == 0)    // 是空链表
    {
        return 1;
    }
    p1 = phead;                               // 定位要删除的节点
    while (p1->data != data && p1->next != 0) // p1指向的节点不是所要查找的，并且它不是最后一个节点，就继续往下找
    {
        p2 = p1;                    // 保存当前节点的地址
        p1 = (lp_link_t *)p1->next; // 后移一个节点
    }
    if (p1->data == data) // 找到了
    {
        if (p1 == phead) // 如果要删除的节点是第一个节点
        {
            phead = (lp_link_t *)p1->next; // 头指针指向第一个节点的后一个节点，也就是第二个节点。这样第一个节点就不在链表中，即删�?
        }
        else // 如果是其它节点，则让原来指向当前节点的指针，指向它的下一个节点，完成删除
        {
            p2->next = p1->next;
        }
        lpbsp_free(p1->data); // 释放内存
        lpbsp_free(p1);       // 释放当前节点
    }
    else // 没有找到
    {
        return 1;
    }
    return 0;
}

/// @brief 返回单链表中元素的数量
/// @param phead 链表句柄
/// @return
int lp_link_len(lp_link_t *phead)
{
    int i = 1;
    if (0 == phead)
        return 0;
    lp_link_t *p = (lp_link_t *)phead->next;
    while (p != phead)
    {
        i++;
        p = (lp_link_t *)p->next;
    }
    return i;
}

///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////
/*----------------------------------------------------------------------------/
 /  los -  system module  R0.1.1
 /-----------------------------------------------------------------------------/
 /
 / Copyright (c) 2014-2017 LP电子,All Rights Reserved.Author's mailbox:lgtbp@126.com.
 /
 / GNU LESSER GENERAL PUBLIC LICENSE  Version 2.1
 /----------------------------------------------------------------------------*/
static fun_os *func = 0;
static int func_size = 0;
uint32_t losgram_addr;

#define RAM_GLOBAL // 开启这个宏，可以直接操作非los内的内存

// #define LOS_DG_CMD

// #define LOS_DEBUG_CALL
// #define LOS_DEBUG_CMD 1
// #define LOS_DEBUG 1

#define SOC_32_BIT 1
#ifdef SOC_32_BIT
#define POINT_SIZE 4
#define POINT_TYPE uint32_t
#else
#define POINT_SIZE 8
#define POINT_TYPE uint64_t
#endif
struct heap
{
    uint8_t los[4];
    uint16_t version;
    uint16_t pad_len;
    uint32_t stack_len;
    uint32_t heap_len;
    uint32_t txt_len;
    uint32_t global_len;
    uint32_t gvar_init_len;
    uint32_t code_len;
    uint32_t code_main;
};

static char los_next_app_pname[32] = {0};
char *los_next_app_get(void)
{
    return los_next_app_pname;
}
int los_next_app_set(char *p)
{
    if (p)
        strcpy(los_next_app_pname, p);
    else
        los_next_app_pname[0] = 0;
    return 0;
}
#ifdef LOS_DEBUG
static void cmd_err(los_t *lp, int err)
{
    longjmp(lp->jbuf, 100 + err);
}
#endif
static void cmd_0(los_t *lp, uint8_t arg)
{
    longjmp(lp->jbuf, 99);
}
static void cmd_1(los_t *lp, uint8_t arg)
{
    lp->stack_end -= CREGLEN;
    memcpy(lp->reg, &lp->gram[lp->stack_end], CREGLEN);
    lp->stack_end -= POINT_SIZE;
    lp->pc = (uint8_t *)(*(POINT_TYPE *)&lp->gram[lp->stack_end]);
}
static void cmd_2(los_t *lp, uint8_t arg)
{
    lp->reg[HEAP_REG].u32 -= arg;
#ifdef LOS_DEBUG
    if (lp->stack_end > lp->reg[HEAP_REG].u32)
    {
        cmd_err(lp, 2);
    }
#endif
}
static void cmd_3(los_t *lp, uint8_t arg)
{
    lp->reg[HEAP_REG].u32 += arg;
    cmd_1(lp, 0);
}
static void cmd_4(los_t *lp, uint8_t arg)
{
    lp->reg[HEAP_REG].u32 -= arg + (*(uint16_t *)lp->pc << 8);
    lp->pc += 2;
#ifdef LOS_DEBUG
    if (lp->stack_end > lp->reg[HEAP_REG].u32)
    {
        cmd_err(lp, 4);
    }
#endif
}
static void cmd_5(los_t *lp, uint8_t arg)
{
    lp->reg[HEAP_REG].u32 += arg + (*(uint16_t *)lp->pc << 8);
    lp->pc += 2;
    cmd_1(lp, 0);
}
static void cmd_6(los_t *lp, uint8_t arg)
{
#ifdef LOS_DEBUG
    cmd_err(lp, 6);
#endif
    lp->reg[arg & 0xf].u32 = arg >> 4;
}
static void cmd_7(los_t *lp, uint8_t arg)
{
    lp->reg[arg].u32 = *(uint16_t *)lp->pc;
    lp->pc += 2;
}
static void cmd_8(los_t *lp, uint8_t arg)
{
    memcpy(lp->reg[arg].data, lp->pc, 4);
    lp->pc += 4;
}
static void cmd_9(los_t *lp, uint8_t arg)
{
#ifdef RAM_GLOBAL
    int addr = lp->reg[arg & 0x7].u32 + *(uint16_t *)lp->pc;
    // printf("%d<>%d\r\n", lp->ram_len, addr);
    if (lp->ram_len < addr)
        *(uint32_t *)addr = lp->reg[arg >> 3].u32;
    else
        *(uint32_t *)(&lp->gram[addr]) = lp->reg[arg >> 3].u32;
#else
    *(uint32_t *)(&lp->gram[lp->reg[arg & 0x7].u32 + *(uint16_t *)lp->pc]) = lp->reg[arg >> 3].u32;
#endif
    lp->pc += 2;
}
static void cmd_10(los_t *lp, uint8_t arg)
{
#ifdef RAM_GLOBAL
    int addr = lp->reg[arg & 0x7].u32 + *(uint16_t *)lp->pc;
    // printf("%d<>%d\r\n", lp->ram_len, addr);
    if (lp->ram_len < addr)
        *(uint8_t *)addr = lp->reg[arg >> 3].data[0];
    else
        lp->gram[addr] = lp->reg[arg >> 3].data[0];
#else
    lp->gram[lp->reg[arg & 0x7].u32 + *(uint16_t *)lp->pc] = lp->reg[arg >> 3].data[0];
#endif
    lp->pc += 2;
}
static void cmd_11(los_t *lp, uint8_t arg)
{
#ifdef RAM_GLOBAL
    int addr = lp->reg[arg & 0x7].u32 + *(uint16_t *)lp->pc;
    // printf("%d<>%d\r\n", lp->ram_len, addr);
    if (lp->ram_len < addr)
        memcpy((uint8_t *)addr, lp->reg[arg >> 3].data, 2);
    else
        memcpy(&lp->gram[addr], lp->reg[arg >> 3].data, 2);
#else
    memcpy(&lp->gram[lp->reg[arg & 0x7].u32 + *(uint16_t *)lp->pc], lp->reg[arg >> 3].data, 2);
#endif
    lp->pc += 2;
}
static void cmd_12(los_t *lp, uint8_t arg)
{
#ifdef RAM_GLOBAL
    int addr = lp->reg[arg & 0x7].s32 + *(uint16_t *)lp->pc;
    if (lp->ram_len < addr)
        lp->reg[arg >> 3].u32 = *(uint32_t *)addr;
    else
        lp->reg[arg >> 3].u32 = *(uint32_t *)&lp->gram[addr];
#else
    lp->reg[arg >> 3].u32 = *(uint32_t *)&lp->gram[lp->reg[arg & 0x7].s32 + *(uint16_t *)lp->pc];
#endif
    lp->pc += 2;
}
static void cmd_13(los_t *lp, uint8_t arg)
{
#ifdef RAM_GLOBAL
    int addr = lp->reg[arg & 0x7].s32 + *(uint16_t *)lp->pc;
    if (lp->ram_len < addr)
        lp->reg[arg >> 3].u32 = *(uint8_t *)addr;
    else
        lp->reg[arg >> 3].s32 = *(uint8_t *)&lp->gram[addr];
#else
    lp->reg[arg >> 3].u32 = *(uint8_t *)&lp->gram[lp->reg[arg & 0x7].s32 + *(uint16_t *)lp->pc];
#endif
    lp->pc += 2;
}
static void cmd_14(los_t *lp, uint8_t arg)
{
#ifdef RAM_GLOBAL
    int addr = lp->reg[arg & 0x7].u32 + *(uint16_t *)lp->pc;
    if (lp->ram_len < addr)
        lp->reg[arg >> 3].u32 = *(uint16_t *)addr;
    else
        lp->reg[arg >> 3].s32 = *(uint16_t *)&lp->gram[addr];
#else
    lp->reg[arg >> 3].u32 = *(uint16_t *)&lp->gram[lp->reg[arg & 0x7].u32 + *(uint16_t *)lp->pc];
#endif
    lp->pc += 2;
}
static void cmd_15(los_t *lp, uint8_t arg)
{
#ifdef RAM_GLOBAL
    int addr = lp->reg[arg & 0x7].u32 + *(uint16_t *)lp->pc;
    if (lp->ram_len < addr)
        lp->reg[arg >> 3].u32 = *(int8_t *)addr;
    else
        lp->reg[arg >> 3].s32 = *(int8_t *)&lp->gram[addr];
#else
    lp->reg[arg >> 3].s32 = *(int8_t *)&lp->gram[lp->reg[arg & 0x7].u32 + *(uint16_t *)lp->pc];
#endif
    lp->pc += 2;
}
static void cmd_16(los_t *lp, uint8_t arg)
{
#ifdef RAM_GLOBAL
    int addr = lp->reg[arg & 0x7].u32 + *(uint16_t *)lp->pc;
    if (lp->ram_len < addr)
        lp->reg[arg >> 3].u32 = *(int16_t *)addr;
    else
        lp->reg[arg >> 3].u32 = *(int16_t *)&lp->gram[addr];
#else
    lp->reg[arg >> 3].s32 = *(int16_t *)&lp->gram[lp->reg[arg & 0x7].u32 + *(uint16_t *)lp->pc];
#endif
    lp->pc += 2;
}
static void cmd_17(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 3].u32 = lp->reg[arg & 0x7].u32;
}
static void cmd_18(los_t *lp, uint8_t arg)
{
    lp->reg[arg & 0xf].u32 = (arg >> 4) + (*(uint16_t *)lp->pc << 4);
    lp->pc += 2;
}
static void cmd_19(los_t *lp, uint8_t arg)
{
    lp->reg[arg & 0xf].u32 = 0xfffff + (arg >> 4) + (*(uint16_t *)lp->pc << 4);
    lp->pc += 2;
}
static void cmd_20(los_t *lp, uint8_t arg)
{
#ifdef LOS_DEBUG
    cmd_err(lp, 20);
#endif
}

static void cmd_21(los_t *lp, uint8_t arg)
{
    uint32_t call_addr = arg + (*(uint16_t *)lp->pc << 8);
    lp->pc += 2;
    *(POINT_TYPE *)&lp->gram[lp->stack_end] = (POINT_TYPE)(&lp->pc[0]);
    lp->stack_end += POINT_SIZE;
    lp->pc = &lp->code[call_addr];
    memcpy(&lp->gram[lp->stack_end], lp->reg, CREGLEN);
    lp->stack_end += CREGLEN;
#ifdef LOS_DEBUG
    if (lp->stack_end > lp->reg[HEAP_REG].u32)
    {
        cmd_err(lp, 21);
    }
#endif
}
static void cmd_22(los_t *lp, uint8_t arg)
{
    *(POINT_TYPE *)&lp->gram[lp->stack_end] = (POINT_TYPE)lp->pc;
    lp->stack_end += POINT_SIZE;
    lp->pc = &lp->code[lp->reg[arg].u32];
    memcpy(&lp->gram[lp->stack_end], lp->reg, CREGLEN);
    lp->stack_end += CREGLEN;
#ifdef LOS_DEBUG
    if (lp->stack_end > lp->reg[HEAP_REG].u32)
    {
        cmd_err(lp, 22);
    }
#endif
}
static void cmd_23(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 4].u32 += lp->reg[arg & 0xf].u32;
}
static void cmd_24(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 + (*(uint16_t *)lp->pc);
    lp->pc += 2;
}
static void cmd_25(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 + *(uint16_t *)lp->pc + (*(uint16_t *)(lp->pc + 2) << 16);
    lp->pc += 4;
}
static void cmd_26(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 - lp->reg[arg >> 4].u32;
}
static void cmd_27(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 - (*(uint16_t *)lp->pc);
    lp->pc += 2;
}
static void cmd_28(los_t *lp, uint8_t arg)
{
#ifdef LOS_DEBUG
    cmd_err(lp, 28);
#endif
    lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 - ((*(uint16_t *)lp->pc + (*(uint16_t *)(lp->pc + 2) << 16)));
    lp->pc += 4;
}
static void cmd_29(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 4].u32 ^= lp->reg[arg & 0xf].u32;
}
static void cmd_30(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 ^ (*(uint16_t *)lp->pc);
    lp->pc += 2;
}
static void cmd_31(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 ^ (*(uint16_t *)lp->pc + (*(uint16_t *)(lp->pc + 2) << 16));
    lp->pc += 4;
}
static void cmd_32(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 4].u32 |= lp->reg[arg & 0xf].u32;
}
static void cmd_33(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 | (*(uint16_t *)lp->pc);
    lp->pc += 2;
}
static void cmd_34(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 | (*(uint16_t *)lp->pc + (*(uint16_t *)(lp->pc + 2) << 16));
    lp->pc += 4;
}
static void cmd_35(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 << lp->reg[arg >> 4].u32;
}
static void cmd_36(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 << (*(uint16_t *)lp->pc);
    lp->pc += 2;
}
static void cmd_37(los_t *lp, uint8_t arg)
{
#ifdef LOS_DEBUG
    cmd_err(lp, 37);
#endif
    lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 << (*(uint16_t *)lp->pc + (*(uint16_t *)(lp->pc + 2) << 16));
    lp->pc += 4;
}
static void cmd_38(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 >> lp->reg[arg >> 4].u32;
}
static void cmd_39(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 >> (*(uint16_t *)lp->pc);
    lp->pc += 2;
}
static void cmd_40(los_t *lp, uint8_t arg)
{
#ifdef LOS_DEBUG
    cmd_err(lp, 40);
#endif
    lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 >> (*(uint16_t *)lp->pc + (*(uint16_t *)(lp->pc + 2) << 16));
    lp->pc += 4;
}
static void cmd_41(los_t *lp, uint8_t arg)
{
    uint32_t mov = lp->reg[arg >> 4].u32;
    if (lp->reg[arg & 0xf].u32 & 0x80000000)
    {
        lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 >> mov;
        if (mov > 31)
        {
            lp->reg[arg >> 4].u32 = 0x80000000;
        }
        else
            lp->reg[arg >> 4].u32 |= (0xffffffff << (32 - mov));
    }
    else
        lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 >> lp->reg[arg >> 4].u32;
}
static void cmd_42(los_t *lp, uint8_t arg)
{
    uint16_t mov = (*(uint16_t *)lp->pc);
    if (lp->reg[arg & 0xf].u32 & 0x80000000)
    {
        lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 >> mov;
        lp->reg[arg >> 4].u32 |= (0xffffffff << (32 - mov));
#ifdef LOS_DEBUG
        if (mov > 31)
        {
            cmd_err(lp, 42);
        }
#endif
    }
    else
        lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 >> mov;
    lp->pc += 2;
}
static void cmd_43(los_t *lp, uint8_t arg)
{
#ifdef LOS_DEBUG
    cmd_err(lp, 43);
#endif
    lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 >> (*(uint16_t *)lp->pc + (*(uint16_t *)(lp->pc + 2) << 16));
    lp->pc += 4;
}
static void cmd_44(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 * lp->reg[arg >> 4].u32;
}
static void cmd_45(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 * (*(uint16_t *)lp->pc);
    lp->pc += 2;
}
static void cmd_46(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 * (*(uint16_t *)lp->pc + (*(uint16_t *)(lp->pc + 2) << 16));
    lp->pc += 4;
}
static void cmd_47(los_t *lp, uint8_t arg)
{
#ifdef LOS_DEBUG
    cmd_err(lp, 46);
#endif
}
static void cmd_48(los_t *lp, uint8_t arg)
{
#ifdef LOS_DEBUG
    cmd_err(lp, 47);
#endif
}
static void cmd_49(los_t *lp, uint8_t arg)
{
#ifdef LOS_DEBUG
    cmd_err(lp, 48);
#endif
}
static void cmd_50(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 / lp->reg[arg >> 4].u32;
}
static void cmd_51(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 / (*(uint16_t *)lp->pc);
    lp->pc += 2;
}
static void cmd_52(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 / (*(uint16_t *)lp->pc + (*(uint16_t *)(lp->pc + 2) << 16));
    lp->pc += 4;
}
static void cmd_53(los_t *lp, uint8_t arg)
{
#ifdef LOS_DEBUG
    if (0 == lp->reg[arg >> 4].s32)
    {
        cmd_err(lp, 53);
    }
#endif
    lp->reg[arg >> 4].s32 = lp->reg[arg & 0xf].s32 / lp->reg[arg >> 4].s32;
}
static void cmd_54(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 4].s32 = lp->reg[arg & 0xf].s32 / (*(uint16_t *)lp->pc);
    lp->pc += 2;
}
static void cmd_55(los_t *lp, uint8_t arg)
{
#ifdef LOS_DEBUG
    cmd_err(lp, 55);
#endif
    lp->reg[arg >> 4].s32 = lp->reg[arg & 0xf].s32 / (int32_t)(*(uint16_t *)lp->pc + (*(uint16_t *)(lp->pc + 2) << 16));
    lp->pc += 4;
}
static void cmd_56(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 4].u32 &= lp->reg[arg & 0xf].u32;
}
static void cmd_57(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 & (*(uint16_t *)lp->pc);
    lp->pc += 2;
}
static void cmd_58(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 4].u32 = lp->reg[arg & 0xf].u32 & (*(uint16_t *)lp->pc + (*(uint16_t *)(lp->pc + 2) << 16));
    lp->pc += 4;
}
static void cmd_59(los_t *lp, uint8_t arg)
{
    lp->pc += (uint8_t)arg;
}
static void cmd_60(los_t *lp, uint8_t arg)
{
    lp->pc -= (uint8_t)arg;
}
static void cmd_61(los_t *lp, uint8_t arg)
{
    lp->reg[arg >> 5].u32 = lp->reg[arg & 0x1F].u32;
}
static void cmd_62(los_t *lp, uint8_t arg)
{
    lp->pc += *(int16_t *)(lp->pc);
}
static void cmd_63(los_t *lp, uint8_t arg)
{
    lp->reg[*lp->pc].u32 = lp->reg[arg].u32;
    lp->pc += 2;
}
static void cmd_64(los_t *lp, uint8_t arg)
{
#ifdef LOS_DEBUG
    cmd_err(lp, 64);
#endif
}
static void cmd_65(los_t *lp, uint8_t arg)
{
    int32_t imm = *(int16_t *)(lp->pc);
    lp->pc += 2;
    if (lp->reg[arg & 0xf].s32 == lp->reg[arg >> 4].s32)
        lp->pc += imm;
}
static void cmd_66(los_t *lp, uint8_t arg)
{
    int32_t cmp = *(int16_t *)(lp->pc);
    int32_t imm = *(int16_t *)(&lp->pc[2]);
    lp->pc += 4;
    if (lp->reg[arg].s32 == cmp)
        lp->pc += imm;
}
static void cmd_67(los_t *lp, uint8_t arg)
{
    int32_t imm = *(int16_t *)(lp->pc);
    lp->pc += 2;
    if (lp->reg[arg & 0xf].s32 != lp->reg[arg >> 4].s32)
        lp->pc += imm;
}
static void cmd_68(los_t *lp, uint8_t arg)
{
    int32_t cmp = *(int16_t *)(lp->pc);
    int32_t imm = *(int16_t *)(&lp->pc[2]);
    lp->pc += 4;
    if (lp->reg[arg].s32 != cmp)
        lp->pc += imm;
}
static void cmd_69(los_t *lp, uint8_t arg)
{
    int32_t imm = *(int16_t *)(lp->pc);
    lp->pc += 2;
    if (lp->reg[arg >> 4].u32 > lp->reg[arg & 0xf].u32)
        lp->pc += imm;
}
static void cmd_70(los_t *lp, uint8_t arg)
{
    int32_t cmp = *(int16_t *)(lp->pc);
    int32_t imm = *(int16_t *)(&lp->pc[2]);
    lp->pc += 4;
    if (lp->reg[arg].u32 > cmp)
        lp->pc += imm;
}
static void cmd_71(los_t *lp, uint8_t arg)
{
    int32_t imm = *(int16_t *)(lp->pc);
    lp->pc += 2;
    if (lp->reg[arg >> 4].u32 >= lp->reg[arg & 0xf].u32)
        lp->pc += imm;
}
static void cmd_72(los_t *lp, uint8_t arg)
{
    int32_t cmp = *(int16_t *)(lp->pc);
    int32_t imm = *(int16_t *)(&lp->pc[2]);
    lp->pc += 4;
    if (lp->reg[arg].u32 >= cmp)
        lp->pc += imm;
}
static void cmd_73(los_t *lp, uint8_t arg)
{
    int32_t imm = *(int16_t *)(lp->pc);
    lp->pc += 2;
    if (lp->reg[arg >> 4].s32 > lp->reg[arg & 0xf].s32)
        lp->pc += imm;
}
static void cmd_74(los_t *lp, uint8_t arg)
{
    int32_t cmp = *(int16_t *)(lp->pc);
    int32_t imm = *(int16_t *)(&lp->pc[2]);
    lp->pc += 4;
    if (lp->reg[arg].s32 > cmp)
        lp->pc += imm;
}
static void cmd_75(los_t *lp, uint8_t arg)
{
    int32_t imm = *(int16_t *)(lp->pc);
    lp->pc += 2;
    if (lp->reg[arg >> 4].s32 >= lp->reg[arg & 0xf].s32)
        lp->pc += imm;
}
static void cmd_76(los_t *lp, uint8_t arg)
{
    int32_t cmp = *(int16_t *)(lp->pc);
    int32_t imm = *(int16_t *)(&lp->pc[2]);
    lp->pc += 4;
    if (lp->reg[arg].s32 >= cmp)
        lp->pc += imm;
}
static void cmd_77(los_t *lp, uint8_t arg)
{
    int32_t imm = *(int16_t *)(lp->pc);
    lp->pc += 2;
    if (lp->reg[arg >> 4].u32 < lp->reg[arg & 0xf].u32)
        lp->pc += imm;
}
static void cmd_78(los_t *lp, uint8_t arg)
{
    int32_t cmp = *(int16_t *)(lp->pc);
    int32_t imm = *(int16_t *)(&lp->pc[2]);
    lp->pc += 4;
    if (lp->reg[arg].u32 < cmp)
        lp->pc += imm;
}
static void cmd_79(los_t *lp, uint8_t arg)
{
    int32_t imm = *(int16_t *)(lp->pc);
    lp->pc += 2;
    if (lp->reg[arg >> 4].u32 <= lp->reg[arg & 0xf].u32)
        lp->pc += imm;
}
static void cmd_80(los_t *lp, uint8_t arg)
{
    int32_t cmp = *(int16_t *)(lp->pc);
    int32_t imm = *(int16_t *)(&lp->pc[2]);
    lp->pc += 4;
    if (lp->reg[arg].u32 <= cmp)
        lp->pc += imm;
}
static void cmd_81(los_t *lp, uint8_t arg)
{
    int32_t imm = *(int16_t *)(lp->pc);
    lp->pc += 2;
    if (lp->reg[arg >> 4].s32 < lp->reg[arg & 0xf].s32)
        lp->pc += imm;
}
static void cmd_82(los_t *lp, uint8_t arg)
{
    int32_t cmp = *(int16_t *)(lp->pc);
    int32_t imm = *(int16_t *)(&lp->pc[2]);
    lp->pc += 4;
    if (lp->reg[arg].s32 < cmp)
        lp->pc += imm;
}
static void cmd_83(los_t *lp, uint8_t arg)
{
    int32_t imm = *(int16_t *)(lp->pc);
    lp->pc += 2;
    if (lp->reg[arg >> 4].s32 <= lp->reg[arg & 0xf].s32)
        lp->pc += imm;
}
static void cmd_84(los_t *lp, uint8_t arg)
{
    int32_t cmp = *(int16_t *)(lp->pc);
    int32_t imm = *(int16_t *)(&lp->pc[2]);
    lp->pc += 4;
    if (lp->reg[arg].s32 <= cmp)
        lp->pc += imm;
}
static void cmd_85(los_t *lp, uint8_t arg)
{
#ifdef LOS_DEBUG
    cmd_err(lp, 85);
#endif
}
static void cmd_86(los_t *lp, uint8_t arg)
{
#ifdef LOS_DEBUG
    cmd_err(lp, 86);
#endif
}
static void cmd_87(los_t *lp, uint8_t arg)
{
    lp->reg[arg & 0xf].s32 = (int32_t)((arg >> 4) - 16);
}
static void cmd_88(los_t *lp, uint8_t arg)
{
    int len = ((*(uint16_t *)lp->pc) << 8) + arg;
    lp->pc += 2;
#ifdef LOS_DEBUG_CALL
    printf("call %x\r\n", len);
#endif
    if (func[len])
        func[len](lp);
#ifdef LOS_DEBUG_CALL
    printf("\tcall end %d\r\n", len);
#endif
}
static void cmd_89(los_t *lp, uint8_t arg)
{
    lp->reg[HEAP_REG].u32 += arg;
    longjmp(lp->jbuf, 89);
}
const los_cmd los_cmd_api[] = {
    cmd_0,
    cmd_1,
    cmd_2,
    cmd_3,
    cmd_4,
    cmd_5,
    cmd_6,
    cmd_7,
    cmd_8,
    cmd_9,
    cmd_10,
    cmd_11,
    cmd_12,
    cmd_13,
    cmd_14,
    cmd_15,
    cmd_16,
    cmd_17,
    cmd_18,
    cmd_19,
    cmd_20,
    cmd_21,
    cmd_22,
    cmd_23,
    cmd_24,
    cmd_25,
    cmd_26,
    cmd_27,
    cmd_28,
    cmd_29,
    cmd_30,
    cmd_31,
    cmd_32,
    cmd_33,
    cmd_34,
    cmd_35,
    cmd_36,
    cmd_37,
    cmd_38,
    cmd_39,
    cmd_40,
    cmd_41,
    cmd_42,
    cmd_43,
    cmd_44,
    cmd_45,
    cmd_46,
    cmd_47,
    cmd_48,
    cmd_49,
    cmd_50,
    cmd_51,
    cmd_52,
    cmd_53,
    cmd_54,
    cmd_55,
    cmd_56,
    cmd_57,
    cmd_58,
    cmd_59,
    cmd_60,
    cmd_61,
    cmd_62,
    cmd_63,
    cmd_64,
    cmd_65,
    cmd_66,
    cmd_67,
    cmd_68,
    cmd_69,
    cmd_70,
    cmd_71,
    cmd_72,
    cmd_73,
    cmd_74,
    cmd_75,
    cmd_76,
    cmd_77,
    cmd_78,
    cmd_79,
    cmd_80,
    cmd_81,
    cmd_82,
    cmd_83,
    cmd_84,
    cmd_85,
    cmd_86,
    cmd_87,
    cmd_88,
    cmd_89,
};
int los_call_addr(los_t *lp, uint32_t call_addr, int _argc, uint32_t *_argv)
{
    int nums = 0;
    uint32_t baddr = 0, slen = 0;
    uint8_t i = 0, arg, res;

    if (0 == lp)
        return 0;

    baddr = (POINT_TYPE)(&lp->pc[0]);
    slen = lp->stack_end;
    *(POINT_TYPE *)&lp->gram[lp->stack_end] = (POINT_TYPE)(&lp->pc[0]);
    lp->stack_end += POINT_SIZE;
    lp->pc = &lp->code[call_addr];
    memcpy(&lp->gram[lp->stack_end], lp->reg, CREGLEN);
    lp->stack_end += CREGLEN;
    for (; i < _argc; i++)
    {
        lp->reg[8 + i].u32 = _argv[i];
    }
    while (1)
    {
        if (slen == lp->stack_end && baddr == (POINT_TYPE)(&lp->pc[0]))
        {
            _argv[0] = lp->reg[REG_RETURN].u32;
            // printf("call_addr[%x]init out=%d\r\n", call_addr, nums); //又回到那个地址了,还要判断内存位置
            return 0;
        }
        nums++;
        arg = *(lp->pc + 1);
        res = *lp->pc;
#ifdef LOS_DEBUG_CMD
        if (res > ARRLEN(los_cmd_api))
        {
            printf("err init op[%d]\r\n", res); // 出错并不会中断程序，需要修改
            return 4;
        }
        printf("[init][%d][%x]", res, arg);
        for (size_t i = 0; i < 15; i++)
        {
            printf("%d,", lp->reg[i].u32);
        }
        // printf("[%d]",*(uint32_t*)&lp->gram[1056584]);
        printf("\n");
#endif
        lp->pc += 2;
        los_cmd_api[res](lp, arg);
    }
    return 0;
}
int los_irq(los_t *lp, int num)
{
    int res;

    if (0 == lp)
        return 0;

    if (0 == lp->los_irq)
        return 0;
    // lp->arg = arg;
    memcpy(&lp->gram[lp->stack_end], &lp->arg, sizeof(los_irq_len_t));
    lp->stack_end += sizeof(los_irq_len_t);
    lp->pc = lp->code;
    res = setjmp(lp->jbuf);
    if (res)
    {
        lp->stack_end -= sizeof(los_irq_len_t);
        memcpy(&lp->arg, &lp->gram[lp->stack_end], sizeof(los_irq_len_t));
        // arg = lp->arg;
        return 0;
    }
    lp->reg[8].u32 = num;
    res = los_app_run(lp);
    return res;
}
int los_function_add(fun_os *f, int size)
{
    int ret = 0;
    uint8_t *temp = 0;

    ret = sizeof(fun_os *) * size;
    if (func)
    {
        temp = lpbsp_malloc(func_size + ret);
        if (0 == temp)
            return 1;
        memcpy(temp, func, func_size);
        memcpy(&temp[func_size], f, ret);

        lpbsp_free(func);
        func = temp;
        func_size += ret;
    }
    else
    {
        func = lpbsp_malloc(size * sizeof(fun_os *));
        if (0 == func)
            return 1;
        func_size = ret;
        memcpy(func, f, func_size);
    }
    return 0;
}
static uint8_t los_init_code(los_t *lp, uint32_t *data, uint8_t inter)
{
    uint32_t temp = 0;
    struct heap *los_heap;
    los_heap = (struct heap *)data;
    // los_heap->stack_len = los_heap->stack_len;
#ifdef LOS_DEBUG
    printf("int size %d\r\n", sizeof(int));
    printf("inter = %d\r\n", inter);
    printf("size %d\r\n", sizeof(struct heap));
    printf("version=%x\r\n", los_heap->version);
    printf("pad_len=%d\r\n", los_heap->pad_len);
    printf("stack_len=%d\r\n", los_heap->stack_len);
    printf("heap_len=%d\r\n", los_heap->heap_len);
    printf("txt_len=%d\r\n", los_heap->txt_len);
    printf("gvar_init_len=%d\r\n", los_heap->gvar_init_len);
    printf("global_len=%d\r\n", los_heap->global_len);
    printf("code_len=%d\r\n", los_heap->code_len);
#endif
    lp->los_irq = (los_heap->code_main & 0x80000000) > 0 ? 1 : 0;
    los_heap->code_main &= 0x7fffffff;
    if (0 != strcmp((char *)los_heap->los, "Los"))
        return 0xff;
    if (los_api_vs < los_heap->version)
        return 0xfe;

    lp->code_len = los_heap->code_len;
    lp->data_len = los_heap->txt_len;
    lp->bss_len = los_heap->txt_len + los_heap->gvar_init_len;

    lp->lvar_start = los_heap->txt_len + los_heap->gvar_init_len + los_heap->global_len;
    lp->stack_end = lp->lvar_start;
    temp = lp->lvar_start + los_heap->stack_len;
    temp = temp + los_heap->heap_len;
    lp->reg[HEAP_REG].u32 = temp;
    if (inter)
    {
        lp->ram_len = temp;
        lp->ram = (uint8_t *)lpbsp_malloc(temp);
        if (0 == lp->ram)
        {
            printf("losmalloc error len=%ld\r\n", temp);
            return 3;
        }
        memset(lp->ram, 0, temp);
        memcpy(lp->ram, (uint8_t *)&data[9] + los_heap->code_len, los_heap->txt_len + los_heap->gvar_init_len);
        lp->code = (uint8_t *)&data[9];
        lp->gram = lp->ram - los_heap->code_len;
        lp->pc = lp->code + los_heap->code_main;

        lp->reg[HEAP_REG].u32 += los_heap->code_len;
        lp->stack_end += los_heap->code_len;
    }
    else
    {
        lp->ram_len = temp + los_heap->code_len;
        lp->ram = (uint8_t *)lpbsp_malloc(temp + los_heap->code_len);
        if (0 == lp->ram)
            return 4;
        memset(lp->ram, 0, temp + los_heap->code_len);
        memcpy(lp->ram, &data[9], los_heap->txt_len + los_heap->gvar_init_len + los_heap->code_len);
        lp->code = lp->ram;
        lp->gram = lp->ram;
        lp->pc = lp->code + los_heap->code_main;

        lp->reg[HEAP_REG].u32 += los_heap->code_len;
        lp->stack_end += los_heap->code_len;
        lpbsp_free(data);
    }
#ifdef LOS_DEBUG
    printf("lp->ram_len %d\r\n", lp->ram_len);
#endif
    return 0;
}
int los_app_run(los_t *lp)
{
#ifdef LOS_DEBUG_CMD
    int cmd_num = 0;
#endif
    int res;
    uint8_t arg;
    losgram_addr = (uint32_t)lp->gram;
    res = setjmp(lp->jbuf);
    if (res)
    {
        return res;
    }
    for (;;)
    {
        arg = *(lp->pc + 1);
        res = *lp->pc;
#ifdef LOS_DG_CMD
        if (res > ARRLEN(los_cmd_api))
        {
            printf("err op[%d]\r\n", res);
            return 4;
        }
#endif
#ifdef LOS_DEBUG_CMD
        printf("[%d][%d][%x]:\r\n", cmd_num++, res, arg);
#endif
#if LOS_DEBUG == 2
        for (size_t i = 0; i < 15; i++)
        {
            printf("%x,", lp->reg[i].u32);
        }
        printf("\r\n");
#endif
        lp->pc += 2;
        los_cmd_api[res](lp, arg);
        // printf("v=%x\n",*(uint32_t *)(&lp->gram[10188]));
#if LOS_DEBUG == 2
        printf("\t[%d][%x]:", res, arg);
        for (size_t i = 0; i < 15; i++)
        {
            printf("%x,", lp->reg[i].u32);
        }
        printf("\r\n");
#endif
    }
    return res;
}
los_t *los_app_new(uint32_t *addr, uint8_t inter)
{
    int ret = 0;
    if (addr == 0)
        return 0;
    los_t *lp = (los_t *)lpbsp_malloc(sizeof(los_t));
    if (0 == lp)
        return 0;
    memset(lp, 0, sizeof(los_t));
    ret = los_init_code(lp, addr, inter);
    if (ret)
    {
#ifdef LOS_DEBUG_CMD
        printf("los_init_code[%x]:\r\n", ret);
#endif
        lpbsp_free(lp);
        return 0;
    }
    lp->malloc = lp_link_new();
    return lp;
}

int los_app_close(los_t *lp)
{
    lp_link_del(lp->malloc);
    lpbsp_free(lp->ram);
    lpbsp_free(lp);
    return 0;
}

void los_set_return(los_t *lp, uint32_t data)
{
    lp->reg[REG_RETURN].u32 = data;
}
void los_set_struct(los_t *lp, uint8_t *data, int len)
{
    memcpy((uint8_t *)(&lp->gram[lp->reg[8].u32]), data, len);
}

void *los_get_struct(los_t *lp, uint32_t num)
{
    return (void *)(&lp->gram[lp->reg[8 + num].u32]);
}
uint8_t *los_get_ram(los_t *lp)
{
    return lp->gram;
}
uint8_t los_get_u8(los_t *lp, uint32_t num)
{
    return lp->reg[8 + num].u8;
}
int8_t los_get_s8(los_t *lp, uint32_t num)
{
    return lp->reg[8 + num].s8;
}
uint16_t los_get_u16(los_t *lp, uint32_t num)
{
    return lp->reg[8 + num].u16;
}
int16_t los_get_s16(los_t *lp, uint32_t num)
{
    return lp->reg[8 + num].s16;
}
uint32_t los_get_u32(los_t *lp, uint32_t num)
{
    return lp->reg[8 + num].u32;
}
int32_t los_get_s32(los_t *lp, uint32_t num)
{
    return lp->reg[8 + num].s32;
}
void *los_get_voidp(los_t *lp, uint32_t num)
{
    return (void *)(&lp->gram[lp->reg[8 + num].u32]);
}
void *los_get_p(los_t *lp, uint32_t num)
{
    if (lp->reg[8 + num].u32 > lp->ram_len) // 可能有些问题,code在内部rom的话,ram_len就不包含code的长度了
        return (void *)lp->reg[8 + num].u32;
    else
        return (void *)(&lp->gram[lp->reg[8 + num].u32]);
}
uint8_t *los_get_u8p(los_t *lp, uint32_t num)
{
    return (uint8_t *)(&lp->gram[lp->reg[8 + num].u32]);
}
int8_t *los_get_s8p(los_t *lp, uint32_t num)
{
    return (int8_t *)(&lp->gram[lp->reg[8 + num].u32]);
}
int16_t *los_get_s16p(los_t *lp, uint32_t num)
{
    return (int16_t *)(&lp->gram[lp->reg[8 + num].u32]);
}
uint16_t *los_get_u16p(los_t *lp, uint32_t num)
{
    return (uint16_t *)(&lp->gram[lp->reg[8 + num].u32]);
}
uint32_t *los_get_u32p(los_t *lp, uint32_t num)
{
    return (uint32_t *)(&lp->gram[lp->reg[8 + num].u32]);
}
int32_t *los_get_s32p(los_t *lp, uint32_t num)
{
    return (int32_t *)(&lp->gram[lp->reg[8 + num].u32]);
}
#if 0
void *los_get_p2(los_t *lp, uint32_t num, int offset)
{
	printf("P2P[%d]=%x,%x\r\n",offset,lp->reg[8 + num].u32,(*(uint32_t *)&lp->gram[lp->reg[8 + num].u32 + offset * 4]));
	return (void *)(&lp->gram[(*(uint32_t *)&lp->gram[lp->reg[8 + num].u32 + offset * 4])]);
}
#else
void *los_get_p2(los_t *lp, uint32_t num, int offset)
{
    // printf("P2P[%d]=%x,%x\r\n", offset, lp->reg[8 + num].u32, (*(uint32_t *)&lp->gram[lp->reg[8 + num].u32 + offset * 4]));
    return (void *)(&lp->gram[(*(uint32_t *)&lp->gram[lp->reg[8 + num].u32 + offset * 4])]);
}
#endif
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////////////////////
static lplib_rtc_ram_t *lplib_rtc_ram;
/// @brief rtc 内存初始化
/// @return
int lplib_rtc_ram_init(void)
{
    int ram_size = 0;

    lplib_rtc_ram = (lplib_rtc_ram_t *)lpbsp_rtc_ram_get(&ram_size);
    if (0 == lplib_rtc_ram)
        return 1;

    if (0xa5a55a5a != lplib_rtc_ram->flg)
    {
        lplib_rtc_ram->flg = 0xa5a55a5a;
        lplib_rtc_ram->dt = lpbsp_rtc_get();
        lplib_rtc_ram->ram_now = 0;
        lplib_rtc_ram->zip_len = 0;
        lplib_rtc_ram->zip_start = 0;
    }
    lplib_rtc_ram->ram_len = ram_size - sizeof(lplib_rtc_ram_t);
    return 0;
}
/// @brief 添加一个数据 存储格式 time type data
/// @param data 添加内容  type + 数据
/// @param len 数据长度
/// @return
int lplib_rtc_ram_add(uint8_t *data, int len)
{
    uint32_t dt = 0;
    int dt_gap = 0, num;
    uint8_t *tbuf8;

    if (0 == lplib_rtc_ram)
        return 1;

    if ((lplib_rtc_ram->ram_now + len) > lplib_rtc_ram->ram_len)
    {
        lplib_rtc_ram_save();
    }

    tbuf8 = (uint8_t *)((uint32_t)lplib_rtc_ram + sizeof(lplib_rtc_ram_t));
    dt = lpbsp_rtc_get();
    dt_gap = dt - lplib_rtc_ram->dt;
    num = lplib_rtc_ram->ram_now;
    if (0 > dt_gap)
    {
        lplib_rtc_ram->dt = dt;
        lplib_rtc_ram->zip_len = 0;
        lplib_rtc_ram->zip_start = 0;
        dt_gap = 0;
        LP_LOGD("rtc ram add err[%d - %d]\r\n", dt, dt_gap);
    }
    tbuf8[num] = dt_gap;
    tbuf8[num + 1] = dt_gap >> 8;
    memcpy(&tbuf8[num + 2], data, len);
    lplib_rtc_ram->ram_now += 2 + len;

    return 0;
}
/// @brief 立刻保存到文件系统去
/// @return
int lplib_rtc_ram_save(void)
{
    int tbuf_len = 0;
    uint8_t *tbuf8, *tbuf;
    char fname[40], dtstr[20];

    if (0 == lplib_rtc_ram)
        return 1;

    if (0 == lplib_rtc_ram->ram_now)
        return 2;

    tbuf_len = lplib_rtc_ram->ram_now + 4;
    tbuf = lpbsp_malloc(8192);
    tbuf8 = (uint8_t *)((uint32_t)lplib_rtc_ram + sizeof(lplib_rtc_ram_t) - 4);

    memcpy(tbuf, tbuf8, tbuf_len);
    lp_time_utc2string(lplib_rtc_ram->dt, dtstr, 2);
    sprintf(fname, "/C/App/%s.txt", dtstr);

    LP_LOGD("rtc ram file  [%s - %d]\r\n", fname, tbuf_len);
    lp_save(fname, tbuf, tbuf_len);

    lplib_rtc_ram->dt = lpbsp_rtc_get();
    lpbsp_free(tbuf);
    lplib_rtc_ram->ram_now = 0;
    return 0;
}
/*
0 low mode
1 tft on light mode
2 tft on deep mode
*/
int lplib_power_mode_get(void)
{

    return 0;
}
int lplib_power_mode_set(int type)
{

    return 0;
}

int lplib_language_set(char *lang)
{
    return 0;
}
char *lplib_language_get(void)
{
    return 0;
}
int lplib_bl_time_set(int sec)
{
    return 0;
}
int lplib_bl_time_get(void)
{
    return 0;
}
int lplib_bl_value_set(int value)
{
    return 0;
}
int lplib_bl_value_get(void)
{
    return 0;
}
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////