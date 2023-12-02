#include "lpbsp.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/touch_pad.h"
#include "driver/spi_master.h"
#include <stdio.h>
#include <time.h>
#include <sys/time.h>
#include "soc/soc_caps.h"
#include "esp_sleep.h"
#include "driver/adc.h"
#include "driver/rtc_io.h"
#include "soc/rtc.h"
#include "soc/gpio_struct.h"
#include "hal/gpio_hal.h"
#include "esp_system.h"
#include "esp_event.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "driver/ledc.h"
#include "lwip/err.h"
#include "lwip/sockets.h"
#include "esp_adc_cal.h"
#include "driver/i2c.h"
#include "esp_task_wdt.h"
#include "esp_flash.h"
#include "esp_pm.h"
#include "esp_timer.h"
#include "esp_efuse.h"
#include "esp_efuse_table.h"
#include "esp_spi_flash.h"
///////////////////////wifi/////////////////////
static uint32_t lpbsp_error_value = 0; // 上一个错误值
static uint8_t wifi_init_flag = 0;     // wifi状态
static uint32_t wifi_event_group = 0;  // 事件标记
static wifi_ap_record_t *ap_info = 0;  // wifi list
static uint16_t ap_count = 0;          // wifi num
static char *ipstring[32] = {0};       // 本机ip地址
int lpbsp_wifi_stop(void);
///////////////////////wifi/////////////////////
#if 0
#define BSP_LOG(format, ...) printf("%04d,%16s:%04d " format, lpbsp_tick_ms(), __FILE__, __LINE__, ##__VA_ARGS__)
#else
#define BSP_LOG(...)
#endif
static int lpbsp_sensor_magnetic_write(uint8_t reg, uint8_t data);
void sd_fat_init(uint8_t eanble);
int lpbsp_sleep_config_cpu(int ms);

uint32_t lpbsp_error(void)
{
    return lpbsp_error_value;
}
#define TWDT_TIMEOUT_S 6

static QueueHandle_t gpio_evt_queue = NULL;
static int charge_flg = 0;
static void IRAM_ATTR gpio_isr_handler(void *arg)
{
    uint8_t gpio_num = (uint32_t)arg;

    {
        xQueueSendFromISR(gpio_evt_queue, &gpio_num, NULL);
    }
}

int lpbsp_iqr_event(uint32_t *io)
{
    uint8_t io_num;
    if (xQueueReceive(gpio_evt_queue, &io_num, 0))
    {
        // return io_num;
        return 7;
    }
    return 0;
}
void lpbsp_iqr_event_clear(void)
{
    xQueueGenericReset(gpio_evt_queue, 1);
}
void lpbsp_gpio_disable_num(int num)
{
    gpio_config_t io_conf = {
        .pull_down_en = 0,              // disable pull-down mode
        .pull_up_en = 0,                // disable pull-up mode
        .intr_type = GPIO_INTR_DISABLE, // disable interrupt
        .mode = GPIO_MODE_DISABLE,
        .pin_bit_mask = 1ULL << num,
    };

    gpio_config(&io_conf);

    if (22 > num)
        rtc_gpio_isolate(num);
}
void lpbsp_gpio_eanble_num(int num)
{
    if (22 > num)
        rtc_gpio_hold_dis(num);

    gpio_config_t io_conf = {
        .pull_down_en = 0,              // disable pull-down mode
        .pull_up_en = 1,                // disable pull-up mode
        .intr_type = GPIO_INTR_DISABLE, // disable interrupt
        .mode = GPIO_MODE_OUTPUT,
        .pin_bit_mask = 1ULL << num,
    };
    gpio_config(&io_conf);
}
#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE
#define LEDC_CHANNEL LEDC_CHANNEL_0
static void tft_led_init(int eanble)
{
    if (eanble)
    {
        lpbsp_gpio_eanble_num(PIN_NUM_BCKL);

        // Prepare and then apply the LEDC PWM timer configuration
        ledc_timer_config_t ledc_timer = {
            .speed_mode = LEDC_MODE,
            .timer_num = LEDC_TIMER,
            .duty_resolution = LEDC_TIMER_10_BIT, // 1024
            .freq_hz = 5000,                      // Set output frequency at 1 kHz
            .clk_cfg = LEDC_AUTO_CLK};
        ESP_ERROR_CHECK(ledc_timer_config(&ledc_timer));

        // Prepare and then apply the LEDC PWM channel configuration
        ledc_channel_config_t ledc_channel = {
            .speed_mode = LEDC_MODE,
            .channel = LEDC_CHANNEL,
            .timer_sel = LEDC_TIMER,
            .intr_type = LEDC_INTR_DISABLE,
            .gpio_num = PIN_NUM_BCKL,
            .duty = 1024, // Set duty to 0%
            .hpoint = 0};
        ESP_ERROR_CHECK(ledc_channel_config(&ledc_channel));
    }
    else
    {
        ledc_stop(LEDC_MODE, LEDC_CHANNEL, 1);
        esp_rom_gpio_pad_select_gpio(PIN_NUM_BCKL);
        gpio_set_direction(PIN_NUM_BCKL, GPIO_MODE_DISABLE);
        rtc_gpio_isolate(PIN_NUM_BCKL);
    }
}
void lpbsp_gpio_disable(void)
{
    int i = 0, ret;
    gpio_config_t io_conf = {};

    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 0;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_DISABLE;

    gpio_uninstall_isr_service();
    // gpio_isr_handler_remove(PIN_NUM_TP_INT);
    for (i = 0; i < 22; i++)
    {
        if (i == PIN_NUM_TP_INT)
            continue;
        io_conf.pin_bit_mask = 1ULL << i;
        ret = gpio_config(&io_conf);
        if (ESP_OK != ret)
        {
            BSP_LOG("error %d\r\n", i);
        }
    }
    for (i = 33; i < 43; i++)
    {
        io_conf.pin_bit_mask = 1ULL << i;
        ret = gpio_config(&io_conf);
        if (ESP_OK != ret)
        {
            BSP_LOG("error %d\r\n", i);
        }
    }
    io_conf.pin_bit_mask = 1ULL << 45;
    ret = gpio_config(&io_conf);
    io_conf.pin_bit_mask = 1ULL << 46;
    ret = gpio_config(&io_conf);

    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pin_bit_mask = 1ULL << PIN_NUM_GPS_POWER;
    gpio_config(&io_conf);
    lpbsp_io_set(PIN_NUM_GPS_POWER, 0);
}

