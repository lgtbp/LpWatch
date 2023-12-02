
#ifndef __LP_LIB_H
#define __LP_LIB_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "math.h"
#include "lpbsp.h"
#include "string.h"
#include "stdint.h"
#include "stdio.h"
#include "lvgl.h"
#include "src/core/lv_obj.h"
#include "src/core/lv_indev.h"

#define ARR_SIZE(a) (sizeof(a) / sizeof(a[0]))

    enum LF_RETRUN_E
    {
        LF_OK = 0,             // 成功
        LF_ERR,                // 失败
        LF_ERR_PARAMETER,      // 参数错误
        LF_ERR_PARAMETER_NULL, // 参数错误
        LF_ERR_MALLOC,         // 申请内存失败
        LF_ERR_OPEN_FILE,      // 打开文件失败
        LF_ERR_READ_FILE,      // 读取文件失败
        LF_ERR_WRITE_FILE,     // 写入文件失败
        LF_ERR_SOCKET_CONNECT, // socket链接失败
        LF_ERR_SOCKET_WRITE,   // socket 写失败
        LF_FILE_NO_DRIVER_NUM, // 没有这盘符
        LF_FILE_NO_FUNC_CB     // 没注册这函数

    };

    enum LP_FUNC_CB_E
    {
        LP_FUNC_CB_ERR = 0, // 失败
        LP_FUNC_CB_START,   // 开始
        LP_FUNC_CB_DATA,    // 数据/进度
        LP_FUNC_CB_END,     // 结束
    };

    typedef struct __datetime_t
    {
        unsigned char tm_sec;   /* 秒 – 取值区间为[0,59] */
        unsigned char tm_min;   /* 分 - 取值区间为[0,59] */
        unsigned char tm_hour;  /* 时 - 取值区间为[0,23] */
        unsigned char tm_mday;  /* 一个月中的日期 - 取值区间为[1,31] */
        unsigned char tm_mon;   /* 月份（从一月开始，0代表一月） - 取值区间为[0,11] */
        unsigned char tm_wday;  /* 星期 – 取值区间为[0,6]，其中0代表星期天，1代表星期一，以此类推 */
        unsigned char tm_isdst; /* 夏令时标识符，实行夏令时的时候，为正。不实行夏令时的时候，为0；不了解情况时，为负。*/
        unsigned char tm_tms;   /*10毫秒*/

        int tm_year;         /* 年份，其值等于实际年份减去1900 */
        const char *tm_zone; /*当前时区的名字(与环境变量TZ有关)*/
    } datetime_t;

    int lp_time_get(datetime_t *dt);
    int lp_time_set(datetime_t *dt);
    /// @brief  升级app pfile文件
    /// @param pfile 文件名字
    /// @param prog_cb 升级进度回调
    /// @return
    int lp_ota_up_form_file(char *pfile, int (*prog_cb)(uint32_t type, uint32_t value));

    /// @brief 升级app
    /// @param data 数据buf
    /// @param type LP_FUNC_CB_E
    /// @param value 根据LP_FUNC_CB_E
    /// @return
    int lp_ota_up_form_data(uint8_t *data, uint32_t type, uint32_t value);

    void touch_int_get_xy(void);
    uint32_t lp_time_utc_get(void);
    int lp_time_utc_set(uint32_t utc);
    void lp_rtc_to_time(unsigned int rtc, datetime_t *tm);
    unsigned int lp_time_to_rtc(datetime_t *tm);
    int lp_sensor_accelerometer_step_read(void);
