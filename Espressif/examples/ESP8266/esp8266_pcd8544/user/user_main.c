/*
 *  PCD8544 LCD driver for esp8266 (Nokia 5110 & 3110 display)
 *  https://github.com/eadf/esp8266_pcd8544
 *
 *  The interface requires 5 available GPIO outputs so an ESP-01 will not work.
 *
 *  This is how the code is hooked up:
 *
 *  PCD8544| ESP8266
 *  -------|------------------
 *  RST Pin 1 | GPIO4
 *  CE  Pin 2 | GPIO5
 *  DC  Pin 3 | GPIO12
 *  Din Pin 4 | GPIO13
 *  Clk Pin 5 | GPIO14
 *
 *  Some ESP-12 have GPIO4 & GPIO5 reversed.
 *
 *  I don't know if it is required but i put 1KΩ resistors on each gpio pin, and it does not seem to cause any problems.
 *
 */

#include "ets_sys.h"
#include "osapi.h"
#include "c_types.h"
#include "gpio.h"
#include "os_type.h"
#include "user_config.h"
#include "user_interface.h"
#include "driver/uart.h"
#include "driver/pcd8544.h"

static uint8_t openhardware_logo[] = {
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xc0,0xe0,0xe0,0xe0,0xc0,0x80,0x80,0x00,0x00,0x00,0x00,0x00,0x80,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x80,0x80,0x00,0x00,0x00,0x00,0x00,0x80,0xc0,0xe0,0xe0,0xe0,0xe0,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0e,0x1f,0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x3f,0x1e,0x04,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xc0,0xc0,0xc0,0xc0,0xe0,0xe0,0xe0,0xf0,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x3f,0x1f,0x1f,0x0f,0x0f,0x0f,0x0f,0x0f,0x0f,0x1f,0x3f,0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xfc,0xf0,0xe0,0xe0,0xe0,0xc0,0xc0,0xc0,0xc0,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x7f,0x7f,0x7f,0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xe1,0x80,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0xc0,0xe1,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x7f,0x7f,0x7f,0x7f,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x83,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0x7f,0x1e,0x00,0x00,0x00,0x00,0x00,0x06,0x1e,0x7f,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xff,0xcf,0x83,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x0e,0x1f,0x3f,0x7f,0xff,0xff,0xff,0x7f,0x7f,0x3f,0x3f,0x1f,0x3f,0x3f,0x3f,0x0f,0x01,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x03,0x0f,0x3f,0x3f,0x3f,0x1f,0x3f,0x3f,0x7f,0xff,0xff,0xff,0xff,0x3f,0x3f,0x1e,0x0c,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,0x00,
};

#define user_procLcdUpdatePeriod      500
#define user_procTaskPrio        0
#define user_procTaskQueueLen    1

static os_timer_t loop_timer;
os_event_t user_procTaskQueue[user_procTaskQueueLen];
static PCD8544_Settings pcd8544_settings;

void user_init(void);
static void loop(os_event_t *events);
static void nop_procTask(os_event_t *events);

//Main code function
static void ICACHE_FLASH_ATTR loop(os_event_t *events) {
  static uint32_t loopIterations = 0;
  loopIterations+=1;
  if (loopIterations < 3) {
    PCD8544_lcdImage(openhardware_logo);
  } else if (loopIterations == 3){
    PCD8544_lcdClear();
  } else {

    // Draw a Box
    PCD8544_drawLine();
    int a=0;
    PCD8544_gotoXY(17,1);
    // Put text in Box
    PCD8544_lcdPrint("ESP8266");
    PCD8544_gotoXY(24,2);
    if (loopIterations & 1){
      PCD8544_lcdCharacter('H');
      PCD8544_lcdCharacter('E');
      PCD8544_lcdCharacter('L');
      PCD8544_lcdCharacter('L');
      PCD8544_lcdCharacter('O');
      PCD8544_lcdCharacter(' ');
      PCD8544_lcdCharacter('=');
      // Draw + at this position
      PCD8544_gotoXY(10,2);
      PCD8544_lcdCharacter('=');
    } else {
      PCD8544_gotoXY(24,2);
      PCD8544_lcdCharacter('h');
      PCD8544_lcdCharacter('e');
      PCD8544_lcdCharacter('l');
      PCD8544_lcdCharacter('l');
      PCD8544_lcdCharacter('o');
      PCD8544_lcdCharacter(' ');
      PCD8544_lcdCharacter('-');
      // Draw - at this position
      PCD8544_gotoXY(10,2);
      PCD8544_lcdCharacter('-');
    }
    uint8_t contrast = ((loopIterations << 2)+25) & 0x3f; // +25 so that we start in the visible range
    PCD8544_setContrast(contrast);
    PCD8544_gotoXY(2,3);
    PCD8544_lcdPrint(" contrast:");
    char buf[] = "         ";
    os_sprintf(buf,"%d   ", contrast);
    PCD8544_gotoXY(32,4);
    PCD8544_lcdPrint(buf);
    os_printf("Updating display. Contrast = %d\n", contrast);
  }
}