void lpbsp_gpio_init(uint8_t eanble)
{
    gpio_config_t io_conf = {};

    io_conf.pull_down_en = 0;
    io_conf.pull_up_en = 1;
    io_conf.intr_type = GPIO_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;

    lpbsp_gpio_eanble_num(PIN_NUM_TP_INT);
    lpbsp_gpio_eanble_num(PIN_NUM_NFC_INT);
    lpbsp_gpio_eanble_num(PIN_NUM_CHARGE);
    lpbsp_gpio_eanble_num(PIN_NUM_SENSOR_INT);

    io_conf.pin_bit_mask = 1ULL << PIN_NUM_GPS_POWER;
    gpio_config(&io_conf);
    lpbsp_io_set(PIN_NUM_GPS_POWER, 0);

    gpio_install_isr_service(0);

    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.intr_type = GPIO_INTR_ANYEDGE;

    io_conf.pin_bit_mask = 1ULL << PIN_NUM_TP_INT;
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = 1ULL << PIN_NUM_NFC_INT;
    gpio_config(&io_conf);

    io_conf.pin_bit_mask = 1ULL << PIN_NUM_CHARGE;
    gpio_config(&io_conf);
    // gpio_set_drive_capability(PIN_NUM_CHARGE, 1);

    io_conf.intr_type = GPIO_INTR_NEGEDGE;
    io_conf.pin_bit_mask = 1ULL << PIN_NUM_SENSOR_INT;
    gpio_config(&io_conf);

    gpio_evt_queue = xQueueCreate(64, sizeof(uint8_t));

    gpio_isr_handler_add(PIN_NUM_TP_INT, gpio_isr_handler, (void *)PIN_NUM_TP_INT);

    gpio_isr_handler_add(PIN_NUM_NFC_INT, gpio_isr_handler, (void *)PIN_NUM_NFC_INT);

    // gpio_isr_handler_add(PIN_NUM_CHARGE, gpio_isr_handler, (void *)PIN_NUM_CHARGE);

    gpio_isr_handler_add(PIN_NUM_SENSOR_INT, gpio_isr_handler, (void *)PIN_NUM_SENSOR_INT);
}
int lpbsp_gps_uart_init(int type)
{
#define ECHO_UART_PORT_NUM 1
#define BUF_SIZE (512)

    if (type)
    {
        uart_config_t uart_config = {
            .baud_rate = 9600,
            .data_bits = UART_DATA_8_BITS,
            .parity = UART_PARITY_DISABLE,
            .stop_bits = UART_STOP_BITS_1,
            .flow_ctrl = UART_HW_FLOWCTRL_DISABLE,
            .source_clk = UART_SCLK_REF_TICK,
        };

        ESP_ERROR_CHECK(uart_driver_install(ECHO_UART_PORT_NUM, BUF_SIZE, 0, 0, NULL, 0));
        ESP_ERROR_CHECK(uart_param_config(ECHO_UART_PORT_NUM, &uart_config));
        ESP_ERROR_CHECK(uart_set_pin(ECHO_UART_PORT_NUM, PIN_NUM_UART_TX, PIN_NUM_UART_RX, -1, -1));
    }
    else
    {
        ESP_ERROR_CHECK(uart_driver_delete(ECHO_UART_PORT_NUM));
    }
    return 0;
}

///////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////
int lpbsp_uart_init(int num, int type)
{
    BSP_LOG("init uart [%d,%d]\r\n", num, type);
    if (type)
    {
        gpio_config_t io_conf = {};
        io_conf.pull_down_en = 0;
        io_conf.pull_up_en = 1;
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = 1ULL << PIN_NUM_GPS_POWER;
        gpio_config(&io_conf);
    }
    lpbsp_io_set(PIN_NUM_GPS_POWER, type); // open/close gps
    lpbsp_gps_uart_init(type);
    return -1;
}
int lpbsp_uart_write(int num, uint8_t *data, int size)
{
    return uart_write_bytes(num, data, size);
}
int lpbsp_uart_read(int num, uint8_t *data, int size)
{
    uint32_t length = 0;
    uart_get_buffered_data_len(num, (size_t *)&length);
    if (length)
    {
        if (length > size)
            return uart_read_bytes(num, data, size, 100);
        else
        {
            data[length] = 0;
            return uart_read_bytes(num, data, length, 100);
        }
    }
    return 0;
}

static int s_retry_num = 0;

static void wifi_event_handler(void *arg, esp_event_base_t event_base,
                               int32_t event_id, void *event_data)
{
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        BSP_LOG("WIFI_EVENT_STA_START comm\r\n");
        esp_wifi_connect();
    }
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        if (s_retry_num == 0)
        {
            esp_wifi_connect();
            s_retry_num++;
            BSP_LOG("retry to connect to the AP s_retry_num =%d\r\n", s_retry_num);
        }
        else
        {
            s_retry_num = 0;
            BSP_LOG("connect to the AP fail\r\n");
            // xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
        }
    }
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
    {
        ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
        BSP_LOG("got ip:" IPSTR "\r\n", IP2STR(&event->ip_info.ip));
        sprintf(ipstring, IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        wifi_event_group = 1;
        // xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
    else if (event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        BSP_LOG("SYSTEM_EVENT_STA_DISCONNECTED\r\n");
        wifi_event_group = 0;
    }
    else if (event_id == WIFI_EVENT_SCAN_DONE)
    {
        esp_wifi_scan_get_ap_num(&ap_count);
        BSP_LOG("Number of access points found: %d\n", ap_count);
        if (!ap_count)
        {
            BSP_LOG("Nothing AP found\r\n");
        }
        else
        {
            if (ap_info) // 没有读取就释放
            {
                lpbsp_free(ap_info);
            }
            ap_info = lpbsp_malloc(sizeof(wifi_ap_record_t) * ap_count);
            ESP_ERROR_CHECK(esp_wifi_scan_get_ap_records(&ap_count, ap_info));
            for (int i = 0; i < ap_count; i++)
            {
                BSP_LOG("[%02d]MAC:%02x%02x%02x%02x%02x%02x,rssi:%d,SSID:%s\r\n", i,
                        ap_info[i].bssid[0], ap_info[i].bssid[1], ap_info[i].bssid[2],
                        ap_info[i].bssid[3], ap_info[i].bssid[4], ap_info[i].bssid[5],
                        ap_info[i].rssi, ap_info[i].ssid);
            }
        }
    }
}
int lpbsp_init(uint8_t eanble)
{
    BSP_LOG("lpbsp_init %d\r\n", eanble);
    if (eanble)
    {
        for (int i = 0; i < 22; i++)
            rtc_gpio_hold_dis(i);
        lpbsp_gpio_init(1);
    }
    return 0;
}
int lpbsp_init_2(uint8_t eanble)
{
    {
        gpio_config_t io_conf = {
            .pull_down_en = 0,              // disable pull-down mode
            .pull_up_en = 0,                // disable pull-up mode
            .intr_type = GPIO_INTR_DISABLE, // disable interrupt
            .mode = GPIO_MODE_OUTPUT,
            .pin_bit_mask = 1ULL << PIN_NUM_MOTO,
        };
        gpio_config(&io_conf);

        esp_err_t err = nvs_flash_init();
        if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND)
        {

            ESP_ERROR_CHECK(nvs_flash_erase());
            err = nvs_flash_init();
        }
        ESP_ERROR_CHECK(err);
        ESP_ERROR_CHECK(esp_event_loop_create_default());
        ESP_ERROR_CHECK(esp_netif_init());

        esp_netif_create_default_wifi_sta();
    }
    return 0;
}
int lpbsp_restart(void)
{
    esp_restart();
    return 0;
}

