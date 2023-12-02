
#include "lvgl_inc.h"

static lv_timer_t *clock_timer;

static lv_obj_t *clock_tv = 0;
static lv_obj_t *clock_2_hh;
static lv_obj_t *clock_2_mm;
static lv_obj_t *clock_3_lab;
extern const lv_font_t clock_nor_110;
extern const lv_font_t clock_nor_80;
/////////////////////////////////////////////////////////////////////////////////
static lv_obj_t *sleep_hour = 0, *sleep_minute = 0;
static void clock_timer_cb(lv_timer_t *t)
{
	datetime_t dt_now;

	lp_time_get(&dt_now);

	lv_img_set_angle(sleep_hour, ((dt_now.tm_hour % 12) * 3600 / 12 + dt_now.tm_min * 5 + 2700) % 3600);
	lv_img_set_angle(sleep_minute, (dt_now.tm_min * 60 + 2700) % 3600);

	lp_lvgl_label_set_text_fmt(clock_2_hh, "%02d", dt_now.tm_hour);
	lp_lvgl_label_set_text_fmt(clock_2_mm, "%02d", dt_now.tm_min);

	lp_lvgl_label_set_text_fmt(clock_3_lab, "%02d:%02d", dt_now.tm_hour, dt_now.tm_min);
}

