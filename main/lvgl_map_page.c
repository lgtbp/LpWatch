
#include "lvgl_inc.h"

/*
4-6 guo
8-10 sheng
12-14 shi
16 qu
*/
// 依赖lvgl LV_USE_PNG png
extern unsigned lodepng_decode24_file(unsigned char **out, unsigned *w, unsigned *h, const char *filename);
static lv_obj_t *map_canvas;
static lv_obj_t *map_src;
static lv_obj_t *gps_lab = 0;
static lv_obj_t *gps_line = 0;
static lv_timer_t *timer;

static int gps_x = 0, gps_y = 0; // 默认中间了
static float gps_latitude = 0, gps_longitude = 0;

static void *fd_gps;
static uint8_t map_level = 4;          // map lev
static lv_point_t *map_gps_point;       // gps 轨迹数据 [x][y][x][y]
static uint16_t map_gps_point_size = 0; //
#define GPS_POINT_NUM 3072

#define GPS_COMPASS_WIDTH 12
#define CANVAS_WIDTH 240
#define CANVAS_HEIGHT 250
static uint16_t *map_data_buf;
static lv_img_dsc_t map_data = {
    .header.cf = LV_IMG_CF_TRUE_COLOR,
    .header.always_zero = 0,
    .header.reserved = 0,
    .header.w = 240,
    .header.h = 250,
    .data_size = 240 * 250 * 2,
    .data = 0,
};
static lv_obj_t *obj_img_map;
typedef struct
{
    char *path;
    uint8_t *data;
    uint16_t data_len;
} lp_map_data_t;
static lp_map_data_t lp_map_data[4] = {0};

static lv_area_t map_area = {
    .x1 = 0,
    .y1 = 0,
    .x2 = CANVAS_WIDTH,
    .y2 = CANVAS_HEIGHT,
};
const char *dirPath = "/C/Sys/MapFile";
#define PIC_TYPE 1
#ifdef PIC_TYPE
const char *extName = "png";
#else
const char *extName = "bin"; //"png";
#endif

static const float MinLatitude = -85.05112878;
static const float MaxLatitude = 85.05112878;
static const float MinLongitude = -180;
static const float MaxLongitude = 180;
static const float MATH_PI = 3.1415926535897932384626433832795;
static float Clip(float n, float minValue, float maxValue)
{
    if (n > minValue)
    {
        if (n > maxValue)
            return maxValue;
        return n;
    }
    else
    {
        if (minValue > maxValue)
            return maxValue;
        return minValue;
    }
}
uint32_t MapSize(int levelOfDetail)
{
    return (uint32_t)256 << levelOfDetail;
}
void LatLongToPixelXY(float latitude, float longitude, int levelOfDetail, int *pixelX, int *pixelY)
{
    gps_latitude = latitude;
    gps_longitude = longitude;

    latitude = Clip(latitude, MinLatitude, MaxLatitude);
    longitude = Clip(longitude, MinLongitude, MaxLongitude);

    float x = (longitude + 180) / 360;
    float sinLatitude = sin(latitude * MATH_PI / 180);
    float y = 0.5 - log((1 + sinLatitude) / (1 - sinLatitude)) / (4 * MATH_PI);

    uint32_t mapSize = MapSize(levelOfDetail);
    *pixelX = (int)Clip(x * mapSize + 0.5, 0, mapSize - 1);
    *pixelY = (int)Clip(y * mapSize + 0.5, 0, mapSize - 1);
}

