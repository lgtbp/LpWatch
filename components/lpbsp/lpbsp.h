
#ifndef __LP_BSP_H
#define __LP_BSP_H

#ifdef __cplusplus
extern "C"
{
#endif

#include "stdint.h"

    typedef void (*task_function_t)(void *);
    typedef void (*time_function_t)(void *);

#define E_WIFI_CONNECT 1
#define TFT_VIEW_W 240 // 硬件屏幕宽度
#define TFT_VIEW_H 280 // 硬件屏幕高度

    typedef struct __lpbsp_wifi_ap_info_t
    {
        uint8_t wifi_auth_mode; /**< authenticate mode */
        int8_t rssi;            /**< signal strength of AP */
        uint8_t bssid[6];       /**< MAC address of AP */
        uint8_t ssid[32];       /**< SSID of AP */
    } lpbsp_wifi_ap_info_t;

    enum SLEEP_MODE_E
    {
        SLEEP_MODE_DEEP_LOW_POEWR = 0, // 低电 只能充电才能激活 屏不亮
        SLEEP_MODE_DEEP = 1,           // 充电 和 tp 屏不亮 不自动醒来
        SLEEP_MODE_DEEP_BL = 2,        // 充电 和 tp 屏亮 60内醒来
        SLEEP_MODE_LIGHT = 3,          // 充电 和 tp 灭屏
        SLEEP_MODE_LIGHT_BL = 4,       // 充电 和 tp 屏亮 60内醒来
    };

    enum GPIO_IQR_E
    {
        GPIO_IQR_TP = 1,
        GPIO_IQR_IO = 2,
    };
///////////////////////////////////////
#define LCD_HOST SPI2_HOST

#define PIN_NUM_MOSI 21
#define PIN_NUM_CLK 33
#define PIN_NUM_CS 34
#define PIN_NUM_DC 35

#define PIN_NUM_RST 8

#define PIN_NUM_KEY_UP 12
#define PIN_NUM_KEY_ENTER 13
#define PIN_NUM_KEY_DOWN 14

#define PIN_NUM_MOTO 37

#define PIN_NUM_CHARGE 16
#define PIN_NUM_BCKL 15

#define PIN_NUM_BCKL2 11

#define I2C_MASTER_SCL_IO 10
#define I2C_MASTER_SDA_IO 9

#define PIN_NUM_UART_TX 6 // gps
#define PIN_NUM_UART_RX 5

#define PIN_NUM_SD_CS 39 // sd card
#define PIN_NUM_SD_MOSI 41
#define PIN_NUM_SD_CLK 40
#define PIN_NUM_SD_MISO 42

#define QMC7983_ADDR 0x2C
#define QMC7981_ADDR 0x12

#define QMC7981_ID_REG_ADDR 0 // 0xEX
#define QMC7983_ID_REG_ADDR 0xc

#define PIN_NUM_SD_POWER 2
#define PIN_NUM_GPS_POWER 4

#define PIN_NUM_TP_INT 7

#define PIN_NUM_NFC_INT 3
#define PIN_NUM_SENSOR_INT 18

#define _I2C_NUMBER(num) I2C_NUM_##num
#define I2C_NUMBER(num) _I2C_NUMBER(num)

#define I2C_MASTER_NUM I2C_NUMBER(0) /*!< I2C port number for master dev */
#define I2C_MASTER_FREQ_HZ 100000    /*!< I2C master clock frequency */
#define I2C_MASTER_TX_BUF_DISABLE 0  /*!< I2C master doesn't need buffer */
#define I2C_MASTER_RX_BUF_DISABLE 0  /*!< I2C master doesn't need buffer */
    /////////////////////////////////////////
    uint8_t *lpbsp_rtc_ram_get(int *ram_size);                                 // rtc 内存
    uint32_t lpbsp_error(void);                                                //
    int lpbsp_init(uint8_t eanble);                                            // bsp初始化
    int lpbsp_tft_init(uint8_t eanble, uint8_t *config);                       // tft初始化
    int lpbsp_restart(void);                                                   // 系统复位
    int lpbsp_moto_set(int value_0_to_100);                                    // 马达
    int lpbsp_bl_set(int value_0_to_100);                                      // 背光
    int lpbsp_bat_init(uint8_t eanble);                                        // 电池初始化
    int lpbsp_bat_get_mv(void);                                                // 电池电压 mv
    int lpbsp_charge_get(void);                                                // 充电查询
    int lpbsp_keys_get(void);                                                  // 按键 查询
    int lpbsp_io_set(int io, int value);                                       // io设置
    int lpbsp_io_get(int io);                                                  // io读取
    int lpbsp_delayms(int ms);                                                 // 延时
    int lpbsp_sleep_config(uint32_t sleep_mode, int ms);                       //
    int lpbsp_sleep_wakeup_cause(void);                                        // 醒来原因
    int lpbsp_rtc_set(unsigned int sec);                                       // 时间设置
    unsigned int lpbsp_rtc_get(void);                                          // 时间读取
    int lpbsp_ram_min_get(void);                                               // 内存剩余最低值
    int lpbsp_ram_free_get(void);                                              // 内存剩余
    void *lpbsp_malloc(uint32_t size);                                         // 内存申请
    void lpbsp_free(void *ram);                                                // 内存释放
    void *lpbsp_realloc(void *ram, uint32_t size);                             // 内存重新申请
    void *lpbsp_calloc(uint32_t n, uint32_t size);                             //
    int lpbsp_stack_mini_get(void);                                            //
    void lpbsp_wdt_add(void);                                                  //
    void lpbsp_wdt_feed(void);                                                 //
    void lpbsp_wdt_del(void);                                                  //
    void *lpbsp_queue_create(int uxQueueLength, int uxItemSize);               //
    int lpbsp_queue_send(void *xQueue, void *pvItemToQueue, int wait_ms);      //
    int lpbsp_queue_receive(void *xQueue, void *pvItemToQueue, int wait_ms);   //
    int lpbsp_time_create(time_function_t arg);                                //
    int lpbsp_uart_init(int num, int type);                                    //
    int lpbsp_uart_write(int num, uint8_t *data, int size);                    //
    int lpbsp_uart_read(int num, uint8_t *data, int size);                     //
    int lpbsp_iic_init(int num, int type);                                     //
    int lpbsp_iic_write(int num, uint8_t *data, int size);                     // data[0]=addr,data[1]=reg,data[2]=data
    int lpbsp_iic_read(int num, uint8_t *data, int size);                      // data[0]=addr,data[1]=reg0
    int lpbsp_iic_read_bytes(int num, uint8_t *data, int addr_size, int size); // data[0]=addr,data[1]=reg0,data[2]=reg1
    int lpbsp_disp_axis(int x1, int y1, int x2, int y2, int p, uint16_t *c);   //
    int lpbsp_disp_axis_wait(void);                                            // 等待显示完成
    int lpbsp_task_create(char *name,                                          // task name
                          void *arg,                                           // task arg
                          task_function_t fun,                                 // task function
                          uint32_t stack_depth,                                // 栈大小
                          char lev);                                           // 新建任务 优先级
    int lpbsp_task_stop(void *p);                                              //
    int lpbsp_task_start(void *p);                                             //
    int lpbsp_task_delete(void *p);                                            //
    uint32_t lpbsp_wifi_event(uint32_t event);                                 // wifi 状态
    int lpbsp_wifi_scan_ap_info(lpbsp_wifi_ap_info_t *ap, int max_size);       // ap 信息
    int lpbsp_wifi_init(int mode);                                             // wifi init
    int lpbsp_wifi_scan(void);                                                 // wifi scan
    int lpbsp_wifi_connect(char *ssid, char *password);                        // wifi connect
    int lpbsp_wifi_stop(void);                                                 //
    int lpbsp_wifi_scan_ap_num(void);                                          // ap nnum
    int lpbsp_wifi_addr_ip(char *ip);                                          // wifi ip
    int lpbsp_iqr_event(uint32_t *io);                                         // iqr事件
    void lpbsp_iqr_event_clear(void);                                          //
    int lpbsp_socket_connect_url(char *rul, uint32_t port);                    // 链接url,返回socket num
    int lpbsp_socket_ip4_tcp(void);                                            //
    int lpbsp_socket_connect_ip(int sock, char *ip, int port);                 //
    int lpbsp_socket_close(int fd);                                            //
    int lpbsp_socket_write(int fd, const uint8_t *buf, int nbytes);            //
    int lpbsp_socket_read(int fd, uint8_t *buf, int nbytes);                   //
    unsigned int lpbsp_tick_ms(void);                                          // 系统tick ms
    int lpbsp_inside_flash_erase(int addr);                                    // 4k
    int lpbsp_inside_flash_write(int addr, uint8_t *buf, uint32_t size);       //
    int lpbsp_inside_flash_read(int addr, uint8_t *buf, uint32_t size);        //
    int lpbsp_sn_get(char *buf, int max_len);                                  // 获取sn
    int lpbsp_sn_set(char *buf, int len);                                      // 写sn
    /////////////////////////////////////////////////////////////////////////////////
    int lpbsp_sensor_init(void);
    int lpbsp_init_all(void);
    void lpbsp_qma6100_stepcounter_config(void);
    int lpbsp_sensor_magnetic_xyz_read(int32_t *x, int32_t *y, int32_t *z);
    uint32_t lpbsp_sensor_magnetic_id(void);
    uint32_t lpbsp_sensor_accelerometer_id(void);
    int lpbsp_sensor_nfc_id(uint8_t *out);
    int lpbsp_fs_volume(char *leter, int *all, int *user);
    int lpbsp_fs_mount(char *leter);
    int lpbsp_fs_unmount(char *leter);
    int lpbsp_fs_format(char *leter);

#ifdef __cplusplus
} /*extern "C"*/
#endif

#endif
