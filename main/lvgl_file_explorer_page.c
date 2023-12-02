#include "lvgl_inc.h"
#include "stdio.h"
#include "lplib.h"

static lv_obj_t *list, *lab_path;
static lv_obj_t *page_src;
static char *file_dir = {0};
static const char *btns[] = {"Yes", 0};

// lp_i18n_string_START
const char *FILE_EXPLORER_TEXT[][2] = {
    {LV_SYMBOL_COPY, "Back"},
    {LV_SYMBOL_COPY, "Open"},
    {LV_SYMBOL_COPY, "Rename"},
    {LV_SYMBOL_COPY, "Copy"},
    {LV_SYMBOL_COPY, "Delete"},
    {LV_SYMBOL_COPY, "Up"},
};
// lp_i18n_string_END

const char *FILE_MANAGE_TEXT[] = {
    {"Format"},
    {"Test Write"},
    {"Down Map"},
    {"Down back app"},
};

static int view_page_file_explorer_dir(char *path);
/// @brief 后退
/// @param
static void view_page_file_explorer_back(void)
{
    int i, len;

    len = strlen(file_dir);
    for (i = len; i > 1; i--)
    {
        if ('/' == file_dir[i])
        {
            file_dir[i] = 0;
            lv_label_set_text(lab_path, file_dir);
            lv_obj_del(list);
            view_page_file_explorer_dir(file_dir);
            return;
        }
    }
}

// 打开文件
static int file_open_event_handler(char *fname)
{
    char tbuf[64] = {0};
    int len = 0;

    lp_file_suffix(fname, tbuf);

    APP_LOGD("fname %s - %s\r\n", fname, tbuf);

    if (0 == strcmp("zip", tbuf)) // zip 文件
    {
        lp_file_dirpath_get(fname, tbuf);

        lp_zip_un(fname, tbuf); // 解压到当前文件夹内
    }

    return 0;
}
void lp_page_input_cb(void *arg, char *ole, char *new)
{
    char input_ole[68] = {0};
    char input_new[68] = {0};

    sprintf(input_ole, "%s/%s", file_dir, ole);
    sprintf(input_new, "%s/%s", file_dir, new);
    APP_LOGD("ole %s new %s\r\n", input_ole, input_new);

    lp_rename(input_ole, input_new);
}

void lp_unzip_event_input_cb(void *fname, char *input_ole, char *input_new)
{
    int ret = 0;
    char tbuf[32];

    APP_LOGD("input_ole %s,input_new %s\r\n", input_ole, input_new);

    ret = lp_zip_un(fname, tbuf);
    sprintf(tbuf, "zip ret %d", ret);
    lv_obj_center(lv_msgbox_create(NULL, "Unzip", tbuf, 0, true));
}

/// @brief 长按话框事件
/// @param e
static void msgbox_unzip_btn_callback(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    char tbuf[48], fd_type = 0, *temp;
    char *fname = (char *)lv_event_get_user_data(e);
    lv_obj_t *msg_obj = lv_event_get_current_target(e);
    int ret = 0;

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        temp = lv_msgbox_get_active_btn_text(msg_obj);

        lp_file_dirpath_get(fname, tbuf);

        APP_LOGD("btn %s,fname %s 2 %s\r\n", temp, fname, tbuf);
        if (0 == strcmp("Current", temp))
        {

            ret = lp_zip_un(fname, tbuf);
            sprintf(tbuf, "zip ret %d", ret);
            lv_obj_center(lv_msgbox_create(NULL, "Unzip", tbuf, 0, true));
        }
        else if (0 == strcmp("Other", temp))
        {
            lp_lvgl_input(fname, tbuf, lp_unzip_event_input_cb);
        }

        lv_msgbox_close(msg_obj);
    }
    else if (LV_EVENT_DELETE == code)
    {
        lpbsp_free(fname);
    }
}

static void msgbox_del_btn_callback(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    char *fname = (char *)lv_event_get_user_data(e);
    lv_obj_t *msg_obj = lv_event_get_current_target(e);
    if (code == LV_EVENT_VALUE_CHANGED)
    {
        APP_LOGD("del file [%s]\r\n", fname);
        lp_unlink(fname);
        lv_msgbox_close(msg_obj);
    }
    else if (LV_EVENT_DELETE == code)
    {
        lpbsp_free(fname);
    }
}