static int image_src_init(uint8_t eanble)
{
    int index = 0;
    if (eanble)
    {
        map_gps_point = lpbsp_malloc(sizeof(lv_point_t) * 2048);
        for (index = 0; index < 4; index++)
            lp_map_data[index].path = lpbsp_malloc(48);
    }
    else
    {
        lpbsp_free(map_gps_point);
        for (index = 0; index < 4; index++)
        {
            if (lp_map_data[index].path)
                lpbsp_free(lp_map_data[index].path);
            if (lp_map_data[index].data)
                lpbsp_free(lp_map_data[index].data);
        }
    }
    return 0;
}
static void myconvert_color_depth(uint8_t *img, uint32_t px_cnt)
{
    uint16_t *img_argb = (uint16_t *)img;
    lv_color_t c;
    uint32_t i, k = 0;

    for (i = 0; i < px_cnt; i++)
    {
        c = lv_color_make(img[k], img[k + 1], img[k + 2]);
        img_argb[i] = c.full;
        k += 3;
    }
}
static void map_color_copy(uint16_t *img_data, int32_t x, int32_t y, int32_t img_x, int32_t img_y, uint32_t w, uint32_t h)
{
    int i, j, tw, th;

    tw = x + w;
    th = y + h;
    APP_LOGD("%d  %d -  %d %d - %d   %d\r\n", x, y, img_x, img_y, w, h);

    for (size_t i = y; i < th; i++)
    {
        for (size_t j = x; j < tw; j++)
        {
            map_data_buf[i * 240 + j] = img_data[(i + img_y - y) * 256 + j + img_x - x];
        }
    }
}

static int ConvertMapPath(int32_t x, int32_t y, int draw_x, int draw_y)
{
    char path[32];
    int ret = 0;
    uint16_t *img_data = NULL;
    unsigned png_width, png_height;
    lv_area_t res_p, r_p, a2_p;
    if (abs(draw_x) > 256 || abs(draw_y) > 256)
    {
        APP_LOGD("***** err x:%d,y:%d.\r\n", draw_x, draw_y);
        return 1;
    }

    snprintf(path, sizeof(path), "%s/%d/%ld/%ld.%s", dirPath, map_level, x, y, extName);

    ret = lodepng_decode24_file(&img_data, &png_width, &png_height, path);
    if (ret)
    {
        APP_LOGD("err:open path file %d,%s\r\n", lpbsp_ram_free_get(), path);
        return 1;
    }
    myconvert_color_depth(img_data, png_width * png_height);

    APP_LOGD("cx %d cy %d\r\n", draw_x, draw_y);

    a2_p.x1 = draw_x;
    a2_p.y1 = draw_y;
    a2_p.x2 = draw_x + 256;
    a2_p.y2 = draw_y + 256;
    if (_lv_area_intersect(&res_p, &map_area, &a2_p))
    {
        _lv_area_intersect(&r_p, &a2_p, &map_area);

        map_color_copy(img_data, res_p.x1, res_p.y1,
                       r_p.x1 - draw_x, r_p.y1 - draw_y,
                       res_p.x2 - res_p.x1, res_p.y2 - res_p.y1);
    }
    else
    {
        APP_LOGD("err _lv_area_intersect\r\n");
    }
    if (img_data != NULL)
        lpbsp_free(img_data);
    return 0;
}