int lpbsp_moto_set(int type)
{
    gpio_set_level(PIN_NUM_MOTO, type);
    return 0;
}
int lpbsp_bl_set(int value)
{
    static char last_value = 0;

    BSP_LOG("bl =%d \r\n", value);
    if (0 == value)
    {
        if (last_value)
        {
            tft_led_init(0);
        }

        lpbsp_gpio_disable_num(PIN_NUM_BCKL2);
        lpbsp_gpio_disable_num(PIN_NUM_BCKL);
    }
    else
    {
        if (0 == last_value)
        {
            tft_led_init(1);
        }

        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, LEDC_CHANNEL, (100 - value) * 10 + 10));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, LEDC_CHANNEL));
    }
    last_value = value;
    return 0;
}
int lpbsp_io_set(int io, int value)
{
    gpio_set_level(io, value);
    return 0;
}
int lpbsp_io_get(int io)
{
    return gpio_get_level(io);
}
int lpbsp_delayms(int ms)
{
    vTaskDelay(pdMS_TO_TICKS(ms));
    return 0;
}
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////
/*
    SLEEP_MODE_DEEP_LOW_POEWR = 0, //低电 只能充电才能激活 屏不亮
    SLEEP_MODE_DEEP = 1,           //充电 和 tp 屏不亮 不自动醒来
    SLEEP_MODE_DEEP_BL = 2,        //充电 和 tp 屏亮 60内醒来
    SLEEP_MODE_LIGHT = 3,          //充电 和 tp 灭屏
    SLEEP_MODE_LIGHT_BL = 4,          //充电 和 tp 屏亮 60内醒来
*/
int lpbsp_sleep_config(uint32_t sleep_mode, int ms)
{
    BSP_LOG("ms %d\r\n", ms);

    if (2 == sleep_mode)
    {
        if (ms)
            esp_sleep_enable_timer_wakeup(ms * 1000);
        esp_light_sleep_start();
        return 0;
    }

    lpbsp_wifi_stop();

    // lpbsp_uart_init(1, 0);
    lpbsp_sensor_magnetic_write(0x0a, 0);
    sd_fat_init(0);
    gpio_set_level(PIN_NUM_RST, 0);
    lpbsp_delayms(150);
    gpio_set_level(PIN_NUM_RST, 1);
    lpbsp_gpio_disable_num(PIN_NUM_RST);
    lpbsp_iic_init(0, 0);
    lpbsp_bl_set(0);
    lpbsp_bat_init(0);

    lpbsp_sleep_config_cpu(ms);
    return 0;
}
int lpbsp_sleep_config_cpu(int ms)
{
    lpbsp_gpio_disable();
    for (int i = 0; i < 22; i++)
        if (4 != i && PIN_NUM_TP_INT != i)
            rtc_gpio_isolate(i);

    rtc_gpio_set_direction(PIN_NUM_GPS_POWER, RTC_GPIO_MODE_OUTPUT_ONLY);
    rtc_gpio_set_level(PIN_NUM_GPS_POWER, 0);
    rtc_gpio_hold_en(PIN_NUM_GPS_POWER);

    // 这里不延时 4初始化的时候会让tp复位 导致暂时低电平 触发唤醒
    if (gpio_get_level(PIN_NUM_TP_INT) == 0) /* Wait until GPIO goes high */
    {
        do
        {
            BSP_LOG("wiat\r\n");
            lpbsp_delayms(100);
        } while (gpio_get_level(PIN_NUM_TP_INT) == 0);
    }
    ESP_ERROR_CHECK(esp_sleep_enable_ext0_wakeup(PIN_NUM_TP_INT, 0));
    if (ms)
        esp_sleep_enable_timer_wakeup(ms * 1000);

    esp_deep_sleep_start(); // 会重启的

    return 0;
}

int lpbsp_sleep_wakeup_cause(void)
{
    int cause = esp_sleep_get_wakeup_cause();

    BSP_LOG("case %d\r\n", cause);

    return cause;
}

int lpbsp_task_create(char *name, // task name
                      void *arg,
                      task_function_t fun,
                      uint32_t stack_depth,
                      char lev)
{
    TaskHandle_t *pxCreatedTask;

    pxCreatedTask = malloc(sizeof(TaskHandle_t));
    // xTaskCreatePinnedToCore(fun, name, stack_depth, arg, 0, pxCreatedTask, 1);
    xTaskCreate(fun, name, stack_depth, arg, 1, pxCreatedTask);
    return pxCreatedTask;
}
int lpbsp_task_delete(void *p)
{
    if (p)
    {
        vTaskDelete(*(TaskHandle_t *)p);
        free(p);
    }
    else
    {
        vTaskDelete(0);
    }
    return 0;
}
int lpbsp_task_stop(void *p)
{
    vTaskSuspend(p);
    return 0;
}
int lpbsp_task_start(void *p)
{
    vTaskResume(p);
    return 0;
}

int lpbsp_ram_min_get(void)
{
    return esp_get_minimum_free_heap_size();
}

int lpbsp_ram_free_get(void)
{
    return esp_get_free_heap_size();
}