/// @brief list点击事件
/// @param e
static void file_list_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    int dirp_type = 0, fd_type = 0, len;
    char *temp_p, tbuf[54] = {0};
    lv_obj_t *mbox1;
    static const char *long_btns[] = {"Current", "Other", 0};
    char fname[32];

    if (code == LV_EVENT_SHORT_CLICKED)
    {
        dirp_type = (int)obj->user_data % 1000;
        fd_type = (int)obj->user_data / 1000;

        if (dirp_type == 1)
        {
            if (2 == fd_type) // 打开文件夹
            {
                lv_obj_del(list);
                view_page_file_explorer_dir(file_dir);
                return;
            }
            else // 打开文件
            {
                fd_type = file_open_event_handler(lv_label_get_text(lab_path));
                if (fd_type)
                    return;
            }
        }
        else if (dirp_type == 2) // 改名字
        {
            lp_file_filename_get(lv_label_get_text(lab_path), tbuf);
            lp_lvgl_input(0, tbuf, lp_page_input_cb);
        }
        else if (dirp_type == 3) // 复制文件
        {
            temp_p = lv_label_get_text(lab_path);
            lp_pasteboard_set(fd_type, (uint8_t *)temp_p, strlen(temp_p));
        }
        else if (dirp_type == 4) // 删除文件
        {
            static const char *btns[] = {"Yes", 0};

            lv_obj_t *mbox1 = lv_msgbox_create(NULL, "Tips", "Delete", btns, true);
            temp_p = lpbsp_malloc(strlen(lv_label_get_text(lab_path)) + 1);
            strcpy(temp_p, lv_label_get_text(lab_path));
            lv_obj_add_event_cb(mbox1, msgbox_del_btn_callback, LV_EVENT_ALL, temp_p);
            lv_obj_center(mbox1);
        }
        else if (dirp_type == 5) // 上传文件
        {
            temp_p = lv_label_get_text(lab_path);
            lp_file_filename_get(temp_p, fname);
            sprintf(tbuf, "http://192.168.137.1:5000/%s", fname);
            lp_lvgl_upfile(tbuf, temp_p);
        }
        else if (dirp_type == 106) // 粘贴文件
        {
            fd_type = lp_pasteboard_type_get(&len);
            lp_pasteboard_get((uint8_t *)tbuf);
            lp_copy_file_2_dir(tbuf, lv_label_get_text(lab_path));
        }
        else if (dirp_type == 107) // 解压zip文件
        {
            mbox1 = lv_msgbox_create(NULL, "Open", "unzip current or other directory", long_btns, true);
            len = strlen(lv_label_get_text(lab_path));
            temp_p = lpbsp_malloc(len + 1);
            strcpy(temp_p, lv_label_get_text(lab_path));
            lv_obj_add_event_cb(mbox1, msgbox_unzip_btn_callback, LV_EVENT_ALL, temp_p);
            lv_obj_center(mbox1);
        }
        view_page_file_explorer_back();
    }
}
/// @brief 点击文件
/// @param root 文件路径
/// @param fname 文件名字和后缀
static void list_event_file_click(int file_or_dir, char *root, char *fname)
{
    lv_obj_t *list_btn;
    int i = 0, ret;
    char tbuf2[32];

    sprintf(file_dir, "%s/%s", root, fname);

    lv_label_set_text(lab_path, file_dir);
    list = lv_list_create(page_src);
    lv_obj_set_pos(list, 0, 35);
    lv_obj_set_size(list, 240, 210);

    if (file_or_dir == LF_TYPE_FILE)
    {
        ret = lp_filesize(file_dir);
        APP_LOGD("file_dir %s,len=%d\r\n", file_dir, ret);
        if (10240 * 1024 < ret)
        {
            sprintf(tbuf2, "size:%dMB", lp_filesize(file_dir) / (1024 * 1024));
        }
        else if (10240 < ret)
        {
            sprintf(tbuf2, "size:%dKB", lp_filesize(file_dir) / 1024);
        }
        else
        {
            sprintf(tbuf2, "size:%dB", lp_filesize(file_dir));
        }
        lv_list_add_text(list, tbuf2);
    }
    for (i = 0; i < ARR_SIZE(FILE_EXPLORER_TEXT); i++)
    {
        list_btn = lv_list_add_btn(list, FILE_EXPLORER_TEXT[i][0], lp_i18n_string(FILE_EXPLORER_TEXT[i][1]));
        lv_obj_set_height(list_btn, TFT_OBJ_HEIGHT);
        list_btn->user_data = (void *)(i + file_or_dir * 1000);
        lv_obj_add_event_cb(list_btn, file_list_event_handler, LV_EVENT_SHORT_CLICKED, NULL);
    }
    // 在文件夹内 看看有没有粘贴的文件或文件夹
    if (file_or_dir == LF_TYPE_DIR && lp_pasteboard_type_get(0))
    {
        list_btn = lv_list_add_btn(list, FILE_EXPLORER_TEXT[1][0], lp_i18n_string("Paste"));
        lv_obj_set_height(list_btn, TFT_OBJ_HEIGHT);
        list_btn->user_data = (void *)(106);
        lv_obj_add_event_cb(list_btn, file_list_event_handler, LV_EVENT_SHORT_CLICKED, NULL);
    }

    lp_file_suffix(fname, tbuf2);
    if (0 == strcmp("zip", tbuf2) || 0 == strcmp("ZIP", tbuf2)) // zip 文件
    {
        list_btn = lv_list_add_btn(list, FILE_EXPLORER_TEXT[1][0], lp_i18n_string("UnZip"));
        lv_obj_set_height(list_btn, TFT_OBJ_HEIGHT);
        list_btn->user_data = (void *)(107);
        lv_obj_add_event_cb(list_btn, file_list_event_handler, LV_EVENT_SHORT_CLICKED, NULL);
    }
}