//////////////////////////////////////////////////////////////////////////////////
#define TFT_OBJ_HEIGHT 75                                 // 一行obj的高度
#define TFT_OBJ_GAP 3                                     // 间隔
#define TFT_OBJ_HEIGHT_GAP (TFT_OBJ_HEIGHT + TFT_OBJ_GAP) // 一行obj的高度+行高

    typedef int (*lp_lvgl_page_cb)(lv_obj_t *last_obj);
    typedef struct __lpbsp_main_page_t
    {
        uint8_t num;
        char **name;
        lp_lvgl_page_cb *api;
    } lpbsp_main_api_t;

    typedef void (*lp_lvgl_page_input_cb_t)(void *arg, char *input_ole, char *input_new);
    typedef void (*lp_lvgl_page_view_cb_t)(lv_obj_t *obj);
    typedef void (*lp_lvgl_page_cb_t)(int32_t id, uint32_t value);
    typedef void (*lp_lvgl_ui_notify_t)(void *arg);

    typedef struct
    {
        char *title;   // 显示title
        char *data;    // 显示data
        uint32_t time; // 时间
    } lp_lvgl_msg_tip_t;

    /// @brief 主时间界面-用于在非clock界面下-上拉的显示
    /// @param cb
    void lp_lvgl_page_view_clock_set(lp_lvgl_page_view_cb_t cb);

    /// @brief 主界面app列表-用于在clock界面下-上拉的显示
    /// @param cb
    void lp_lvgl_page_view_main_set(lp_lvgl_page_view_cb_t cb);

    /// @brief 主时间界面-用于在非clock界面下-上拉的初始化
    /// @param cb
    void lp_lvgl_page_view_clock_app_set(lp_lvgl_page_view_cb_t cb);

    /// @brief 主界面app列表-用于在clock界面下-上拉的初始化
    /// @param cb
    void lp_lvgl_page_view_main_app_set(lp_lvgl_page_view_cb_t cb);

    /// @brief 主界面app列表-设置页面的回调
    /// @param cb
    void lp_lvgl_page_view_main_api_set(lpbsp_main_api_t *lpbsp_main_api);

    /// @brief 设置系统的bar是否出现
    /// @param eanble 1出现 0不出现
    void lp_lvgl_page_sysbar_set(uint32_t eanble);

    /// @brief 新建一个page
    /// @param parent 它的父亲
    /// @return page的obj
    lv_obj_t *lp_lvgl_page_create(lv_obj_t *parent);

    /// @brief 设置obj被关闭的回调
    /// @param tv obj
    /// @param cb 回调函数
    void lp_lvgl_page_close_cb_set(lv_obj_t *tv, lp_lvgl_page_cb_t cb);

    /// @brief 用户app有权利去改它的显示
    /// @param data 显示字符
    /// @return 0ok 1error
    int lp_lvgl_page_sysmenu_text_set(char *data);
    /// @brief 为app创建一个新的page
    /// @param src page下面的子obj
    /// @param type 1带状态栏 0不带状态栏的
    /// @return 返回用户可操作区域obj
    lv_obj_t *lp_lvgl_page_app_create_title(lv_obj_t *src, int type);
    /// @brief 新建一个page
    /// @param parent 它的父亲
    /// @param type  1 特殊的界面不带状态栏的 0普通的app page带状态栏的
    /// @return page的obj
    lv_obj_t *lp_lvgl_page_app_create(lv_obj_t *parent, uint8_t type);
    /// @brief 新建一个page-默认参数
    /// @param parent 它的父亲
    /// @return
    lv_obj_t *lp_lvgl_page_app_create_def(lv_obj_t *parent);
    /// @brief 新建一个page
    /// @param parent 它的父亲
    /// @param type  多小个时钟
    /// @return page的obj
    lv_obj_t *lp_lvgl_page_clock_create(lv_obj_t *parent, uint8_t count);
    /// @brief 获取指定的clock界面
    /// @param parent
    /// @param count 指定num
    /// @return 返回obj
    lv_obj_t *lp_lvgl_page_clock_get(lv_obj_t *parent, uint8_t count);
    /// @brief 新建一个page的title
    /// @param tv obj
    /// @param col_id 列
    /// @param row_id 行
    /// @param dir 能获得的方向
    /// @return 新建的obj
    lv_obj_t *lp_lvgl_page_add_tile(lv_obj_t *tv, uint8_t col_id, uint8_t row_id, lv_dir_t dir);

    /// @brief 添加下一个界面
    /// @param tv 首页?
    /// @return next obj
    lv_obj_t *lp_lvgl_page_add_next(lv_obj_t *tv);

    /// @brief 添加下一个界面-带初始化函数
    /// @param cb 页面初始化
    /// @param bar_eanble 是否显示bar
    /// @return next obj
    lv_obj_t *lp_lvgl_page_add_next_with_init(lp_lvgl_page_view_cb_t cb, int bar_eanble);

    /// @brief 添加下一个界面-没有系统条的
    /// @param tv 首页?
    /// @return next obj
    lv_obj_t *lp_lvgl_page_add_next_no_sysbar(lv_obj_t *tv);

    /// @brief 返回上一个页面
    /// @param tv 首页?
    /// @return 0 ok,1 no page can be back
    int lp_lvgl_page_back(lv_obj_t *tv);

    /// @brief 刷新当前界面
    /// @param tv
    /// @return
    int lp_lvgl_page_refresh(void);

    /// @brief 现在设置的page -- page只有3个的时候 这函数不一定起作用.迷幻
    /// @param obj 当前空间
    /// @param tile_obj 需要显示的page
    /// @param anim_en 是否滑动过去
    void lp_lvgl_page_set_tile(lv_obj_t *obj, lv_obj_t *tile_obj, lv_anim_enable_t anim_en);
    /// @brief 现在设置的page
    /// @param tv 当前空间
    /// @param col_id 列
    /// @param row_id 行
    /// @param anim_en 是否滑动过去
    void lp_lvgl_page_set_tile_id(lv_obj_t *tv, uint32_t col_id, uint32_t row_id, lv_anim_enable_t anim_en);

    lv_obj_t *lp_lvgl_page_get_tile_act(lv_obj_t *obj);

    lv_obj_t *lp_lvgl_page_father_get(lv_obj_t *obj);
    //
    lv_obj_t *lp_lvgl_page_father_get(lv_obj_t *obj);
    /// @brief 这是app删除的，或可以做隐藏它
    /// @param obj page下面的子obj
    /// @return 0 ok
    int lp_lvgl_page_app_del(lv_obj_t *obj);

    /// @brief 这是app删除的，或可以做隐藏它
    /// @param obj page下面的子obj
    /// @return 0 ok
    int lp_lvgl_page_app_del(lv_obj_t *obj);

    /// @brief 设置label text,但先检查是否原先就是一样的
    /// @param obj
    /// @param text
    void lp_lvgl_label_set_text(lv_obj_t *obj, const char *text);

    /// @brief 设置label text,但先检查是否原先就是一样的
    /// @param obj
    /// @param fmt
    void lp_lvgl_label_set_text_fmt(lv_obj_t *obj, const char *fmt, ...);
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////
    ///////////////////////////////////LVGL驱动//////////////////////////////////////////////////////////
    ///////////////////////////////////////////////////////////////////////////////////////////////////

    /// <summary>
    /// lvgl 初始化
    /// </summary>
    /// <param name=""></param>
    int lp_lvgl_init(int draw_buf_size);

    /// @brief 发送tip提醒
    /// @param title 标题
    /// @param data 内容
    /// @return 0 ok
    int lp_lvgl_msg_tip_send(char *title, char *data);

    /////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////

    /// @brief 上传文件
    /// @param url 网址
    /// @param fname 存储文件路径
    /// @return 0:ok 1:err
    int lp_lvgl_upfile(char *url, char *fname);

    /// @brief 下载文件-有些服务器会下载不成功
    /// @param url 网址-需要支持分包下载
    /// @param fname 存储文件路径
    /// @return 0:ok 1:err
    int lp_lvgl_down2file(char *url, char *fname);

    /// @brief 下载文件-数据流-有些服务器会下载不成功
    /// @param url 网址-需要支持分包下载
    /// @param prog_cb 数据流
    /// @return 0:ok 1:err
    int lp_lvgl_down2data(char *url, int (*prog_cb)(uint8_t *data, uint32_t type, uint32_t value));

    /// @brief 执行长时间操作带界面进度的
    /// @param title 名字
    /// @param show_txt 显示内存
    /// @param arg 函数参数
    /// @param prog_cb 函数
    /// @return
    int lp_lvgl_func_progress(char *title, char *show_txt, void *arg, int (*prog_cb)(void *arg, void *func_prog));

    /// @brief 询问是否要执行
    /// @param title 名字
    /// @param show_txt 显示内容
    /// @param arg 函数参数
    /// @param func_cb 函数
    /// @return
    int lp_lvgl_func_run(char *title, char *show_txt, void *arg, int (*func_cb)(void *arg));

    /// @brief lvgl 错误提醒
    /// @param code  code
    /// @return
    int lp_lvgl_msg_err_code(char *title, int code);

    /// @brief 输入对话框
    /// @param ole 输入显示buf
    /// @param cb 输入完成回调
    /// @return 0:ok 1:err
    int lp_lvgl_input(void *arg, char *ole, lp_lvgl_page_input_cb_t cb);
    int lp_lvgl_input_en(void *arg, char *ole, lp_lvgl_page_input_cb_t cb);
    int lp_lvgl_input_pinyin(void *arg, char *ole, lp_lvgl_page_input_cb_t cb);

    /////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////////////////////////////////////////////////////////
    /// @brief 消息通知
    /// @param title 标题
    /// @param msg 内容
    /// @return
    int lp_lvgl_page_notify(char *title, char *msg);

    ////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////
    typedef struct
    {
        lp_lvgl_ui_notify_t cb;
        void *arg;
    } lp_lvgl_ui_msg_t;

    int lp_lvgl_ui_notify_run(void);

    int lp_lvgl_ui_notify_send(lp_lvgl_ui_notify_t cb, void *arg);

    ////////////////////////////////////////////////////////////////
    /*
    static uint32_t i18n_crc[] = {1212, 12121, 12121};
    static char *mylang[] = {"en", "zh-cn", "zh-hk"};
    static char *mydata_en[] = {"Start", "Rerst", "Count"};
    static char *mydata_cn[] = {"设置", "升级", "地图"};
    static char *mydata_hk[] = {"设设", "级级", "图图"};

    static char *mydata[] = {mydata_en, mydata_cn, mydata_hk};
    static lp_i18n_t my_i18n = {
    .lang_num = 3,
    .data_num = 3,
    .crc = i18n_crc,
    .lang = mylang,
    .data = mydata,
    };
    */
    typedef struct _lp_i18n_t
    {
        unsigned short lang_num; // lang_num =2
        unsigned short data_num; // data_num=3
        unsigned int *crc;       // uint32_t crc[]={1212,12121,123};
        char **lang;             // char *lang={"en","zh-cn"};
        char **data;             // char *data={ {"a","b","c"},{"A","B","C"}};
    } lp_i18n_t;

    /// @brief 设置位置
    /// @param loction country code
    /// @return
    int lp_i18n_location_set(char *loction);
    /// @brief 获取已经设置的语言
    /// @param loction buf mini 8byte
    /// @return
    int lp_i18n_location_get(char *loction);

    /// @brief 语言文本
    /// @param profile - 路径
    /// @return
    int lp_i18n_profile_set(char *profile);

    /// @brief 获取存在的语言
    /// @param index
    /// @return
    char *lp_i18n_lang_get(uint32_t index);

    /// @brief 返回语言数量
    /// @param
    /// @return 数量
    int lp_i18n_lang_num_get(void);

    /// @brief 语言数据设置
    /// @param profile
    /// @return
    int lp_i18n_set(lp_i18n_t *p);

    /// @brief 用英语的获取设置的语言
    /// @param en_str
    /// @return
    char *lp_i18n_string(char *en_str);

    /// @brief 用英语的获取设置的语言-多线程不安全
    /// @param ico ico code
    /// @param en_str
    /// @return
    char *lp_i18n_string_ico(char *ico, char *en_str);
    //////////////////////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////
    /////////////////////////////////// fifo //////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////////////////

    typedef struct
    {
        uint16_t fifo_top;  // 队顶指针
        uint16_t fifo_bot;  // 队底指针
        uint16_t fifo_size; // 队长度
        uint16_t fifo_len;  // 缓冲区长度
        uint8_t *fifo_buff; // 缓冲区
    } lp_fifo_t;

    /**
     * @brief  初始化FIFO
     * @param  p_fifo I 初始化FIFO的结构体
     * @param  p_buf  I FIFO的buf
     * @param  buf_size  I FIFO的buf的size
     * @retval 0 成功入
     */
    int lp_fifo_init(lp_fifo_t *p_fifo, uint8_t *p_buf, uint16_t buf_size);

    /**
     * @brief  初始化FIFO
     * @param  p_fifo I 初始化FIFO的结构体
     * @param  buf_size  I FIFO的buf的size
     * @retval 0 成功,1 malloc err
     */
    int lp_fifo_malloc_init(lp_fifo_t *p_fifo, uint16_t buf_size);

    /**
     * @brief  反初始化FIFO
     * @param  p_fifo I 初始化FIFO的结构体
     * @retval 0 成功,1 malloc err
     */
    int lp_fifo_malloc_deinit(lp_fifo_t *p_fifo);

    /**
     * @brief  将数据压入FIFO
     * @param  p_fifo I FIFO的结构体
     * @param  data I 待入FIFO的数据
     * @param  len  I 待入FIFO的数据量
     * @retval 1 成功入fifo 0 fifo空间不足
     */
    int lp_fifo_write(lp_fifo_t *p_fifo, uint8_t *data, int len);

    /**
     * \brief  获取数据，并弹出FIFO
     * @param  p_fifo I FIFO的结构体
     * @param  buf O 数据输出地址
     * @param  size I 要出FIFO的个数
     * @retval 1成功 0失败
     */
    int lp_fifo_read(lp_fifo_t *p_fifo, uint8_t *buf, int size);

