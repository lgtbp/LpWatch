
#include "lvgl_inc.h"

#define SN_MODE "P0041"
static int sn_num = 0;
static lv_obj_t *lab_obj_sn;
static lv_obj_t *sn_spinbox;
const char sn_value[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

int sys_init_file(int index, char *pfile, int write_en);
void sn_count_get(char *data)
{
    int i = 0;
    uint8_t sum = 0;

    sprintf(data, SN_MODE "%c%c", sn_value[sn_num / 36], sn_value[sn_num % 36]);
    for (i = 0; i < 7; i++)
    {
        sum += data[i];
    }
    data[7] = sn_value[sum % 36];
    data[8] = 0;
}

// 生产系统初始化事件
static void sys_init_btn_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    int ret = 0;

    if (code == LV_EVENT_SHORT_CLICKED)
    {
        ret = sys_init_file(1, "/C/Sys/MapFile/map.zip", 1);
        if (ret)
        {
            lp_lvgl_msg_err_code("Write map", ret);
        }
        else
        {
            ret = lp_zip_un("/C/Sys/MapFile/map.zip", "/C/Sys/MapFile/"); // 解压到当前文件夹内
            if (ret)
                lp_lvgl_msg_err_code("Unzip map", ret);
        }
        ret = sys_init_file(0, "/C/LosHide/backapp.zip", 1);
        if (ret)
        {
            lp_lvgl_msg_err_code("Write app", ret);
        }
    }
}

static void lv_spinbox_increment_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    char temp[16] = {0};

    if (code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT)
    {
        lv_spinbox_increment(sn_spinbox);
        sn_num = lv_spinbox_get_value(sn_spinbox);
        sn_count_get(temp);
        lv_label_set_text_fmt(lab_obj_sn, "ready 2 write %s\r\n", temp);
    }
}

static void lv_spinbox_decrement_event_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    char temp[16] = {0};

    if (code == LV_EVENT_SHORT_CLICKED || code == LV_EVENT_LONG_PRESSED_REPEAT)
    {
        lv_spinbox_decrement(sn_spinbox);
        sn_num = lv_spinbox_get_value(sn_spinbox);
        sn_count_get(temp);
        lv_label_set_text_fmt(lab_obj_sn, "ready 2 write %s\r\n", temp);
    }
}

//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////////////////////////
#define SYS_HIDE_PATH "/C/LosHide"    // 系统文件隐藏夹路径
#define SYS_APP_ROOT_PATH "/C/App"    // app文件夹路径
#define SYS_ROOT_PATH "/C/Sys"        // 系统文夹路径
#define SYS_MAP_PATH "/C/Sys/MapFile" // 系统文夹路径
#ifdef WIN32
#define INSIDE_FLASH_OFF 0
#else
#define INSIDE_FLASH_OFF 0x200000
#endif
int sys_init_file(int index, char *pfile, int write_en)
{
    int ret = 0, file_len = 0, file_addr = 0, i = 0, k = 0, len = 4096;
    uint8_t *temp = 0;
    lp_info_t lf_info;
    lp_file_t *file;
    uint16_t sum16_read = 1, sum16_crc = 0;

    ret = lp_stat(pfile, &lf_info);
    APP_LOGD("write_en[%d]lf_stat ret [%s] %d\r\n", write_en, pfile, ret);
    if (LF_OK != ret || write_en)
    {
        file = lp_open(pfile, "wb+");
        if (0 == file)
        {
            return 3;
        }

        temp = lpbsp_malloc(4096);
        if (0 == temp)
        {
            lp_close(file);
            return 2;
        }
        lpbsp_inside_flash_read(INSIDE_FLASH_OFF + index * 8, temp, 8);
        file_addr = temp[0] | temp[1] << 8 | temp[2] << 16;
        file_len = temp[3] | temp[4] << 8 | temp[5] << 16;
        sum16_read = temp[6] | temp[7] << 8;
        if (2048 * 2048 > file_len) // 少于 4M FILE
        {
            for (i = 0; i < file_len;)
            {
                if (i + 4096 > file_len)
                {
                    len = file_len - i;
                }
                lpbsp_inside_flash_read(INSIDE_FLASH_OFF + file_addr + i, temp, len);
                for (k = 0; k < len; k++)
                    sum16_crc += temp[k];
                ret = lp_write(temp, 1, len, file);
                if (ret != len)
                    break;

                if (len != 4096)
                    break;
                i += len;
            }
            ret = lp_close(file);
        }
        lpbsp_free(temp);
    }
    else
        return 1;

    APP_LOGD("sum16_read %02x sum16_crc %02x\r\n", sum16_read, sum16_crc);
    if (sum16_read != sum16_crc)
    {
        return 12;
    }

    return ret;
}

int lp_sys_init_def(void)
{
    lp_info_t lf_info;
    int ret;

    ret = lp_stat(SYS_HIDE_PATH, &lf_info);
    APP_LOGD("lf_stat ret [%s] %d\r\n", SYS_HIDE_PATH, ret);
    if (LF_OK != ret)
    {
        ret = lp_mkdir(SYS_HIDE_PATH);
    }

    ret = lp_stat(SYS_ROOT_PATH, &lf_info);
    APP_LOGD("lf_stat ret [%s] %d\r\n", SYS_ROOT_PATH, ret);
    if (LF_OK != ret)
    {
        ret = lp_mkdir(SYS_ROOT_PATH);
    }

    ret = lp_stat(SYS_APP_ROOT_PATH, &lf_info);
    APP_LOGD("lf_stat ret [%s] %d\r\n", SYS_APP_ROOT_PATH, ret);
    if (LF_OK != ret)
    {
        ret = lp_mkdir(SYS_APP_ROOT_PATH);
    }

    ret = lp_stat(SYS_MAP_PATH, &lf_info);
    APP_LOGD("lf_stat ret [%s] %d\r\n", SYS_MAP_PATH, ret);
    if (LF_OK != ret)
    {
        ret = lp_mkdir(SYS_MAP_PATH);
        ret = sys_init_file(1, "/C/Sys/MapFile/map.zip", 0);
        if (0 == ret)
        {
            ret = lp_zip_un("/C/Sys/MapFile/map.zip", "/C/Sys/MapFile/"); // 解压到当前文件夹内
            if (ret)
                lp_lvgl_msg_err_code("Unzip map", ret);
        }
    }
    sys_init_file(0, "/C/LosHide/backapp.zip", 0);

    return 0;
}