/// @brief list点击事件
/// @param e
static void list_event_back_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (code == LV_EVENT_SHORT_CLICKED)
    {
        view_page_file_explorer_back();
    }
}
/// @brief list点击事件
/// @param e
static void list_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    int dirp_type = 0, i;

    if (code == LV_EVENT_SHORT_CLICKED)
    {
        dirp_type = (int)obj->user_data;

        if (dirp_type == LF_TYPE_DIR)
        {
            lp_string_add_string(file_dir, "/");
            lp_string_add_string(file_dir, lv_list_get_btn_text(list, obj));
            lv_obj_del(list);
            view_page_file_explorer_dir(file_dir);
        }
    }
}

/// @brief 长按话框事件
/// @param e
static void msgbox_long_btn_callback(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    char tbuf[32], fd_type = 0;
    lv_obj_t *obj = (lv_obj_t *)lv_event_get_user_data(e);
    lv_obj_t *msg_obj = lv_event_get_current_target(e);

    if (code == LV_EVENT_VALUE_CHANGED)
    {
        if (0 == strcmp("Yes", lv_msgbox_get_active_btn_text(msg_obj)))
        {
            strcpy(tbuf, lv_list_get_btn_text(list, obj));
            fd_type = (int)obj->user_data;

            lv_obj_del(list);
            list_event_file_click(fd_type, file_dir, tbuf);
        }
        else
        {
            lv_obj_add_event_cb(obj, list_event_handler, LV_EVENT_SHORT_CLICKED, NULL);
        }
        lv_msgbox_close(msg_obj);
    }
}
/// @brief list长按点击事件
/// @param e
static void list_event_long_btn_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_target(e);
    lv_obj_t *mbox1;
    static const char *long_btns[] = {"Yes", "No", 0};

    if (code == LV_EVENT_LONG_PRESSED)
    {
        lv_obj_remove_event_cb(obj, list_event_handler);

        mbox1 = lv_msgbox_create(NULL, "Open", "Open file / dir", long_btns, 0);
        lv_obj_add_event_cb(mbox1, msgbox_long_btn_callback, LV_EVENT_VALUE_CHANGED, obj);
        lv_obj_center(mbox1);
    }
}
/// @brief 显示文件列表
/// @param path 路径
/// @return  0 ok
static int view_page_file_explorer_dir(char *path)
{
    lp_dir_t *dirp;
    lfdir_t *dir;
    lv_obj_t *list_btn;

    lv_label_set_text(lab_path, path);

    list = lv_list_create(page_src);
    lv_obj_set_pos(list, 0, 35);
    lv_obj_set_size(list, 240, 210);

    if (strlen(path) > 2)
    {
        list_btn = lv_list_add_btn(list, LV_SYMBOL_BACKSPACE, lp_i18n_string("Back"));
        lv_obj_set_height(list_btn, TFT_OBJ_HEIGHT);
        list_btn->user_data = (void *)0;
        lv_obj_add_event_cb(list_btn, list_event_back_handler, LV_EVENT_SHORT_CLICKED, NULL);
    }

    dir = lp_opendir(path);
    if (0 == dir)
    {
        APP_LOGD("open dir=%s error\r\n", path);
        return 1;
    }
    APP_LOGD("open dir=%s\r\n", path);
    while (1)
    {
        dirp = lp_readdir(dir);
        if (0 == dirp)
            break;

        if (dirp->type == LF_TYPE_DIR)
        {
            list_btn = lv_list_add_btn(list, LV_SYMBOL_DIRECTORY, dirp->name);
            lv_obj_add_event_cb(list_btn, list_event_handler, LV_EVENT_SHORT_CLICKED, NULL);
        }
        else
        {
            list_btn = lv_list_add_btn(list, LV_SYMBOL_FILE, dirp->name);
        }
        lv_obj_set_height(list_btn, TFT_OBJ_HEIGHT);
        list_btn->user_data = (void *)dirp->type;
        lv_obj_add_event_cb(list_btn, list_event_long_btn_handler, LV_EVENT_LONG_PRESSED, NULL);
    }
    lp_closedir(dir);
    APP_LOGD("close dir\n\n");
    return 0;
}
/// @brief 升级对话框事件
/// @param e
static int fat_format_callback(void *arg)
{
    int ret = 0;

    ret = lp_format("/C");
    if (ret)
        return ret;

    ret = lp_mount("/C");

    return ret;
}