/// @brief 时钟被删除了
/// @param e 事件
static void clock_event_cb(lv_event_t *e)
{
	lv_event_code_t code = lv_event_get_code(e);

	if (code == LV_EVENT_DELETE)
	{
		lv_timer_del(clock_timer);
		clock_timer = 0;
	}
}
///////////////////////////////////////////////////////////////////////////////
/// @brief 指针表盘--这也是上拉的预览，但没开定时器，上拉预览时钟是不走
/// @param src_obj
int view_page_clock_3(lv_obj_t *sleep_clock_src)
{
	LV_IMG_DECLARE(clock_bg);
	LV_IMG_DECLARE(lvgl_img_hand_min)
	LV_IMG_DECLARE(lvgl_img_hand_hour)

	lv_obj_set_style_bg_opa(sleep_clock_src, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(sleep_clock_src, lv_color_make(255, 255, 255), 0);

	lv_obj_t *image = lv_img_create(sleep_clock_src);
	lv_img_set_src(image, &clock_bg);
	lv_obj_align(image, LV_ALIGN_TOP_MID, 0, 20);

	sleep_hour = lv_img_create(sleep_clock_src);
	lv_img_set_src(sleep_hour, &lvgl_img_hand_hour);
	lv_obj_set_pos(sleep_hour, 120 - 3, 140 - 4);
	lv_img_set_pivot(sleep_hour, 3, 4);

	sleep_minute = lv_img_create(sleep_clock_src);
	lv_img_set_src(sleep_minute, &lvgl_img_hand_min);
	lv_obj_set_pos(sleep_minute, 120 - 3, 140 - 4);
	lv_img_set_pivot(sleep_minute, 3, 4);

	datetime_t dt_now; // 更新一下时间  --上拉的时候，只有这一次初始化了

	lp_time_get(&dt_now);
	lv_img_set_angle(sleep_hour, ((dt_now.tm_hour % 12) * 3600 / 12 + dt_now.tm_min * 5 + 1800) % 3600);
	lv_img_set_angle(sleep_minute, (dt_now.tm_sec * 60 + 1800) % 3600);
	return 0;
}

/// <summary>
/// 演示page
/// </summary>
/// <param name="src">page obj</param>
/// <param name="name">page name</param>
/// <returns>返回</returns>
static int view_page_clock_2(lv_obj_t *src)
{
	clock_3_lab = lv_label_create(src);

	static lv_style_t font_style; // 定义LV_STYLE
	lv_style_init(&font_style);	  // 初始化LV_STYLE
	// 下面的部分就是用来修饰你的style的

	lv_style_set_text_font(&font_style, &clock_nor_80);					   // 设置字体大小为48
	lv_style_set_text_color(&font_style, lv_color_make(0x00, 0xff, 0x00)); // 设置字体颜色为绿色
	lv_style_set_text_align(&font_style, LV_TEXT_ALIGN_CENTER);

	// lv_label_set_text(clock_3_lab, name);
	lv_obj_set_pos(clock_3_lab, 0, 20);
	lv_obj_set_width(clock_3_lab, 240);

	lv_obj_add_style(clock_3_lab, &font_style, LV_PART_ANY);	  // 给label的所有部分
	lv_obj_add_style(clock_3_lab, &font_style, LV_STATE_DEFAULT); // 给label的默认状态

	lv_obj_t *arc = lv_arc_create(src);
	lv_obj_set_pos(arc, 30, 100);
	lv_obj_set_size(arc, 180, 180);
	lv_arc_set_rotation(arc, 135);
	lv_arc_set_bg_angles(arc, 0, 270);
	lv_arc_set_value(arc, 40);
	lv_obj_clear_flag(arc, LV_OBJ_FLAG_CLICKABLE);
	return 0;
}

int view_page_clock_1(lv_obj_t *src)
{
	datetime_t dt_now;
	char tbuf[4];
	lv_obj_t *bar_bat;
	int temp = 0;

#if 0
	//lv_obj_remove_style_all(src);
	lv_obj_set_style_bg_opa(src, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(src, lv_color_make(0, 0, 0), 0); // 全黑背景
#endif
	lp_time_get(&dt_now);

	clock_2_hh = lv_label_create(src);
	clock_2_mm = lv_label_create(src);

	lp_lvgl_label_set_text_fmt(clock_2_hh, "%02d", dt_now.tm_hour);
	lp_lvgl_label_set_text_fmt(clock_2_mm, "%02d", dt_now.tm_min);

	lv_obj_set_pos(clock_2_hh, 0, 20);
	lv_obj_set_width(clock_2_hh, 240);

	lv_obj_set_pos(clock_2_mm, 0, 130);
	lv_obj_set_width(clock_2_mm, 240);

	lv_obj_set_style_text_align(clock_2_hh, LV_TEXT_ALIGN_CENTER, 0);
	lv_obj_set_style_text_align(clock_2_mm, LV_TEXT_ALIGN_CENTER, 0);

	lv_obj_set_style_text_font(clock_2_hh, &clock_nor_110, 0);
	lv_obj_set_style_text_font(clock_2_mm, &clock_nor_110, 0);

	bar_bat = lv_bar_create(src);
	lv_obj_set_pos(bar_bat, 230, 150);
	lv_obj_set_size(bar_bat, 7, 60);
	lv_obj_set_style_border_color(bar_bat, lv_color_make(0, 0xff, 0xff), 0);
	lv_obj_set_style_border_width(bar_bat, 1, 0);
	lv_bar_set_value(bar_bat, lp_bat_value_get() % 100, 0);

	bar_bat = lv_bar_create(src);
	lv_obj_set_pos(bar_bat, 0, 220);
	lv_obj_set_size(bar_bat, 240, 5);
	lv_obj_set_style_border_color(bar_bat, lv_color_make(0, 0xff, 0xff), 0);
	lv_obj_set_style_border_width(bar_bat, 1, 0);

	temp = lp_sensor_accelerometer_step_read();
	if (temp > 20000) //  20000 -30000 或更多
	{
		temp -= 20000;
		lv_obj_set_style_bg_color(bar_bat, lv_color_make(0x00, 0xff, 0x00), LV_PART_INDICATOR);
		lv_bar_set_value(bar_bat, temp / 100, 0);
	}
	if (temp > 10000) // 10000 - 20000
	{
		temp -= 10000;
		lv_obj_set_style_bg_color(bar_bat, lv_color_make(0xff, 0x00, 0x00), LV_PART_INDICATOR);
		lv_bar_set_value(bar_bat, temp / 100, 0);
	}
	else // 0-10000
	{
		lv_bar_set_value(bar_bat, temp / 100, 0);
	}
	return 0;
}
/// @brief
/// @param src
/// @return
int view_page_clock_idie(lv_obj_t *src)
{
	lv_obj_t *clock_idie = 0, *bar_bat;
	datetime_t dt_now;
	uint32_t temp = 0;
	lv_obj_t *obj_base = 0;

	obj_base = lv_obj_create(src);
	lv_obj_remove_style_all(obj_base);
	lv_obj_set_pos(obj_base, 0, 120);
	lv_obj_set_size(obj_base, 240, 65);

	lp_time_get(&dt_now);
	clock_idie = lv_label_create(obj_base);
	lp_lvgl_label_set_text_fmt(clock_idie, "%02d:%02d", dt_now.tm_hour, dt_now.tm_min);
	lv_obj_set_pos(clock_idie, 20, 0);

	lv_obj_set_style_text_font(clock_idie, &clock_nor_80, 0);
	lv_obj_set_style_bg_opa(obj_base, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(obj_base, lv_color_make(0, 0, 0), 0); // 全黑背景
	lv_obj_set_style_text_color(obj_base, lv_color_make(0xff, 0xff, 0xff), LV_PART_MAIN);

	bar_bat = lv_bar_create(obj_base);
	lv_obj_set_pos(bar_bat, 230, 0);
	lv_obj_set_size(bar_bat, 7, 60);
	lv_obj_set_style_border_color(bar_bat, lv_color_make(0, 0xff, 0xff), 0);
	lv_obj_set_style_border_width(bar_bat, 1, 0);
	lv_bar_set_value(bar_bat, lp_bat_value_get() % 100, 0);

	bar_bat = lv_bar_create(obj_base);
	lv_obj_set_pos(bar_bat, 0, 60);
	lv_obj_set_size(bar_bat, 240, 5);
	lv_obj_set_style_border_color(bar_bat, lv_color_make(0, 0xff, 0xff), 0);
	lv_obj_set_style_border_width(bar_bat, 1, 0);

	temp = lp_sensor_accelerometer_step_read();
	if (temp > 20000) //  20000 -30000 或更多
	{
		temp -= 20000;
		lv_obj_set_style_bg_color(bar_bat, lv_color_make(0x00, 0xff, 0x00), LV_PART_INDICATOR);
		lv_bar_set_value(bar_bat, temp / 100, 0);
	}
	if (temp > 10000) // 10000 - 20000
	{
		temp -= 10000;
		lv_obj_set_style_bg_color(bar_bat, lv_color_make(0xff, 0x00, 0x00), LV_PART_INDICATOR);
		lv_bar_set_value(bar_bat, temp / 100, 0);
	}
	else // 0-10000
	{
		lv_bar_set_value(bar_bat, temp / 100, 0);
	}
	return 0;
}
///////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////////

/// <summary>
/// 开机主界面
/// </summary>
/// <param name="obj"></param>
/// <returns></returns>
int view_page_main_clock(lv_obj_t *src_obj)
{
	lv_obj_t *obj;

	clock_tv = lp_lvgl_page_clock_create(src_obj, 3);
	lv_obj_add_event_cb(clock_tv, clock_event_cb, LV_EVENT_DELETE, 0);

	obj = lp_lvgl_page_clock_get(clock_tv, 0);
	lv_obj_set_style_bg_opa(obj, LV_OPA_COVER, 0);
	lv_obj_set_style_bg_color(obj, lv_color_make(255, 255, 255), 0);
	view_page_clock_1(obj);

	obj = lp_lvgl_page_clock_get(clock_tv, 1);
	view_page_clock_2(obj);

	obj = lp_lvgl_page_clock_get(clock_tv, 2);
	view_page_clock_3(obj);

	clock_timer_cb(0);
	clock_timer = lv_timer_create(clock_timer_cb, 1000, NULL); // 真的在clock界面才开启时钟

	return 0;
}