/**
 * Setup program. When user_init runs the debug printouts will not always
 * show on the serial console. So i run the inits in here, 2 seconds later.
 */
static void ICACHE_FLASH_ATTR setup(void) {
  pcd8544_settings.lcdVop = 0xB1;
  pcd8544_settings.tempCoeff = 0x04;
  pcd8544_settings.biasMode = 0x14;
  pcd8544_settings.inverse = false;

  pcd8544_settings.resetPin = -1; // 4;
  pcd8544_settings.scePin = -1; //5;

  pcd8544_settings.dcPin = 12;
  pcd8544_settings.sdinPin = 13;
  pcd8544_settings.sclkPin = 14;

  PCD8544_init(&pcd8544_settings);
  os_printf("pcd8544 lcd initiated\n");

  // Start loop timer
  os_timer_disarm(&loop_timer);
  os_timer_setfn(&loop_timer, (os_timer_func_t *) loop, NULL);
  os_timer_arm(&loop_timer, user_procLcdUpdatePeriod, true);
}

//Do nothing function
static void ICACHE_FLASH_ATTR nop_procTask(os_event_t *events) {
  os_delay_us(10);
}

/******************************************************************************
 * FunctionName : user_rf_cal_sector_set
 * Description  : SDK just reversed 4 sectors, used for rf init data and paramters.
 *                We add this function to force users to set rf cal sector, since
 *                we don't know which sector is free in user's application.
 *                sector map for last several sectors : ABCCC
 *                A : rf cal
 *                B : rf init data
 *                C : sdk parameters
 * Parameters   : none
 * Returns      : rf cal sector
*******************************************************************************/
uint32 ICACHE_FLASH_ATTR
user_rf_cal_sector_set(void)
{
    enum flash_size_map size_map = system_get_flash_size_map();
    uint32 rf_cal_sec = 0;

    switch (size_map) {
        case FLASH_SIZE_4M_MAP_256_256:
            rf_cal_sec = 128 - 5;
            break;

        case FLASH_SIZE_8M_MAP_512_512:
            rf_cal_sec = 256 - 5;
            break;

        case FLASH_SIZE_16M_MAP_512_512:
        case FLASH_SIZE_16M_MAP_1024_1024:
            rf_cal_sec = 512 - 5;
            break;

        case FLASH_SIZE_32M_MAP_512_512:
        case FLASH_SIZE_32M_MAP_1024_1024:
            rf_cal_sec = 1024 - 5;
            break;

        case FLASH_SIZE_64M_MAP_1024_1024:
            rf_cal_sec = 2048 - 5;
            break;
        case FLASH_SIZE_128M_MAP_1024_1024:
            rf_cal_sec = 4096 - 5;
            break;
        default:
            rf_cal_sec = 0;
            break;
    }

    return rf_cal_sec;
}

void ICACHE_FLASH_ATTR user_rf_pre_init(void)
{
}

void ICACHE_FLASH_ATTR user_init(void)
{
  // Make uart0 work with just the TX pin. Baud:115200,n,8,1
  // The RX pin is now free for GPIO use.
  UARTInit(BIT_RATE_115200);
  os_delay_us(1000);

  os_printf("\r\nSDK version:%s\r\n", system_get_sdk_version());

  // turn off WiFi for this console only demo
  wifi_station_set_auto_connect(false);
  wifi_station_disconnect();

  os_timer_disarm(&loop_timer);

  // Start setup timer
  os_timer_setfn(&loop_timer, (os_timer_func_t *) setup, NULL);
  os_timer_arm(&loop_timer, user_procLcdUpdatePeriod*2, false);

  //Start no-operation os task
  system_os_task(nop_procTask, user_procTaskPrio, user_procTaskQueue, user_procTaskQueueLen);
  system_os_post(user_procTaskPrio, 0, 0);
}
