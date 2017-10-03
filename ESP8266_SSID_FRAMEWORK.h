/*************************************************
* ESP8266 SSID CONNECT FRAMEWORK
*
* SOFT AP DETAILS
* SSID = ESP8266 | PASSWORD = 123456789
*
* SET THE WIFI RECONNECT INTERVAL TO ATLEAST
* 4000ms (4 s), SO AS TO GIVE ENOUGH TIME TO
* ESP8266 TO CONNECT TO WIFI
*
*  INPUT_MODE        TRIGGER                   IF NOT ABLE TO CONNECT TO WIFI
*  ----------        --------------            -----------------------------------------------
*
*  GPIO              GPIO PIN HIGH             - START SSID FRAMEWORK EITHER IN SMARTCONFIG
*                                                OR IN TCP WEBSERVER
*                                              - WIFI CONNECT SUCCESSFULL
*                                              - AUTO CONNECT ON RESTART FROM INTERNAL SSID CACHE
*
*  HARDCODED         NOT ABLE TO CONNECT       - START SSID FRAMEWORK EITHER IN SMARTCONFIG
*                    TO HARDCODED SSID           OR IN TCP WEBSERVER
*                                              - WIFI CONNECT SUCCESSFULL
*                                              - ON RESTART CHECK FOR VALID SSID CACHE. IF FOUND
*                                                CONNECT USING THAT ELSE USE HARDCODED ONE
*
*  INTERNAL          NOT ABLE TO CONNECT       - START SSID FRAMEWORK EITHER IN SMARTCONFIG
*                    TO INTERNALLY CACHED        OR IN TCP WEBSERVER
*                    SSID                      - WIFI CONNECT SUCCESSFULL
*                                              - AUTO CONNECT ON RESTART FROM INTERNAL SSID CACHE
*
*  FLASH             NOT ABLE TO CONNECT       - START SSID FRAMEWORK EITHER IN SMARTCONFIG
*  EEPROM            TO SSID READ FROM           OR IN TCP WEBSERVER
*                    FLASH/EEPROM AT GIVEN     - WIFI CONNECT SUCCESSFULL
*                    ADDRESS                   - SAVE WIFI CREDENTIAL TO FLASH/EEPROM
*                                              - ON RESTART, AGAIN READ FLASH/EEPROM ADDRESS
*                                                AND CONNECT TO READ SSID
*
*
*
* REFERENCES
* -----------
*   (1) ONLINE HTML EDITOR
*       http://bestonlinehtmleditor.com/
*		(2) BOOTSTRAP HTML GUI EDITOR
*				http://http://pingendo.com/
*				https://diyprojects.io/bootstrap-create-beautiful-web-interface-projects-esp8266/#.WdMcBmt95hH
*
* AUGUST 13 2017
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
#include "string.h"
#include "ESP8266_GPIO.h"
#include "ESP8266_SYSINFO.h"

#define ESP8266_SSID_HARDCODED
#define ESP8266_SSID_WEBCONFIG

#define ESP8266_SSID_FRAMEWORK_WEBCONFIG_PATH_STRING		"/config"
#define ESP8266_SSID_FRAMEWORK_SSID_NAME_LEN            32
#define ESP8266_SSID_FRAMEWORK_SSID_PSWD_LEN            64
#define ESP8266_SSID_FRAMEWORK_CUSTOM_FIELD_MAX_COUNT   5
#define ESP8266_SSID_FRAMEWORK_MAX_CUSTOM_FIELD_MAX_LEN 32

#if defined(ESP8266_SSID_FLASH)
  #include "ESP8266_FLASH.h"
#elif defined(ESP8266_SSID_EEPROM)
  #include "ESP8266_EEPROM_AT24.h"
#elif defined(ESP8266_SSID_SMARTCONFIG)
  #include "ESP8266_SMARTCONFIG.h"
#elif defined(ESP8266_SSID_WEBCONFIG)
  #include "ESP8266_MDNS.h"
  #include "ESP8266_TCP_SERVER.h"
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
    char* ssid_pwd;
}ESP8266_SSID_FRAMEWORK_HARDCODED_SSID_DETAILS;

typedef struct
{
    uint32_t ssid_name_addr;
    uint32_t ssid_pwdaddr;
}ESP8266_SSID_FRAMEWORK_FLASH_SSID_DETAILS;

typedef struct
{
    uint32_t ssid_name_addr;
    uint32_t ssid_pwdaddr;
}ESP8266_SSID_FRAMEWORK_EEPROM_SSID_DETAILS;

typedef enum
{
    ESP8266_SSID_FRAMEWORK_CONFIG_SMARTCONFIG = 0,
    ESP8266_SSID_FRAMEWORK_CONFIG_WEBCONFIG
}ESP8266_SSID_FRAMEWORK_CONFIG_MODE;

typedef struct
{
    char* custom_field_name;
    char* custom_field_label;
} ESP8266_SSID_FRAMEWORK_CONFIG_USER_FIELD;

typedef struct
{
    ESP8266_SSID_FRAMEWORK_CONFIG_USER_FIELD* custom_fields;
    uint8_t custom_fields_count;
} ESP8266_SSID_FRAMEWORK_CONFIG_USER_FIELD_GROUP;
//END CUSTOM VARIABLE STRUCTURES/////////////////////////

//FUNCTION PROTOTYPES/////////////////////////////////////////////
//CONFIGURATION FUNCTIONS
void ICACHE_FLASH_ATTR ESP8266_SSID_FRAMEWORK_SetDebug(uint8_t debug);
void ICACHE_FLASH_ATTR ESP8266_SSID_FRAMEWORK_SetParameters(ESP8266_SSID_FRAMEWORK_SSID_INPUT_MODE input_mode,
                                                            ESP8266_SSID_FRAMEWORK_CONFIG_MODE config_mode,
                                                            void* user_data,
                                                            ESP8266_SSID_FRAMEWORK_CONFIG_USER_FIELD_GROUP* user_field_data,
                                                            uint8_t retry_count,
                                                            uint32_t retry_delay_ms,
                                                            uint8_t gpio_led_pin,
																														char* project_name);

void ICACHE_FLASH_ATTR ESP8266_SSID_FRAMEWORK_SetCbFunctions(void (*wifi_connected_cb)(char**));

//OPERATION FUNCTIONS
void ICACHE_FLASH_ATTR ESP8266_SSID_FRAMEWORK_Initialize(void);

//INTERNAL FUNCTIONS
void ICACHE_FLASH_ATTR _esp8266_ssid_framework_led_toggle_cb(void* pArg);
void ICACHE_FLASH_ATTR _esp8266_ssid_framework_wifi_connect_timer_cb(void* pArg);
void ICACHE_FLASH_ATTR _esp8266_ssid_framework_wifi_event_handler_cb(System_Event_t* event);
void ICACHE_FLASH_ATTR _esp8266_ssid_framework_wifi_start_ssid_configuration(void);
void ICACHE_FLASH_ATTR _esp8266_ssid_framework_wifi_start_connection_process(struct station_config* sconfig);
void ICACHE_FLASH_ATTR _esp8266_ssid_framework_wifi_start_softap(void);
void ICACHE_FLASH_ATTR _esp8266_ssid_framework_tcp_server_path_config_cb(void);
void ICACHE_FLASH_ATTR _esp8266_ssid_framework_tcp_server_post_data_cb(char* data, uint16_t len, uint8_t post_flag);
#endif