static int test_2m_file_callback(void *arg)
{
    lp_file_t *fd;
    int i = 0;
    char *tbuf;

    tbuf = lpbsp_malloc(20 * 1024);
    memset(tbuf, '2', 20 * 1024);
    fd = lp_open("/C/test.txt", "wb+");
    for (i = 0; i < 100; i++) // 200*20;
    {
        lp_write(tbuf, 20 * 1024, 1, fd);
    }
    lp_close(fd);

    return 0;
}
/// @brief 格式化事件
/// @param e
static void file_manage_btn_event_handler(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    lv_obj_t *obj = lv_event_get_current_target(e);
    lv_obj_t *mbox1;
    if (code == LV_EVENT_SHORT_CLICKED)
    {
        switch ((int)obj->user_data)
        {
        case 0: /// @brief 格式化事件
            lp_lvgl_func_run("Format", "format /C", 0, fat_format_callback);
            break;
        case 1:
            lp_lvgl_func_run("Test", "Test 2M size file", 0, test_2m_file_callback);
            break;

        case 2: // down file
            lp_lvgl_down2file("192.168.137.1:5000/map.zip", "/C/map.zip");
            break;

        case 3: // down back file
            lp_lvgl_down2file("192.168.137.1:5000/back.app", "/C/back.app");
            break;
        }
    }
}
static void view_page_strong_close_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (LV_EVENT_DELETE == code)
    {
        lvgl_sleep_time_ms(15000);
    }
}
/// @brief 存储
/// @param src
int view_page_strong(lv_obj_t *src)
{
    lv_obj_t *btn, *lab;
    int i = 0;
    int total, free_bs;

    lp_fs_volume("/C", &total, &free_bs);

    lvgl_sleep_time_ms(0xfffffff);
    lv_obj_add_event_cb(src, view_page_strong_close_cb, LV_EVENT_DELETE, 0);

    lab = lv_label_create(src); // 添加文字
    // lv_obj_set_style_text_font(lab, &lv_font_montserrat_22, 0);
    lv_obj_set_pos(lab, 0, 0);
    lv_obj_set_size(lab, 240, 35);
    lv_label_set_text_fmt(lab, "SD:%d/%dMB", free_bs / 1048576, total / 1048576);

    for (i = 0; i < ARR_SIZE(FILE_MANAGE_TEXT); i++)
    {
        btn = lv_btn_create(src);
        btn->user_data = (void *)i;
        lv_obj_set_pos(btn, 20, 45 + TFT_OBJ_HEIGHT_GAP * i);
        lv_obj_set_size(btn, 200, TFT_OBJ_HEIGHT);
        lab = lv_label_create(btn);
        lv_obj_center(lab);
        lv_label_set_text(lab, FILE_MANAGE_TEXT[i]);
        lv_obj_add_event_cb(btn, file_manage_btn_event_handler, LV_EVENT_SHORT_CLICKED, NULL);
    }
    return 0;
}

static void view_page_close_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (LV_EVENT_DELETE == code)
    {
        lvgl_sleep_time_ms(15000);
        lpbsp_free(file_dir);
    }
}

/// <summary>
/// 文件管理器界面
/// </summary>
/// <param name="last_src"></param>
/// <returns></returns>
int view_page_file_explorer(lv_obj_t *last_src)
{
    lv_obj_t *btn, *lab;

    file_dir = lpbsp_malloc(128);
    page_src = last_src;
    lvgl_sleep_time_ms(0xfffffff);

    lv_obj_add_event_cb(last_src, view_page_close_cb, LV_EVENT_DELETE, 0);

    lab_path = lv_label_create(last_src); // 添加文字
    lv_obj_set_pos(lab_path, 0, 0);
    lv_obj_set_size(lab_path, 240, 35);

    strcpy(file_dir, "/C");
    view_page_file_explorer_dir(file_dir);
    return 0;
}