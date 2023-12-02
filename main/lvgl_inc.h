#ifndef _VIEW_PAGE_H
#define _VIEW_PAGE_H

#include "lvgl.h"
#include "math.h"
#include "lpbsp.h"
#include "stdio.h"
#include "string.h"
#include "lplib.h"
#include "lpverify.h"
#ifdef WIN32
#define SYS_INIT_FLG 1
#else
#define SYS_INIT_FLG 0
#endif

#if WIN32
#define PRINTF_NONE "\033[m"
#define PRINTF_CYAN "\033[0;36m"
#define PRINTF_GREEN "\033[0;32;32m"
#define PRINTF_RED "\033[0;32;31m"
#define APP_LOGD(format, ...) printf(PRINTF_CYAN "%08d,%32s:%04d " format PRINTF_NONE, lpbsp_tick_ms(), __FILE__, __LINE__, ##__VA_ARGS__);
#define APP_LOGW(format, ...) printf(PRINTF_GREEN "%08d,%32s:%04d " format PRINTF_NONE, lpbsp_tick_ms(), __FILE__, __LINE__, ##__VA_ARGS__);
#define APP_LOGE(format, ...) printf(PRINTF_RED "%08d,%32s:%04d " format PRINTF_NONE, lpbsp_tick_ms(), __FILE__, __LINE__, ##__VA_ARGS__);
#else
#define APP_LOGD(...)
#define APP_LOGW(...)
#define APP_LOGE(...)
#endif
// page 任务不返回
int init_page_task(void *pvParameter);
int lvgl_sleep_time_ms(int ms);
int view_page_clock_1(lv_obj_t *scr);
int view_page_clock_idie(lv_obj_t *src);
int view_page_main_clock(lv_obj_t *src_obj);
int view_page_setting(lv_obj_t *last_obj);
int view_page_version(lv_obj_t *scr);
int view_page_wifi_setting(lv_obj_t *scr);
int view_page_display_setting(lv_obj_t *scr);
int view_page_general(lv_obj_t *scr);
int view_page_calendar(lv_obj_t *scr);
int view_page_alarm(lv_obj_t *scr);
int view_page_map(lv_obj_t *scr);
int view_page_test(lv_obj_t *last_obj);
int view_page_sport(lv_obj_t *last_obj);
int view_page_up(lv_obj_t *last_obj);
int view_page_calculator(lv_obj_t *src_obj);
int view_page_compass(lv_obj_t *last_src);
int view_page_file_explorer(lv_obj_t *last_src);
int view_page_strong(lv_obj_t *last_src);
#endif