int lpbsp_time_create(time_function_t arg)
{
    esp_timer_create_args_t periodic_timer_args = {
        .skip_unhandled_events = true,
        .callback = arg};
    esp_timer_handle_t periodic_timer;
    ESP_ERROR_CHECK(esp_timer_create(&periodic_timer_args, &periodic_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(periodic_timer, 1000));

    return 1;
}
int lpbsp_rtc_set(unsigned int sec)
{
    struct timeval now = {.tv_sec = sec};
    struct timezone tz = {0, 0};
    settimeofday(&now, &tz);
    return 0;
}
unsigned int lpbsp_rtc_get(void)
{
    return time(0);
}
/*
5.1 8192 - 16
5.1.1 8192
*/
RTC_NOINIT_ATTR uint8_t rtc_ram_buf1[8192];
uint8_t *lpbsp_rtc_ram_get(int *ram_size)
{
    if (ram_size)
        *ram_size = sizeof(rtc_ram_buf1);
    return rtc_ram_buf1;
}
void *lpbsp_malloc(uint32_t size)
{
    return (void *)heap_caps_malloc(size, MALLOC_CAP_SPIRAM);
}
void lpbsp_free(void *ram)
{
    heap_caps_free(ram);
}
void *lpbsp_realloc(void *ram, uint32_t size)
{
    return heap_caps_realloc(ram, size, MALLOC_CAP_SPIRAM);
}
void *lpbsp_calloc(uint32_t n, uint32_t size)
{
    return (void *)heap_caps_realloc(n, size, MALLOC_CAP_SPIRAM);
}
int lpbsp_stack_mini_get(void)
{
    return uxTaskGetStackHighWaterMark(NULL);
}

/////////////////////////////////////////////////////////////////////////////////
static void lpesp_wdt_isr_user_handler(void)
{
    esp_restart();
}
void lpbsp_wdt_add(void)
{
    esp_task_wdt_add(NULL);
    esp_task_wdt_status(NULL);
}
void lpbsp_wdt_feed(void)
{
    esp_task_wdt_reset(); // Comment this line to trigger a TWDT timeout
}
void lpbsp_wdt_del(void)
{
    esp_task_wdt_delete(NULL); // Unsubscribe task from TWDT
    esp_task_wdt_status(NULL); // ESP_ERR_NOT_FOUND Confirm task is unsubscribed
}

/////////////////////////////////////////////////////////////////////////////////
void *lpbsp_queue_create(int uxQueueLength, int uxItemSize)
{
    return xQueueCreate(uxQueueLength, uxItemSize);
}

int lpbsp_queue_send(void *xQueue, void *pvItemToQueue, int wait_ms)
{
    return xQueueSend(xQueue, pvItemToQueue, wait_ms);
}

int lpbsp_queue_receive(void *xQueue, void *pvItemToQueue, int wait_ms)
{
    return xQueueReceive(xQueue, pvItemToQueue, wait_ms);
}

///////////////////////////////////////////////////////////////////////////////
///////////////////////wifi/////////////////////

uint32_t lpbsp_wifi_event(uint32_t event)
{
    if (event)
        return wifi_event_group & event; // 事件标记
    else
        return wifi_event_group;
}
int lpbsp_wifi_scan_ap_info(lpbsp_wifi_ap_info_t *ap, int max_size)
{
    int i = 0;

    if (0 == ap_count)
    {
        return 0;
    }

    for (i = 0; (i < max_size) && (i < ap_count); i++)
    {
        memcpy(ap[i].bssid, ap_info[i].bssid, 6);
        memcpy(ap[i].ssid, ap_info[i].ssid, 32);
        ap[i].rssi = ap_info[i].rssi;
        ap[i].wifi_auth_mode = ap_info[i].authmode;
    }
    ap_count = 0;
    free(ap_info);
    ap_info = 0;
    return i;
}
int lpbsp_wifi_scan_ap_num(void)
{
    return ap_count;
}
int lpbsp_wifi_addr_ip(char *ip)
{
    strcpy((char *)ip, (const char *)ipstring);
    return strlen((const char *)ip);
}

int lpbsp_wifi_stop(void)
{
    esp_err_t err = esp_wifi_stop();
    wifi_event_group = 0;
    if (err == ESP_ERR_WIFI_NOT_INIT)
    {
        return 0;
    }
    ESP_ERROR_CHECK(err);
    ESP_ERROR_CHECK(esp_wifi_deinit());
    wifi_init_flag = 0;
    return 0;
}
static void lpbsp_wifi_config_init(void)
{

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &wifi_event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_start());

    esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
}
int lpbsp_wifi_init(int mode)
{
    if (0 == mode) // deinit
    {
        wifi_init_flag = 0;
        lpbsp_wifi_stop();
        wifi_event_group = 0;
        BSP_LOG("wifi stop\r\n");
    }
    else if (1 == mode) // init
    {
        wifi_init_flag = 1;
        BSP_LOG("wifi init\r\n");
        lpbsp_wifi_config_init();
    }
    return 0;
}
int lpbsp_wifi_scan(void)
{
    int ret = 0;

    if (1 != wifi_init_flag)
        lpbsp_wifi_init(1);

    ret = esp_wifi_scan_start(NULL, false);
    return ret;
}

int lpbsp_wifi_connect(char *ssid, char *password)
{
    wifi_config_t wifi_config;
    int ret = 0;

    if (1 != wifi_init_flag)
        lpbsp_wifi_init(1);

    memset(&wifi_config, 0, sizeof(wifi_config));
    strcpy((char *)&wifi_config.sta.ssid, (const char *)ssid);
    strcpy((char *)&wifi_config.sta.password, (const char *)password);

    BSP_LOG("Connecting to %s..[%s].\r\n", wifi_config.sta.ssid, wifi_config.sta.password);

    wifi_config.sta.listen_interval = 10;
    ret = esp_wifi_set_config(WIFI_IF_STA, &wifi_config);
    if (0 == ret)
    {
        ret = esp_wifi_connect();
    }
    return ret;
}

int lpbsp_socket(int af, int type, int protocol)
{
    return socket(af, type, protocol);
}
int lpbsp_socket_ip4_tcp(void)
{
    return lpbsp_socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
}

int lpbsp_socket_connect(int sock, const struct sockaddr *serv_addr, int addrlen) // Windows
{
    return connect(sock, serv_addr, addrlen);
}

int lpbsp_socket_connect_ip(int sock, char *ip, int port) // Windows
{
    int ret = 0;
    // u_long non_blocking = 1;
    struct sockaddr_in serv_addr_in;

    // BSP_LOG("%s:%d\r\n", ip, port); // 127.127.127.127

    serv_addr_in.sin_family = AF_INET;
    serv_addr_in.sin_port = htons(port);
    serv_addr_in.sin_addr.s_addr = inet_addr(ip);

    ret = lpbsp_socket_connect(sock, (struct sockaddr *)&serv_addr_in, sizeof(struct sockaddr));

    // ioctlsocket(sock, FIONBIO, &non_blocking); // 设置为非阻塞

    return ret;
}

int lpbsp_socket_rx_time_out(int s, int time_out_ms)
{
    struct timeval receiving_timeout;
    receiving_timeout.tv_sec = 0;
    receiving_timeout.tv_usec = time_out_ms;
    return setsockopt(s, SOL_SOCKET, SO_RCVTIMEO, &receiving_timeout,
                      sizeof(receiving_timeout));
}

int lpbsp_socket_connect_url(char *rul, uint32_t port)
{
    int sock = -1, ret = 0;

    sock = lpbsp_socket_ip4_tcp();

    ret = lpbsp_socket_connect_ip(sock, rul, port);
    if (0 > ret)
    {
        return -1;
    }

    return sock;
}
int lpbsp_socket_close(int fd)
{
    return close(fd);
}