#define lp_fifo_write_int(fifo, x) lp_fifo_write(fifo, &x, 4)
#define lp_fifo_read_int(fifo, x) lp_fifo_read(fifo, &x, 4)

    /**
     * \brief  尝试读一个字节出来，并把长度发回
     * @param  p_fifo I FIFO的结构体
     * @param  buf O 数据输出
     * @retval 长度
     */
    int lp_fifo_peek(lp_fifo_t *p_fifo, uint8_t *buf);

    /**
     * @brief  将数据压入FIFO (写len 带长度的 byte)
     * @param  p_fifo I FIFO的结构体
     * @param  data I 待入FIFO的数据
     * @param  len  I 待入FIFO的数据量
     * @retval 1 成功入fifo 0 fifo空间不足
     */
    int lp_fifo_msg_set(lp_fifo_t *p_fifo, uint8_t *data, int len);

    /**
     * \brief  获取数据(第一个字节是后面数据的长度)
     * @param  p_fifo I FIFO的结构体
     * @param  out O 数据输出地址
     * @param  max_out I 要出FIFO的个数
     * @retval 1成功 0失败
     */
    int lp_fifo_msg_get(lp_fifo_t *p_fifo, uint8_t *out, int max_out);

    /**
     * \brief  清空fifo
     * @param  p_fifo I FIFO的结构体
     * @retval 1成功 0失败
     */
    int lp_fifo_clear(lp_fifo_t *p_fifo);

    ///////////////////////////////////////////////////////////////////////
    /**
     * 判断一年是否为闰年
     * @param year a year
     * @return 0: not leap year; 1: leap year
     */
    uint8_t lp_time_is_leap_year(uint32_t year);
    /**
     * Get the number of days in a month
     * @param year a year
     * @param month a month. The range is basically [1..12] but [-11..0] or [13..24] is also
     *              supported to handle next/prev. year
     * @return [28..31]
     */
    uint8_t lp_time_month_length_get(int32_t year, int32_t month);

    /**
     * 这天是星期几
     * @param year a year
     * @param month a  month [1..12]
     * @param day a day [1..32]
     * @return [0..6] which means [Sun..Sat] or [Mon..Sun] depending on 日历周从周一开始
     */
    uint8_t lp_time_day_of_week_get(uint32_t year, uint32_t month, uint32_t day);
    /**
     * utc转time时间
     * @param rtc I  utc时间
     * @param tm O time时间
     */
    void lp_rtc_to_time(unsigned int rtc, datetime_t *tm);

    /**
     *  time时间转utc
     * @param tm I time时间
     * @return 0: utc时间
     */
    unsigned int lp_time_to_rtc(datetime_t *tm);
    uint32_t lp_time_utc_get(void);
    int lp_time_utc_set(uint32_t utc);
    int lp_time_get(datetime_t *dt);
    int lp_time_set(datetime_t *dt);
    int lp_bat_value_get(void);

    /// 读取步数
    int lp_sensor_accelerometer_step_read(void);
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////FAT////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////
#define LF_DRIVER_NUM 4
    typedef struct _lp_file_t
    {
        void *file;
        uint8_t driver_num; // 哪一个驱动上的
        uint16_t err_code;  // 出错代码
    } lp_file_t;

