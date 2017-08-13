/*************************************************
* ESP8266 SSID CONNECT FRAMEWORK
*
* AUGUST 13 2016
*
* ANKIT BHATNAGAR
* ANKIT.BHATNAGARINDIA@GMAIL.COM
************************************************/

#ifndef _ESP8266_SSID_FRAMEWORK_H_
#define _ESP8266_SSID_FRAMEWORK_H_

#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "os_type.h"
#include "user_interface.h"
#include "ESP8266_GPIO.h"

#define ESP8266_SSID_HARDCODED
#define ESP8266_SSID_SMARTCONFIG

#if defined(SSID_HARDCODED)
#elif defined(SSID_FLASH)
  #include "ESP8266_FLASH.h"
#elif defined(SSID_EEPROM)
  #include "ESP8266_EEPROM_AT24.h"
#elif defined(SSID_INTERNAL)
#elif defined(SSID_GPIO)
  #include "ESP8266_GPIO.h"
#elif defined(SSID_SMARTCONFIG)
  #include "ESP8266_SMARTCONFIG.h"
#elif defined(SSID_WEBCONFIG)
  #include "ESP8266_MDNS.h"
  #include "ESP8266_TCP_SERVER.h"
  #include "ESP8266_WEBCONFIG.h"
#endif

//CUSTOM VARIABLE STRUCTURES/////////////////////////////
typedef enum
{
    ESP8266_SSID_FRAMEWORK_SSID_INPUT_HARDCODED = 0,
    ESP8266_SSID_FRAMEWORK_SSID_INPUT_FLASH,
    ESP8266_SSID_FRAMEWORK_SSID_INPUT_EEPROM,
    ESP8266_SSID_FRAMEWORK_SSID_INPUT_INTERNAL,
    ESP8266_SSID_FRAMEWORK_SSID_INPUT_GPIO
}ESP8266_SSID_FRAMEWORK_SSID_INPUT_MODE;

typedef struct
{
    char* ssid_name;
    uint8_t ssid_name_len;
    char* ssid_pwd;
    uint8_t ssid_pwd_len;
}ESP8266_SSID_FRAMEWORK_HARDCODED_SSID_DETAILS;

typedef struct
{
    uint32_t ssid_name_addr;
    uint8_t ssid_name_len;
    uint32_t ssid_pwdaddr;
    uint8_t ssid_pwd_len;
}ESP8266_SSID_FRAMEWORK_FLASH_SSID_DETAILS;

typedef struct
{
    uint32_t ssid_name_addr;
    uint8_t ssid_name_len;
    uint32_t ssid_pwdaddr;
    uint8_t ssid_pwd_len;
}ESP8266_SSID_FRAMEWORK_EEPROM_SSID_DETAILS;

typedef enum
{
    ESP8266_SSID_FRAMEWORK_CONFIG_SMARTCONFIG = 0,
    ESP8266_SSID_FRAMEWORK_CONFIG_WEBCONFIG
}ESP8266_SSID_FRAMEWORK_CONFIG_MODE;
//END CUSTOM VARIABLE STRUCTURES/////////////////////////

//FUNCTION PROTOTYPES/////////////////////////////////////////////
//CONFIGURATION FUNCTIONS
void ICACHE_FLASH_ATTR ESP8266_SSID_FRAMEWORK_SetDebug(uint8_t debug);
void ICACHE_FLASH_ATTR ESP8266_SSID_FRAMEWORK_SetParameters(ESP8266_SSID_FRAMEWORK_SSID_INPUT_MODE input_mode,
                                                            ESP8266_SSID_FRAMEWORK_CONFIG_MODE config_mode,
                                                            void* user_data,
                                                            uint8_t retry_count,
                                                            uint32_t retry_delay_ms,
                                                            uint8_t gpio_led_pin);
void ICACHE_FLASH_ATTR ESP8266_SSID_FRAMEWORK_SetCbFunctions(void (*wifi_connected_cb)(void));

//OPERATION FUNCTIONS
void ICACHE_FLASH_ATTR ESP8266_SSID_FRAMEWORK_Initialize(void);

//INTERNAL FUNCTIONS
void ICACHE_FLASH_ATTR _esp8266_ssid_framework_led_toggle_cb(void* pArg);
void ICACHE_FLASH_ATTR _esp8266_ssid_framework_wifi_connect_timer_cb(void* pArg);
void ICACHE_FLASH_ATTR _esp8266_ssid_framework_wifi_event_handler_cb(System_Event_t* event);
void ICACHE_FLASH_ATTR _esp8266_ssid_framework_wifi_start_ssid_configuration(void);
void ICACHE_FLASH_ATTR _esp8266_ssid_framework_wifi_start_connection_process(void);
#endif