int lpbsp_socket_write(int fd, const uint8_t *buf, int nbytes)
{
    return send(fd, buf, nbytes, 0);
}

int lpbsp_socket_read(int fd, uint8_t *buf, int nbytes)
{
    return recv(fd, buf, nbytes, 0);
}

unsigned int lpbsp_tick_ms(void)
{
    return clock();
}

///////////////////////wifi/////////////////////
static esp_adc_cal_characteristics_t *adc_chars;
static void print_char_val_type(esp_adc_cal_value_t val_type)
{
    if (val_type == ESP_ADC_CAL_VAL_EFUSE_TP)
    {
        BSP_LOG("Characterized using Two Point Value\n");
    }
    else if (val_type == ESP_ADC_CAL_VAL_EFUSE_VREF)
    {
        BSP_LOG("Characterized using eFuse Vref\n");
    }
    else
    {
        BSP_LOG("Characterized using Default Vref\n");
    }
}

int lpbsp_bat_get_mv(void)
{
    uint32_t adc_reading = 0;
    for (int i = 0; i < 16; i++)
        adc_reading += adc1_get_raw(ADC1_CHANNEL_0);

    adc_reading /= 16;
    uint32_t voltage = (esp_adc_cal_raw_to_voltage(adc_reading, adc_chars) + 85) * 2;
    return voltage;
}
int lpbsp_charge_get(void)
{
    return !lpbsp_io_get(PIN_NUM_CHARGE);
}
int lpbsp_bat_init(uint8_t eanble)
{
    if (eanble)
    {
        rtc_gpio_hold_dis(1);
        if (esp_adc_cal_check_efuse(ESP_ADC_CAL_VAL_EFUSE_TP) == ESP_OK)
        {
            BSP_LOG("eFuse Two Point: Supported\n");
            adc1_config_width(ADC_WIDTH_BIT_13);
            adc1_config_channel_atten(ADC1_CHANNEL_0, ADC_ATTEN_DB_11);

            adc_chars = calloc(1, sizeof(esp_adc_cal_characteristics_t));
            esp_adc_cal_value_t val_type = esp_adc_cal_characterize(ADC_UNIT_1,
                                                                    ADC_ATTEN_DB_11,
                                                                    ADC_WIDTH_BIT_13,
                                                                    1100,
                                                                    adc_chars);
            print_char_val_type(val_type);
        }
        else
        {
            BSP_LOG("Cannot retrieve eFuse Two Point calibration values. Default calibration values will be used.\n");
        }
    }
    else
    {
        lpbsp_gpio_disable_num(1);
    }
    return 0;
}
/////////////////////////////////////////////////////////////
static esp_err_t myi2c_master_init(uint8_t eanble)
{
    int ret = 0;
    int i2c_master_port = I2C_MASTER_NUM;

    BSP_LOG("myi2c_master_init %d\r\n", eanble);
    if (eanble)
    {
        // lpbsp_gpio_eanble_num(I2C_MASTER_SCL_IO);
        // lpbsp_gpio_eanble_num(I2C_MASTER_SDA_IO);

        i2c_config_t conf = {
            .mode = I2C_MODE_MASTER,
            .sda_io_num = I2C_MASTER_SDA_IO,
            .sda_pullup_en = GPIO_PULLUP_ENABLE,
            .scl_io_num = I2C_MASTER_SCL_IO,
            .scl_pullup_en = GPIO_PULLUP_ENABLE,
            .master.clk_speed = 400000,
            // .clk_flags = 0,          /*!< Optional, you can use I2C_SCLK_SRC_FLAG_* flags to choose i2c source clock here. */
        };
        esp_err_t err = i2c_param_config(i2c_master_port, &conf);
        if (err != ESP_OK)
        {
            BSP_LOG("i2c_param_config err=%d\r\n", err);
            return err;
        }
        ret = i2c_driver_install(i2c_master_port, conf.mode, I2C_MASTER_RX_BUF_DISABLE, I2C_MASTER_TX_BUF_DISABLE, 0);

        BSP_LOG("i2c_driver_install [%d]\r\n", ret);
    }
    else
    {
        ret = i2c_driver_delete(i2c_master_port);
        BSP_LOG("i2c_driver_delete [%d]\r\n", ret);

        lpbsp_gpio_disable_num(I2C_MASTER_SCL_IO);
        lpbsp_gpio_disable_num(I2C_MASTER_SCL_IO);
    }
    return ret;
}