int map_show_xy(int x, int y)
{
    char tbuf[44];
    int cx, cy, hw, hh;
    int tempx, tempy;

    // APP_LOGD("lev %d,x %d  y %d  %d-%d \n", lev, x, y, CANVAS_WIDTH, CANVAS_HEIGHT);

    hw = CANVAS_WIDTH / 2;
    hh = CANVAS_HEIGHT / 2; // 屏幕的中心点
    cx = x % 256;
    cy = y % 256;
    x /= 256;
    y /= 256;

    // APP_LOGD("%d %d ,cx %d cy %d\r\n", x, y, cx, cy);

    cx = hw - cx; //
    cy = hh - cy; // 它们在左上角的话，xy差距
    APP_LOGD(" cx %d cy %d\r\n", cx, cy);

    if (-16 > cx) //[ < -16]
    {
        if (-6 > cy) //[ < -6]
        {
            ConvertMapPath(x, y, cx, cy);
            ConvertMapPath(x + 1, y, cx + 256, cy);
            ConvertMapPath(x, y + 1, cx, cy + 256);
            ConvertMapPath(x + 1, y + 1, cx + 256, cy + 256);
        }
        else if (0 >= cy) // [-6 - 0]
        {
            ConvertMapPath(x, y, cx, cy);
            ConvertMapPath(x + 1, y, cx + 256, cy);
        }
        else // [ > 0]
        {
            ConvertMapPath(x, y, cx, cy);
            ConvertMapPath(x + 1, y, cx + 256, cy);
            ConvertMapPath(x, y - 1, cx, cy - 256);
            ConvertMapPath(x + 1, y - 1, cx + 256, cy - 256);
        }
    }
    else if (0 >= cx) // [-16 - 0]
    {
        ConvertMapPath(x, y, cx, cy);
        if (-6 > cy) //[ < -6]
        {
            ConvertMapPath(x, y + 1, cx, cy + 256);
        }
        else if (0 >= cy) // [-6 - 0]
        {
        }
        else // [ > 0]
        {
            ConvertMapPath(x, y - 1, cx, cy - 256);
        }
    }
    else // [ > 0]
    {
        if (-6 > cy) //[ < -6]
        {
            ConvertMapPath(x - 1, y, cx - 256, cy);
            ConvertMapPath(x, y, cx, cy);
            ConvertMapPath(x - 1, y + 1, cx - 256, cy + 256);
            ConvertMapPath(x, y + 1, cx, cy + 256);
        }
        else if (0 >= cy) // [-6 - 0]
        {
            ConvertMapPath(x - 1, y, cx - 256, cy);
        }
        else // [ > 0]
        {
            ConvertMapPath(x - 1, y - 1, cx - 256, cy - 256);
            ConvertMapPath(x - 1, y, cx - 256, cy);
            ConvertMapPath(x, y - 1, cx, cy - 256);
            ConvertMapPath(x, y, cx, cy);
        }
    }
    lv_obj_invalidate(obj_img_map);
    return 0;
}