#define LF_TYPE_NO 0
#define LF_TYPE_FILE 1
#define LF_TYPE_DIR 2

#define FILE_NAME_MAX 32
    typedef struct _lp_dirent_t
    {
        unsigned int ino;             /* inode number 索引节点号 */
        unsigned short off;           /* offset to this dirent 在目录文件中的偏移 */
        unsigned short reclen;        /* length of this d_name 文件名长 */
        unsigned char type;           /* the type of d_name 文件类型 */
        char name[FILE_NAME_MAX + 1]; /* file name (null-terminated) 文件名，最长255字符 */
    } lp_dir_t;

    typedef struct _lp_info_t
    {
        uint8_t type;   // 文件或文件夹
        uint32_t fsize; // 文件大小
        // 下面的信息不一定有
        uint32_t fdate;   // Modified date
        uint32_t ftime;   // Modified time
        uint32_t fattrib; // File attribute

        uint32_t _atime;
        uint32_t _mtime;
        uint32_t _ctime;
    } lp_info_t;

    typedef struct _lfdir_t
    {
        void *dir;
        lp_dir_t dirent;
        int driver_num; // 哪一个驱动上的
    } lfdir_t;

    typedef struct _lp_mdrive_t
    {
        char letter;                                                             // 盘符号
        char name[7];                                                            // 文件系统显示名字
        void *(*open_cb)(const char *name, const char *mode);                    // 打开文件
        int (*close_cb)(lp_file_t *file);                                        // 关闭文件
        int (*read_cb)(void *ptr, int size, int type, lp_file_t *stream);        // 读取文件数据
        int (*write_cb)(const void *ptr, int size, int type, lp_file_t *stream); // 写入数据
    } lp_mdrive_t;

    typedef struct _lp_fat_drive_t
    {
        char letter;                                                              // 盘符号
        uint8_t is_mount;                                                         //
        char name[7];                                                             // 文件系统显示名字
        void *(*open_cb)(const char *filename, const char *mode);                 // 打开文件
        int (*close_cb)(lp_file_t *file);                                         // 关闭文件
        int (*read_cb)(void *ptr, int size, int nmemb, lp_file_t *stream);        // 读取文件数据
        int (*write_cb)(const void *ptr, int size, int nmemb, lp_file_t *stream); // 写入数据
        int (*stat_cb)(const char *path, lp_info_t *lp_info);                     // 文件或文件夹状态信息
        int (*unlink_cb)(const char *filename);                                   // 删除文件/文件夹
        int (*seek_cb)(lp_file_t *file, uint32_t pos, int whence);                // 文件偏移地址
        int (*tell_cb)(lp_file_t *file);                                          // 得到文件当前位置
        int (*format_cb)(const char *driver_name);                                // 格式化文件系统
        int (*mount_cb)(const char *driver_name);                                 // 挂载文件系统
        int (*unmount_cb)(const char *driver_name);                               // 卸载文件系统
        int (*rename_cb)(const char *oldpath, const char *newpath);               // 改文件名
        int (*mkdir_cb)(const char *newdir);                                      // 新建文件夹
        void *(*dir_open_cb)(const char *path);                                   // 打开文件夹
        lp_dir_t *(*dir_read_cb)(lfdir_t *dir);                                   // 读取文件夹内容-可遍历
        int (*dir_close_cb)(lfdir_t *dir);                                        // 关闭文件夹
        int (*volume_cb)(char *driver, int *total, int *free_bs);                 // 获取容量
        void *user_data1;                                                         // 用户自定义数据
        void *user_data2;                                                         // 用户自定义数据
    } lp_fat_drive_t;

    void lp_fat_driver_info(void);                                               // 驱动信息打印，需要开log
    int lp_fat_driver_nums(void);                                                // 返回多小个盘
    int lp_fat_driver_init(lp_fat_drive_t *lp_fat_drive, int lp_fat_drive_size); // 注册盘
    void *lp_fat_driver_user_data(uint32_t drive_num, int num);                  // 给指定盘位 自定义数据
    lp_file_t *lp_open(const char *filename, const char *mode);                  // 打开文件
    int lp_close(lp_file_t *stream);                                             // 关闭文件
    int lp_read(void *ptr, int size, int nmemb, lp_file_t *stream);              // 读取文件数据
    int lp_write(const void *ptr, int size, int nmemb, lp_file_t *stream);       // 写入数据
    int lp_stat(const char *path, lp_info_t *lp_info);                           // 文件或文件夹状态信息
    int lp_unlink(const char *path);                                             // 删除文件/文件夹
    int lp_fsize(lp_file_t *file);                                               // 文件大小
    int lp_filesize(const char *path);                                           // 文件大小
    int lp_seek(lp_file_t *file, int offset, int fromwhere);                     // 文件偏移地址
    int lp_tell(lp_file_t *file);                                                // 得到文件当前位置
    int lp_rewind(lp_file_t *stream);                                            //
    int lp_format(const char *driver_name);                                      // 格式化文件系统
    int lp_mount(const char *driver_name);                                       // 挂载文件系统
    int lp_unmount(const char *driver_name);                                     // 卸载文件系统
    int lp_rename(const char *oldpath, const char *newpath);                     // 改文件名
    int lp_mkdir(const char *path);                                              // 新建文件夹
    lfdir_t *lp_opendir(const char *path);                                       // 打开文件夹
    lp_dir_t *lp_readdir(lfdir_t *dir);                                          // 读取文件夹内容-可遍历
    int lp_closedir(lfdir_t *dir);                                               // 关闭文件夹
    int lp_file_exist(const char *path);                                         // 文件是否存在
    int lp_read_uint16(lp_file_t *fp, uint16_t *i);                              // 读取 2字节 uint16_t
    int lp_write_uint16(lp_file_t *fp, uint16_t i);                              // 写入 2字节 uint16_t
    int lp_ferror(lp_file_t *fp);                                                // 返回错误码

    /// @brief 获取文件系统容量信息
    /// @param driver 盘符如 "/C"  "/D"   "/E"
    /// @param total 输出总大小 byte
    /// @param free_bs 可用大小 byte
    /// @return 返回码
    int lp_fs_volume(char *driver, int *total, int *free_bs);

    /// @brief 遍历文件
    /// @param panme  文件夹路径
    /// @param lp_file_list_func_cb 回调处理函数
    /// @param pOpaque 自定义参数
    /// @return
    int lp_filelist(char *panme, int (*lp_file_list_func)(void *pOpaque, int root_len, const char *pFilename, int type), void *pOpaque);

    /// @brief 遍历文件夹
    /// @param panme 文件夹路径
    /// @return
    int lp_filelist_printf(char *panme);

    /// @brief 文件尾部追加数据
    /// @param filename 文件路径
    /// @param ptr 追究内容
    /// @param size 长度
    /// @return 返回码
    int lp_append(const char *filename, const void *ptr, int size);

    /// @brief 读取整个文件
    /// @param buf_path 文件路径
    /// @param len 返回文件长度
    /// @return 文件内容-外部需要free释放内存
    uint8_t *lp_readall(char *buf_path, uint32_t *len);

    /// @brief 新建文件方式写入一个文件
    /// @param path 文件路径
    /// @param data 文件内容
    /// @param len 文件内存长度
    /// @return 返回码
    uint32_t lp_save(char *path, uint8_t *data, uint32_t len);

    /// @brief 复制文件
    /// @param src 源文件
    /// @param dec 目标文件
    /// @return 返回码
    int lp_copy(char *src, char *dec);

    /// @brief 复制文件到文件夹
    /// @param src 文件
    /// @param dec 文件夹
    /// @return 0 ok
    int lp_copy_file_2_dir(char *src, char *dec);

    /// @brief 给文件句柄写入printf格式化数据
    /// @param fd 文件句柄
    /// @param fmt 格式字符
    /// @param
    /// @return 返回码
    int lp_log(lp_file_t *fd, const char *fmt, ...);

    /// @brief 先判断有没有路径，没有就新建
    /// @param dir 路径
    /// @return 0 ok
    int lp_mkdir_try(char *dir);

    int lp_fs_init(void);

    //////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////

    /// @brief socket接受数据-已经收取到数据的时候-超出3ms就立马返回
    /// @param fd socket 句柄
    /// @param buf 接收buf
    /// @param nbytes 期望接收大小
    /// @param timeout_ms 超时时间单位ms
    /// @return 真实接收大小
    int lpbsp_socket_read_timeout(int fd, uint8_t *buf, int nbytes, int timeout_ms);
    int lpbsp_socket_read_http_timeout(int fd, uint8_t *buf, int nbytes, int timeout_ms);

    /// @brief 读取http的返回
    /// @param fd socket句柄
    /// @param buf 读取数据buf
    /// @param inout in，导入的buf大小，out，实体数据的大小
    /// @param read_len out 读取到的全部数据大小
    /// @return 0 没有读取出实体数据  1实体数据buf开始的地址
    uint8_t *lpbsp_socket_read_http(int fd, uint8_t *buf, uint32_t *inout, uint32_t *read_len_out);

    /*
    获取指定的webapi的boby数据 (少于4K)
    baby_data:没有就发null
    */
    int lp_http_file_down(char *url, char *file_name, void (*prog_cb)(uint32_t prog, uint32_t fsize));
    /// @brief 下载文件-数据流-有些服务器会下载不成功
    /// @param url 网址-需要支持分包下载
    /// @param prog_cb 数据流
    /// @return 0:ok 1:err
    int lp_http_file_down_data(char *url, int (*prog_cb)(uint8_t *data, uint32_t type, uint32_t value), int (*prog_bar_cb)(uint32_t type, uint32_t value));

    /// @brief 上传文件-数据流
    /// @param url 网址-
    /// @param file_name 待上传文件
    /// @param prog_cb 数据流进度
    /// @return 0:ok 1:err
    int lp_http_file_up(char *url, char *file_name, void (*prog_cb)(uint32_t prog, uint32_t fsize));

    //////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////
    /// @brief 设置粘贴板
    /// @param type 类型
    /// @param data 数据
    /// @param len 长度
    /// @return 0 ok 1 malloc err
    int lp_pasteboard_set(int type, uint8_t *data, int len);
    /// @brief 获取粘贴板类型和长度
    /// @param data_len_out 长度输出
    /// @return 0 ok
    int lp_pasteboard_type_get(int *data_len_out);
    /// @brief 获取粘贴板内容
    /// @param data_out 输出内容buf
    /// @return 0 ok
    int lp_pasteboard_get(uint8_t *data_out);
    /// @brief 清空粘贴板
    /// @param nano
    /// @return 0 ok
    int lp_pasteboard_clear(void);
    //////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////
    /// @brief 字符串相加
    /// @param s1 源字符
    /// @param s2 待相加字符
    char *lp_string_add_string(char *s1, char *s2);
    /**
     *  返回下一个字符的位置
     * @param in I 字符串
     * @param c I 字符
     * @return 下一个字符c的位置
     */
    char *lp_string_next(char *in, char c);

    /**
     *  删除c后面的字符 包括当前
     * @param in I 字符串
     * @param c I 字符
     * @return 0 ok
     */
    int lp_string_cut(char *in, char c);
    /**
     *  返回下一个字符串的位置
     * @param in I 字符串
     * @param c I 字符串
     * @return 下一个字符串c的位置
     */
    char *lp_string_next_chars(char *in, char *c);

    /// @brief string copy
    /// @param src in:
    /// @param out out:
    /// @param max_num in:
    /// @return
    int lp_string_copy(char *src, char *out, int max_num);

    /// @brief 字符相加
    /// @param src 源字符
    /// @param in 待相加字符
    /// @return 返回src字符结尾指针
    char *lp_string_cat(char *src, char *in);
    /**
     *  返回字符串中的ch的数量
     * @param in I 字符串
     * @param c I 字符
     * @return 字符串中的ch的数量
     */
    int lp_string_char_num_get(char *src, char ch);

    /**
     *  返回字符串中的c 分割 第index个 的长度
     * @param src I 字符串
     * @param c I 字符
     * @param index I 第index个
     * @return 字符串长度
     */
    int lp_string_split_index_size(char *src, char c, int index);
    /**
     *  返回字符串中的c 分割 第num个 的data
     * @param in I 字符串
     * @param c I 字符
     * @param num I 第num个
     * @param out O data
     * @return 下一份字符串的位置
     */
    char *lp_string_split_index(char *in, char c, int num, char *out);
    /// <summary>
    /// 替代字符中的字符
    /// </summary>
    /// <param name="path">字符串</param>
    /// <param name="ole">旧字符</param>
    /// <param name="cnew">新字符</param>
    void lp_string_replace_char(char *path, char ole, char cnew);

    /// @brief 返回文件名后缀
    /// @param in 文件路径
    /// @param out 后缀输出buf
    /// @return
    uint8_t lp_file_suffix(char *in, char *out);

    // 判断文件名后缀
    uint8_t lp_file_suffix_if(char *path, char *suffix);

    /// <summary>
    /// 全局路径加相对路径，输出malloc后数据
    /// </summary>
    /// <param name="rootpath_in">全局路径</param>
    /// <param name="path_in">相对路径</param>
    /// <returns>输出合成路径</returns>
    char *lp_file_dir_add(char *rootpath_in, char *path_in);
    /// <summary>
    /// 全路径的文件提取路径 如 /C/A/B.zip -> /C/A
    /// </summary>
    /// <param name="path">文件全路径 输入</param>
    /// <param name="name">路径 输出</param>
    /// <returns></returns>
    uint8_t lp_file_dirpath_get(char *path, char *name);
    /// <summary>
    /// 提取文件名带后缀 如  /C/A/B.zip -> B.zip
    /// </summary>
    /// <param name="path">全路径，输入</param>
    /// <param name="name">文件名 输出</param>
    /// <returns>0 ok 1 err</returns>
    uint8_t lp_file_filename_get(char *path, char *name);
    /// <summary>
    /// 把全路径去除点后缀，返回如  /C/A/B.zip -> /C/A/B
    /// </summary>
    /// <param name="path">全路径，输入</param>
    /// <param name="name">有路径文件名，不带后缀</param>
    /// <returns></returns>
    uint8_t lp_file_dirname_get(char *path, char *name);
    //////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////

    /// @brief 压缩文件或文件夹
    /// @param zip_file 待压缩文件或文件夹
    /// @param panem 输出路径
    /// @return
    int lp_zip_en(char *zip_file, char *panem);
    /// @brief 解压zip_file
    /// @param zip_file 路径
    /// @param out_path 放在这,这个后缀不带/
    /// @return
    int lp_zip_un(char *zip_file, char *out_path);

    //////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////
    //////////////////////////////////////////////////////////////

    /// @brief list init
    /// @param nano
    /// @return >0 ok handle
    int lp_list_init(void);

    /// @brief list长度
    /// @param handle 句柄
    /// @return 已存在长度
    int lp_list_len(int handle);

    /// @brief list 添加
    /// @param handle 句柄
    /// @param str 字符串
    /// @return 0 ok
    int lp_list_add(int handle, char *str);

    /// @brief 获取list 依据index
    /// @param handle 句柄
    /// @param index 序号 0 start
    /// @return 内容指针
    char *lp_list_get(int handle, int index);

    /// @brief list 删除一个index
    /// @param handle 句柄
    /// @param index 序号
    /// @return 0 ok
    int lp_list_del_one(int handle, int index);

    /// @brief 释放list
    /// @param handle 句柄
    /// @return 0 ok
    int lp_list_free(int handle);

    ////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////
    struct lp_gps_info_t
    {
        datetime_t dt;
        float latitude;
        float longitude;
        float speed;
    };

    int lp_gps_init(uint8_t eanble);
    int lp_gps_state(void); // > 0 open gps 0:close
    int lp_gps_read(struct lp_gps_info_t *info);
    ////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////
    int lp_sensor_magnetic_angle_read(void);
    ////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////
    /////////////////////////// link 链表 ///////////////////////////////////////////
    ////////////////////////////////////////////////////////////////////////////////
    typedef struct _lp_link_t
    {
        void *data; // 数据
        struct _lp_link_t *next;
    } lp_link_t;
    lp_link_t *lp_link_new(void);
    lp_link_t *lp_link_find(lp_link_t *phead, void *data);
    lp_link_t *lp_link_add(lp_link_t *L, void *data);
    int lp_link_del(lp_link_t *phead);
    int lp_link_remove(lp_link_t *phead, void *data);
    int lp_link_len(lp_link_t *phead);