#define I2C_MASTER_TIMEOUT_MS 50
int lpbsp_iic_init(int num, int eanble)
{
    esp_err_t ret = 0;
    myi2c_master_init(eanble);

    return ret;
}
// data[0]=addr,data[1]=reg,data[2]=data   size =3
int lpbsp_iic_write(int num, uint8_t *data, int size)
{
    int ret = 0;
    if (0 > num)
        return -10;
    ret = i2c_master_write_to_device(I2C_MASTER_NUM, data[0], &data[1], size - 1, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    return ret;
}
// data[0]=addr,data[1]=reg0 size= read size bytes
int lpbsp_iic_read(int num, uint8_t *data, int size)
{
    int ret = 0;
    uint8_t reg_addr = data[1];

    if (0 > num)
        return -10;
    ret = i2c_master_write_read_device(I2C_MASTER_NUM, data[0], &reg_addr, 1, data, size, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    return ret;
}
// data[0]=addr,data[1]=reg0,data[2]=reg1 addr_size=   size= read size bytes
int lpbsp_iic_read_bytes(int num, uint8_t *data, int addr_size, int size)
{
    int ret = 0, i;
    uint8_t reg_addr[4];

    for (i = 0; i < addr_size; i++)
    {
        reg_addr[i] = data[1 + i];
    }

    if (0 > num)
        return -10;
    ret = i2c_master_write_read_device(I2C_MASTER_NUM, data[0], reg_addr, addr_size, data, size, I2C_MASTER_TIMEOUT_MS / portTICK_PERIOD_MS);
    return ret;
}
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////////////////////
static spi_device_handle_t tft_spi;
static uint16_t *tft_buf;
static uint32_t *tft_buf32;

static void lcd_cmd(const uint8_t cmd)
{
    esp_err_t ret;
    spi_transaction_t t;
    memset(&t, 0, sizeof(t));                       // Zero out the transaction
    t.length = 8;                                   // Command is 8 bits
    t.tx_buffer = &cmd;                             // The data is the cmd itself
                                                    // t.user = (void *)0;                             // D/C needs to be set to 0
    ret = spi_device_polling_transmit(tft_spi, &t); // Transmit!
    if (ret != ESP_OK)
    {
        assert(ret == ESP_OK); // Should have had no issues.
    }
}
void tft_data_show(const uint8_t *data, int len)
{
    esp_err_t ret;
    spi_transaction_t t;
    if (len == 0)
        return;                                     // no need to send anything
    memset(&t, 0, sizeof(t));                       // Zero out the transaction
    t.length = len * 8;                             // Len is in bytes, transaction length is in bits.
    t.tx_buffer = data;                             // Data
    t.user = (void *)1;                             // D/C needs to be set to 1
    ret = spi_device_polling_transmit(tft_spi, &t); // Transmit!
    if (ret != ESP_OK)
    {
        assert(ret == ESP_OK); // Should have had no issues.
    }
}
static void IRAM_ATTR lcd_spi_pre_transfer_callback(spi_transaction_t *t)
{
    if ((int)t->user)
        GPIO.out1_w1ts.data = 8;
    else
        GPIO.out1_w1tc.data = 8;
}

void tft_spi_init(uint8_t eanble)
{
    esp_err_t ret;

    if (eanble)
    {
        lpbsp_gpio_eanble_num(PIN_NUM_MOSI);
        lpbsp_gpio_eanble_num(PIN_NUM_CLK);
        lpbsp_gpio_eanble_num(PIN_NUM_CS);
        lpbsp_gpio_eanble_num(PIN_NUM_DC);
        lpbsp_gpio_eanble_num(PIN_NUM_RST);

        esp_rom_gpio_pad_select_gpio(PIN_NUM_DC);
        gpio_set_direction(PIN_NUM_DC, GPIO_MODE_OUTPUT);

        spi_bus_config_t buscfg = {
            .miso_io_num = -1,
            .mosi_io_num = PIN_NUM_MOSI,
            .sclk_io_num = PIN_NUM_CLK,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = (TFT_VIEW_W * 2) * 140 + 32};
        spi_device_interface_config_t devcfg = {
            .clock_speed_hz = SPI_MASTER_FREQ_80M,   // Clock out at 10 MHz
            .mode = 0,                               // SPI mode 0
            .spics_io_num = PIN_NUM_CS,              // CS pin
            .queue_size = 6,                         // We want to be able to queue 7 transactions at a time
            .pre_cb = lcd_spi_pre_transfer_callback, // Specify pre-transfer callback to handle D/C line
            .flags = SPI_DEVICE_NO_DUMMY,
        };

        // Initialize the SPI bus
        ret = spi_bus_initialize(LCD_HOST, &buscfg, SPI_DMA_CH_AUTO);
        ESP_ERROR_CHECK(ret);
        // Attach the LCD to the SPI bus
        ret = spi_bus_add_device(LCD_HOST, &devcfg, &tft_spi);
        ESP_ERROR_CHECK(ret);

        BSP_LOG("tft spi [%d]\r\n", LCD_HOST);
    }
    else
    {
        BSP_LOG("tft free spi [%d]\r\n", LCD_HOST);
        extern int lpbsp_disp_axis_wait(void);
        lpbsp_disp_axis_wait();

        spi_bus_remove_device(tft_spi);
        spi_bus_free(LCD_HOST);

        lpbsp_gpio_disable_num(PIN_NUM_MOSI);
        lpbsp_gpio_disable_num(PIN_NUM_CLK);
        lpbsp_gpio_disable_num(PIN_NUM_CS);
        lpbsp_gpio_disable_num(PIN_NUM_DC);
    }
}

int tft_spiss_init(void)
{
    tft_buf = heap_caps_malloc(480 * 140, MALLOC_CAP_DMA);
    if (0 == tft_buf)
    {
        BSP_LOG("malloc error\r\n");
        return 1;
    }
    tft_buf32 = (uint32_t *)tft_buf;
    return 0;
}
static const uint8_t st7789_init_cmds[] = {
    0x11, 0x80, 1,
    0x3a, 0x1, 0x55,
    0x36, 0x1, 0,
    0x21, 0x80, 1,
    0x13, 0x80, 1,
    0x29, 0x80, 1,
    0, 0};

static spi_transaction_t trans[6];
void lpbsp_tft_data_init(void)
{
    int x = 0;
    memset(trans, 0, 6 * sizeof(spi_transaction_t));

    for (x = 0; x < 6; x++)
    {
        if ((x & 1) == 0)
        {
            trans[x].length = 8;
        }
        else
        {
            trans[x].length = 8 * 4;
            trans[x].user = (void *)1;
        }
        trans[x].flags = SPI_TRANS_USE_TXDATA;
    }
    trans[0].tx_data[0] = 0x2A;   // Column Address Set
    trans[2].tx_data[0] = 0x2B;   // Page address set
    trans[4].tx_data[0] = 0x2C;   // memory write
    trans[5].tx_buffer = tft_buf; // finally send the line data

    trans[5].flags = 0;
}

int lpbsp_tft_init(uint8_t eanble, uint8_t *config)
{
    int cmd = 0, temp;
    BSP_LOG("bsp_tft_init %d\r\n", eanble);
    if (1 == eanble)
    {
        lpbsp_tft_data_init();
        if (0 == config)
            config = st7789_init_cmds;
        tft_spiss_init();
        tft_spi_init(1);

        esp_rom_gpio_pad_select_gpio(PIN_NUM_RST);
        gpio_set_direction(PIN_NUM_RST, GPIO_MODE_OUTPUT);

#if 1
        gpio_set_level(PIN_NUM_RST, 0);
        lpbsp_delayms(150);
#endif
        gpio_set_level(PIN_NUM_RST, 1);

        while (1)
        {
            if (config[cmd])
            {
                lcd_cmd(config[cmd++]);
                temp = config[cmd++]; // num
                if (0x80 == temp)
                {
                    lpbsp_delayms(config[cmd++]);
                }
                else
                {
                    tft_data_show(&config[cmd], temp);
                    cmd += temp;
                }
            }
            else
            {
                BSP_LOG("init cmd %d\r\n", cmd);
                break;
            }
        }
        memset(tft_buf, 0xff, 2);
        lpbsp_disp_axis(0, 0, 1, 1, 2, tft_buf); // 醒来会等待spi，就得随便写一些数据
    }
    else // 关闭屏幕
    {
        tft_spi_init(0);

        gpio_set_level(PIN_NUM_RST, 0);
        lpbsp_delayms(150);
        gpio_set_level(PIN_NUM_RST, 1);

        lpbsp_gpio_disable_num(PIN_NUM_RST);
    }
    return 0;
}

int IRAM_ATTR lpbsp_disp_axis(int x1, int y1, int x2, int y2, int p_num, unsigned short *color_p)
{
    int x = 0;

    y1 += 20;
    y2 += 20;

    trans[1].tx_data[1] = x1;      // Start Col Low
    trans[1].tx_data[3] = x2;      // End Col Low
    trans[3].tx_data[0] = y1 >> 8; // Start page high
    trans[3].tx_data[1] = y1;      // start page low
    trans[3].tx_data[2] = y2 >> 8; // end page high
    trans[3].tx_data[3] = y2;      // end page low

    for (x = 0; x < 5; x++)
    {
        spi_device_queue_trans(tft_spi, &trans[x], portMAX_DELAY);
    }
    memcpy(tft_buf, color_p, p_num << 1);
    trans[5].length = p_num << 4; // Data length, in bits
    trans[5].tx_buffer = tft_buf; // finally send the line data
    spi_device_queue_trans(tft_spi, &trans[5], portMAX_DELAY);

    return 0;
}

int IRAM_ATTR lpbsp_disp_axis_wait(void)
{
    spi_transaction_t *rtrans;
    for (int x = 0; x < 6; x++)
    {
        spi_device_get_trans_result(tft_spi, &rtrans, portMAX_DELAY);
    }
    return 0;
}
/////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////
#include "esp_vfs_fat.h"
#include "driver/sdspi_host.h"
#include "sdmmc_cmd.h"

#define MOUNT_POINT "/C"
#define SPI_DMA_CHAN host.slot // on ESP32-S2, DMA channel must be the same as host id
static sdmmc_card_t *card = 0;
int lpbsp_fs_volume(char *leter, int *all, int *user)
{
    esp_err_t ret = 0;

    ret = esp_vfs_fat_info(leter, all, user);
    return ret;
}
int lpbsp_fs_format(char *leter)
{
    if ('C' == leter[1])
    {
        return esp_vfs_fat_sdcard_format(MOUNT_POINT, card);
    }
    return 0;
}

void sd_fat_init(uint8_t eanble)
{
    esp_err_t ret, i;
    static sdmmc_host_t host = SDSPI_HOST_DEFAULT();

    if (eanble)
    {
        gpio_config_t io_conf = {};
        io_conf.pull_down_en = 0;
        io_conf.pull_up_en = 1;
        io_conf.intr_type = GPIO_INTR_DISABLE;
        io_conf.mode = GPIO_MODE_OUTPUT;
        io_conf.pin_bit_mask = 1ULL << PIN_NUM_SD_POWER;

        gpio_config(&io_conf);
        gpio_set_drive_capability(PIN_NUM_SD_POWER, 3); // sd 大电流驱动
        gpio_set_level(PIN_NUM_SD_POWER, 1);

        esp_vfs_fat_sdmmc_mount_config_t mount_config = {
            .format_if_mount_failed = true,
            .max_files = 5,
            .allocation_unit_size = 4 * 1024};

        BSP_LOG("Using SPI peripheral %d\r\n", host.slot);

        spi_bus_config_t bus_cfg = {
            .mosi_io_num = PIN_NUM_SD_MOSI,
            .miso_io_num = PIN_NUM_SD_MISO,
            .sclk_io_num = PIN_NUM_SD_CLK,
            .quadwp_io_num = -1,
            .quadhd_io_num = -1,
            .max_transfer_sz = 4000,
        };
        ret = spi_bus_initialize(host.slot, &bus_cfg, SPI_DMA_CHAN);
        if (ret != ESP_OK)
        {
            BSP_LOG("Failed to initialize bus.\r\n");
            return;
        }

        // This initializes the slot without card detect (CD) and write protect (WP) signals.
        // Modify slot_config.gpio_cd and slot_config.gpio_wp if your board has these signals.
        sdspi_device_config_t slot_config = SDSPI_DEVICE_CONFIG_DEFAULT();
        slot_config.gpio_cs = PIN_NUM_SD_CS;
        slot_config.host_id = host.slot;

        for (i = 0; i < 5; i++)
        {
            ret = esp_vfs_fat_sdspi_mount(MOUNT_POINT, &host, &slot_config, &mount_config, &card);

            if (ret != ESP_OK)
            {
                if (ret == ESP_FAIL)
                {
                    BSP_LOG("Failed to mount filesystem. "
                            "If you want the card to be formatted, set the EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.\r\n");
                }
                else
                {
                    BSP_LOG("Failed to initialize the card (%s). "
                            "Make sure SD card lines have pull-up resistors in place.\r\n",
                            esp_err_to_name(ret));
                }
            }
            else
                break;

            lpbsp_delayms(50);
        }
        if (ret != ESP_OK)
            return;

        // Card has been initialized, print its properties
        // sdmmc_card_print_info(stdout, card);
    }
    else if (card)
    {
        // All done, unmount partition and disable SDMMC or SPI peripheral
        esp_vfs_fat_sdcard_unmount(MOUNT_POINT, card);
        BSP_LOG("Card unmounted");
        spi_bus_free(host.slot);

        gpio_set_level(PIN_NUM_SD_POWER, 0); // sd  关闭电源
    }
}
int lpbsp_fs_mount(char *leter)
{
    if ('C' == leter[1])
    {
        sd_fat_init(1);
    }
    return 0;
}
int lpbsp_fs_unmount(char *leter)
{
    if ('C' == leter[1])
    {
        sd_fat_init(0);
    }
    return 0;
}

////////////////////////////////////////////////////////////////////////
int lpbsp_sensor_nfc_id(uint8_t *out)
{
    uint8_t data[12];
    int ret;

    data[0] = 0x57;
    data[1] = 0;
    data[2] = 0x18;

    memset(out, 0, 8);
    ret = lpbsp_iic_read_bytes(0, data, 2, 8);
    if (ret == 0)
    {
        memcpy(out, data, 8);
    }
    return ret;
}

uint32_t lpbsp_sensor_magnetic_id(void)
{
    uint8_t data[4];
    int ret;
    data[0] = 0x2c; //  外设addr
    data[1] = 0x0;  // id  reg addr
    ret = lpbsp_iic_read(0, data, 1);
    if (ret == 0)
    {
        return data[0];
    }
    return 0;
}
uint32_t lpbsp_sensor_accelerometer_id(void)
{
    uint8_t data[4];
    int ret;
    data[0] = 0x12; //  外设addr
    data[1] = 0;    // id  reg addr
    ret = lpbsp_iic_read(0, data, 1);
    if (ret == 0)
    {
        return data[0];
    }
    return 0;
}
static int lpbsp_sensor_magnetic_write(uint8_t reg, uint8_t data)
{
    uint8_t tbuf[3];
    tbuf[0] = 0x1c;              //
    tbuf[1] = reg;               //
    tbuf[2] = data;              //
    lpbsp_iic_write(0, tbuf, 3); //
    return 0;
}

int lpbsp_sensor_magnetic_xyz_read(int32_t *x, int32_t *y, int32_t *z)
{
    uint8_t data[8];
    int ret = 0;

    data[0] = 0x1c;
    data[1] = 1;

    ret = lpbsp_iic_read(0, data, 6);
    if (0 == ret)
    {
        *x = (short)(data[0] | data[1] << 8);
        *y = (short)(data[2] | data[3] << 8);
        *z = (short)(data[4] | data[5] << 8);
    }
    return ret;
}

int32_t qma6100_writereg(uint8_t reg_add, uint8_t reg_dat)
{
    uint8_t tbuf[3];
    tbuf[0] = 0x12;
    tbuf[1] = reg_add;
    tbuf[2] = reg_dat;
    lpbsp_iic_write(0, tbuf, 3);
    return 0;
}

int32_t qma6100_readreg(uint8_t reg_add, uint8_t *bufs, int num)
{
    uint8_t tbuf[24];
    int ret = 0;
    tbuf[0] = 0x12;
    tbuf[1] = reg_add;
    ret = lpbsp_iic_read(0, tbuf, num);
    memcpy(bufs, tbuf, num);
    return ret;
}
uint32_t lpbsp_qma6100_read_stepcounter(void)
{
    uint8_t data[5] = {7};
    uint8_t ret;
    uint32_t step_num;

    ret = qma6100_readreg(0x07, data, 2);
    if (ret != 0)
    {
        return 0;
    }

    step_num = (uint32_t)(((uint32_t)data[2] << 16) | ((uint32_t)data[1] << 8) | data[0]);

    return step_num;
}
int32_t qma6100_soft_reset(void)
{
    uint8_t reg_0x11 = 0;
    uint8_t reg_0x33 = 0;
    uint32_t retry = 0;

    qma6100_writereg(0x36, 0xb6);
    lpbsp_delayms(5);
    qma6100_writereg(0x36, 0x00);
    lpbsp_delayms(100);

    retry = 0;
    while (reg_0x11 != 0x84)
    {
        qma6100_writereg(0x11, 0x84);
        lpbsp_delayms(5);
        qma6100_readreg(0x11, &reg_0x11, 1);
        if (retry++ > 100)
            break;
    }
    qma6100_writereg(0x33, 0x08);
    lpbsp_delayms(5);

    retry = 0;
    while (reg_0x33 != 0x05)
    {
        qma6100_readreg(0x33, &reg_0x33, 1);
        lpbsp_delayms(5);
        if (retry++ > 100)
            break;
    }

    return 0;
}
void lpbsp_qma6100_stepcounter_config(void)
{
    uint8_t reg_12 = 0x00;
    uint8_t reg_14 = 0x00;
    uint8_t reg_15 = 0x00;
    uint8_t reg_1e = 0x00;

    qma6100_writereg(0x13, 0x80);
    lpbsp_delayms(5);
    qma6100_writereg(0x13, 0x7f);

    reg_12 = 0x89;
    reg_14 = 0x0b;
    reg_15 = 0x0c;

    qma6100_writereg(0x12, reg_12);
    qma6100_writereg(0x14, reg_14);
    qma6100_writereg(0x15, reg_15);
    qma6100_readreg(0x1e, &reg_1e, 1);
    reg_1e &= 0x3f;
    qma6100_writereg(0x1e, (uint8_t)(reg_1e | (0x00 << 6)));
    qma6100_writereg(0x1f, (uint8_t)0xa0 | 0x08);
}
void qma6100_initialize(void)
{
    qma6100_soft_reset();

    qma6100_writereg(0x4a, 0x20);
    qma6100_writereg(0x50, 0x51);
    qma6100_writereg(0x56, 0x01);

    qma6100_writereg(0x0f, 0x02);

    qma6100_writereg(0x10, 0x05);

    qma6100_writereg(0x11, 0x84);

    qma6100_writereg(0x4A, 0x28);
    lpbsp_delayms(5);
    qma6100_writereg(0x20, 0x08);
    lpbsp_delayms(5);
    qma6100_writereg(0x5F, 0x80);
    lpbsp_delayms(5);
    qma6100_writereg(0x5F, 0x00);
    lpbsp_delayms(5);
    lpbsp_qma6100_stepcounter_config();
}

int lpbsp_sensor_init(void)
{
    uint8_t tbuf[32];
    int ret;

    lpbsp_sensor_nfc_id(tbuf);
    ret = lpbsp_sensor_magnetic_id();
    BSP_LOG("sensor_magnetic_id %x\r\n", ret);
    ret = lpbsp_sensor_accelerometer_id();
    BSP_LOG("lpbsp_sensor_accelerometer_id %x\r\n", ret);

    lpbsp_sensor_magnetic_write(0x29, 0x06);
    lpbsp_sensor_magnetic_write(0x0b, 0x08);
    lpbsp_sensor_magnetic_write(0x0a, 0xC3);

    if (0 == lpbsp_qma6100_read_stepcounter())
    {
        qma6100_initialize();
    }
    return 0;
}
////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////
int lpbsp_init_all(void)
{
    BSP_LOG("[%d]free %d,min=%d\r\n", lpbsp_tick_ms(), lpbsp_ram_free_get(), lpbsp_ram_min_get());
    lpbsp_init(1);
    lpbsp_bat_init(1);
    if (3600 > lpbsp_bat_get_mv())
    {
        lpbsp_bat_init(0);
        lpbsp_sleep_config_cpu(0);
    }
    lpbsp_init_2(1);
#if 0
    lpbsp_iic_init(0, 1);
    sd_fat_init(1);
    lpbsp_bl_set(50);
    {
        lpbsp_qsensor_init();
    }
    lpbsp_wifi_init(1);
    // lpbsp_wifi_connect("tes", "12345678");
#endif
    return 0;
}
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////
/////////////////////////////////////////////////////////////////////////////////////////////////////////

int lpbsp_inside_flash_erase(int addr)
{
    int ret = 0;

    ret = esp_flash_erase_region(0, addr, 4096);
    return ret;
}
int lpbsp_inside_flash_write(int addr, uint8_t *buf, uint32_t size)
{
    int ret = 0;

    ret = esp_flash_write(0, buf, addr, size);
    return ret;
}
int lpbsp_inside_flash_read(int addr, uint8_t *buf, uint32_t size)
{
    int ret = 0;

    ret = esp_flash_read(0, buf, addr, size);
    return ret;
}
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
static const esp_efuse_desc_t MODULE_SN[] = {
    {EFUSE_BLK3, 0, 64}, // Module sn,
};
const esp_efuse_desc_t *ESP_EFUSE_MODULE_SN[] = {
    &MODULE_SN[0], // Module sn
    NULL};

int lpbsp_sn_get(char *buf, int max_len)
{
    if (8 > max_len)
        return 0;
    ESP_ERROR_CHECK(esp_efuse_read_field_blob(ESP_EFUSE_MODULE_SN, buf, 64));
    return strlen(buf);
}
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////
///////////////////////////////////////////////////////////////////////////////////