static void timer_cb(lv_timer_t *timer)
{
    int step = 0, ret = 0;
    struct lp_gps_info_t gps_info = {.latitude = 0, .latitude = 0};
    int gpsx, gpsy, tx, ty;
    int dx, dy, i, getout = 0, j = 0;

    step = lp_sensor_accelerometer_step_read();
    ret = lp_gps_read(&gps_info);
    if (ret)
    {
        {
            LatLongToPixelXY(gps_info.latitude, gps_info.longitude, map_level, &gpsx, &gpsy);
            // APP_LOGD("%d,%d,%d\r\n", map_level, gpsx, gpsy);

            tx = gpsx - gps_x;
            ty = gpsy - gps_y;
            if ((abs(tx) > 75) || (abs(ty) > 70))
            {
                dx = gpsx - gps_x;
                dy = gpsy - gps_y;
                gps_x = gpsx;
                gps_y = gpsy;
                map_show_xy(gps_x, gps_y);
                tx = 120;
                ty = 125;
                for (i = 0; i < map_gps_point_size; i++)
                {
                    map_gps_point[i].x -= dx;
                    map_gps_point[i].y -= dy;

                    if (0 > map_gps_point[i].x || 0 > map_gps_point[i].y)
                    {
                        getout = i;
                    }
                }
                if (getout)
                {
                    for (i = getout; i < map_gps_point_size; i++)
                    {
                        map_gps_point[j].x = map_gps_point[i].x;
                        map_gps_point[j].y = map_gps_point[i].y;
                        j++;
                    }
                    map_gps_point_size -= getout;
                }
            }
            else
            {
                tx = 120 + tx;
                ty = 125 + ty;
            }
            lv_obj_set_pos(gps_lab, tx - GPS_COMPASS_WIDTH, ty - GPS_COMPASS_WIDTH);

            if (map_gps_point_size && map_gps_point[map_gps_point_size - 1].x == tx && map_gps_point[map_gps_point_size - 1].y == ty)
            {
                // APP_LOGD("repl point\r\n");
                return;
            }
            map_gps_point[map_gps_point_size].x = tx;
            map_gps_point[map_gps_point_size].y = ty;
            map_gps_point_size++;
            lv_line_set_points(gps_line, map_gps_point, map_gps_point_size);
            if (GPS_POINT_NUM == map_gps_point_size)
            {
                map_gps_point_size = GPS_POINT_NUM - 30;
            }
            // APP_LOGD("map_gps_point_size %d\r\n", map_gps_point_size);
        }
    }
}
static void view_page_close_cb(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);

    if (LV_EVENT_DELETE == code)
    {
        image_src_init(0);
        lvgl_sleep_time_ms(15000);
        lpbsp_free(map_data_buf);
        lv_timer_del(timer);
        timer = 0;
    }
}
static void main_page_btn_up_callback(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    int pixelX, pixelY;

    if (code == LV_EVENT_SHORT_CLICKED)
    {
        if (16 == map_level)
            return;
        map_level += 2;
        if (map_level > 16)
            map_level = 16;
        LatLongToPixelXY(gps_latitude, gps_longitude, map_level, &pixelX, &pixelY);
        map_show_xy(pixelX, pixelY);
    }
}
static void main_page_btn_down_callback(lv_event_t *e)
{
    lv_event_code_t code = lv_event_get_code(e);
    int pixelX, pixelY;

    if (code == LV_EVENT_SHORT_CLICKED)
    {
        if (4 == map_level)
            return;
        map_level -= 2;
        if (4 > map_level)
            map_level = 4;

        LatLongToPixelXY(gps_latitude, gps_longitude, map_level, &pixelX, &pixelY);
        map_show_xy(pixelX, pixelY);
    }
}
// 2022年2月10日 map界面
int view_page_map(lv_obj_t *last_obj)
{
    int i = 0;
    lv_obj_t *btn_obj, *lab_obj;

    map_src = last_obj;

    lvgl_sleep_time_ms(0xfffffff);
    image_src_init(1);
    lv_obj_add_event_cb(last_obj, view_page_close_cb, LV_EVENT_DELETE, 0);

    map_data_buf = lpbsp_malloc(CANVAS_WIDTH * CANVAS_HEIGHT * 2);
    map_data.data = map_data_buf;

    obj_img_map = lv_img_create(last_obj);
    lv_obj_set_pos(obj_img_map, 0, 0);
    lv_obj_set_size(obj_img_map, 240, 250);
    lv_img_set_src(obj_img_map, &map_data);

    gps_line = lv_line_create(obj_img_map);
    lv_obj_set_style_line_color(gps_line, lv_color_make(255, 0, 0), 0);
    lv_obj_set_style_line_width(gps_line, 3, 0);

    LV_IMG_DECLARE(lvgl_img_gps_compass)
    gps_lab = lv_img_create(obj_img_map);
    lv_obj_set_pos(gps_lab, 120 - GPS_COMPASS_WIDTH, 125 - GPS_COMPASS_WIDTH);
    lv_img_set_src(gps_lab, &lvgl_img_gps_compass);

    btn_obj = lv_btn_create(obj_img_map);
    lv_obj_set_pos(btn_obj, 0, 80);
    lv_obj_set_size(btn_obj, 60, 60);
    lv_obj_set_style_bg_opa(btn_obj, 0, 0);
    lab_obj = lv_label_create(btn_obj);
    lv_label_set_text(lab_obj, LV_SYMBOL_PLUS); // +
    lv_obj_set_align(lab_obj, LV_ALIGN_CENTER);
    lv_obj_add_event_cb(btn_obj, main_page_btn_up_callback, LV_EVENT_SHORT_CLICKED, NULL);

    btn_obj = lv_btn_create(obj_img_map);
    lv_obj_set_pos(btn_obj, 0, 160);
    lv_obj_set_size(btn_obj, 60, 60);
    lv_obj_set_style_bg_opa(btn_obj, 0, 0);
    lab_obj = lv_label_create(btn_obj);
    lv_label_set_text(lab_obj, LV_SYMBOL_MINUS); // -
    lv_obj_set_align(lab_obj, LV_ALIGN_CENTER);
    lv_obj_add_event_cb(btn_obj, main_page_btn_down_callback, LV_EVENT_SHORT_CLICKED, NULL);

    LatLongToPixelXY(22.536492, 113.929381, map_level, &gps_x, &gps_y);
    map_show_xy(gps_x, gps_y);

    timer = lv_timer_create(timer_cb, 900, NULL);

    return 0;
}