////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////
#include <stdlib.h>
#include <setjmp.h>
#define los_api_vs 1
#define los_bulid_vs 1
#define REG_NUM 32
#define REG_RETURN 14

#define HEAP_REG 0
#define CREGLEN 4 * 13 // 函数调用压栈的大小，因为中断有嵌套所以需要把8-15的reg也保存了
#define AREGLEN (4 * REG_NUM)
#define ARRLEN(arr) (sizeof(arr) / sizeof(arr[0]))
    typedef void (*fun_os)(void *v);

    extern const fun_os los_function_arr[];

    typedef union _cpu_reg_t
    {
        uint8_t u8;
        int8_t s8;
        uint16_t u16;
        int16_t s16;
        uint32_t u32;
        int32_t s32;
        //	float f32;
        uint8_t data[4];
    } cpu_reg_t;

    typedef struct _los_irq_len_t
    {
        uint16_t arg;
        uint16_t los_irq;
        jmp_buf jbuf;
        uint8_t *pc;
        cpu_reg_t reg[REG_NUM];

    } los_irq_len_t;
    typedef struct _los_t
    {
        void *malloc; // link ram
        uint8_t *gram;
        uint8_t los_irq_pc[2]; // 中断pc指向的位置
        uint8_t *iqr;          // 中断保存当前的pc指针---多次中断呢？？？
        uint8_t arg;
        uint8_t los_irq;
        jmp_buf jbuf;
        uint8_t *pc;
        cpu_reg_t reg[REG_NUM];
        uint8_t *ram;
        uint32_t ram_len;
        uint32_t lvar_start;
        uint32_t stack_end;
        uint32_t code_len;
        uint32_t data_len;
        uint32_t bss_len;
        uint8_t *code;

        fun_os *func;
    } los_t;
    typedef void (*los_cmd)(los_t *lp, uint8_t arg);
    ////////////////////////////////////////////////////////////////////////////////////////////

    /**
    移植请修改内存管理api
    Please modify the memory management API
    */

    /**
     * los_vm_api 调用 cmd_88 -> func ->外部函数 再调用此函数
     */
    int los_call_addr(los_t *lp, uint32_t call_addr, int _argc, uint32_t *_argv);
    /**
los中断函数,有返回值
*/
    int los_irq(los_t *lp, int num);
    /**
设置用户函数
Set user function
size 函数数量
*/
    int los_function_add(fun_os *f, int size);
    /**
运行los文件
addr参数:los文件在ram或rom中的地址
Run los file
Addr parameter: the address of the los file in ram or rom
*/
    los_t *los_app_new(uint32_t *addr, uint8_t inter);

    /** note
运行los
*/
    int los_app_run(los_t *lp);
    int los_app_close(los_t *lp);
    /**
给los的返回参数
*/
    void los_set_return(los_t *lp, uint32_t data);
    /**
获取los数据地址
*/
    uint8_t *los_get_ram(los_t *lp);
    /**
获取函数参数num从1开始
*/
    void los_set_struct(los_t *lp, uint8_t *data, int len); // 返回结构体
    void *los_get_struct(los_t *lp, uint32_t num);          // 函数argc获取它的地址

    uint8_t los_get_u8(los_t *lost, uint32_t num);
    int8_t los_get_s8(los_t *lost, uint32_t num);
    uint16_t los_get_u16(los_t *lost, uint32_t num);
    int16_t los_get_s16(los_t *lost, uint32_t num);
    uint32_t los_get_u32(los_t *lost, uint32_t num);
    int32_t los_get_s32(los_t *lost, uint32_t num);
    float los_get_float(los_t *lost, uint32_t num);
    uint64_t los_get_u64(los_t *lost, uint32_t num);
    int64_t los_get_s64(los_t *lost, uint32_t num);
    double los_get_double(los_t *lost, uint32_t num);

    void *los_get_voidp(los_t *lost, uint32_t num);
    uint8_t *los_get_u8p(los_t *lost, uint32_t num);
    int8_t *los_get_s8p(los_t *lost, uint32_t num);
    uint16_t *los_get_u16p(los_t *lost, uint32_t num);
    int16_t *los_get_s16p(los_t *lost, uint32_t num);
    uint32_t *los_get_u32p(los_t *lost, uint32_t num);
    int32_t *los_get_s32p(los_t *lost, uint32_t num);
    uint64_t *los_get_u64p(los_t *lost, uint32_t num);
    int64_t *los_get_s64p(los_t *lost, uint32_t num);
    float *los_get_floatp(los_t *lost, uint32_t num);
    double *los_get_doublep(los_t *lost, uint32_t num);

    /// 读取2维数组的数据 offset 偏移地址1表示int(大小)
    void *los_get_p2(los_t *lp, uint32_t num, int offset);

    char *los_next_app_get(void);
    int los_next_app_set(char *p);

    fun_os *los_api_function_get(void);
    ////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////
    typedef struct _lplib_rtc_ram_t
    {
        uint32_t flg;
        uint16_t zip_start;
        uint16_t zip_len;
        uint32_t ram_len;
        uint32_t ram_now;
        uint32_t dt;
    } lplib_rtc_ram_t;

    int lplib_rtc_ram_init(void);                  // rtc 内存初始化
    int lplib_rtc_ram_add(uint8_t *data, int len); // 添加一个数据
    int lplib_rtc_ram_save(void);                  // 立刻保存到文件系统去
    int lplib_power_mode_get(void);                // 获取功耗模式
    int lplib_power_mode_set(int type);            // 设置功耗模式
    int lplib_language_set(char *lang);            //
    char *lplib_language_get(void);                //
    int lplib_bl_time_set(int sec);                //
    int lplib_bl_time_get(void);                   //
    int lplib_bl_value_set(int value);             //
    int lplib_bl_value_get(void);                  //

    ////////////////////////////////////////////////////////////////
    ////////////////////////////////////////////////////////////////
#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif