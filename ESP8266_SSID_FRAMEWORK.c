/*************************************************
* ESP8266 SSID CONNECT FRAMEWORK
*
* AUGUST 13 2016
*
* ANKIT BHATNAGAR
* ANKIT.BHATNAGARINDIA@GMAIL.COM
************************************************/

#include "ESP8266_SSID_FRAMEWORK.h"

//LOCAL LIBRARY VARIABLES////////////////////////////////
//DEBUG RELATED
static uint8_t _esp8266_ssid_framework_debug;
//END LOCAL LIBRARY VARIABLES/////////////////////////////

void ICACHE_FLASH_ATTR ESP8266_SSID_FRAMEWORK_SetDebug(uint8_t debug_on)
{
    //SET DEBUG PRINTF ON(1) OR OFF(0)

    _esp8266_ssid_framework_debug = debug_on;
}

void ICACHE_FLASH_ATTR ESP8266_SSID_FRAMEWORK_SetParameters(ESP8266_SSID_FRAMEWORK_SSID_INPUT_MODE input_mode, ESP8266_SSID_FRAMEWORK_CONFIG_MODE config_mode, void* user_data)
{
    //SET THE SSID FRAMEWORK SSID INPUT MODE AND SSID CONFIGURATION MODE
    //VOID POINTER user_data INTERPRETED / CASTED AS PER MODE

    switch(input_mode)
    {
        case ESP8266_SSID_FRAMEWORK_SSID_INPUT_HARDCODED:
            if(_esp8266_ssid_framework_debug)
                os_printf("ESP8266 : SSID FRAMEWORK : INPUT MODE = HARDCODED");
            break;

        case ESP8266_SSID_FRAMEWORK_SSID_INPUT_FLASH:
            if(_esp8266_ssid_framework_debug)
                os_printf("ESP8266 : SSID FRAMEWORK : INPUT MODE = FLASH");
            break;

        case ESP8266_SSID_FRAMEWORK_SSID_INPUT_EEPROM:
            if(_esp8266_ssid_framework_debug)
                os_printf("ESP8266 : SSID FRAMEWORK : INPUT MODE = EEPROM");
            break;

        case ESP8266_SSID_FRAMEWORK_SSID_INPUT_INTERNAL:
            if(_esp8266_ssid_framework_debug)
                os_printf("ESP8266 : SSID FRAMEWORK : INPUT MODE = INTERNAL");
            break;

        case ESP8266_SSID_FRAMEWORK_SSID_INPUT_GPIO:
            if(_esp8266_ssid_framework_debug)
                os_printf("ESP8266 : SSID FRAMEWORK : INPUT MODE = GPIO");
            break;

        default:
            break;
    }

    switch(config_mode)
    {
        case ESP8266_SSID_FRAMEWORK_CONFIG_SMARTCONFIG:
            if(_esp8266_ssid_framework_debug)
                os_printf("ESP8266 : SSID FRAMEWORK : CONFIG MODE = SMARTCONFIG");
            break;

        case ESP8266_SSID_FRAMEWORK_CONFIG_WEBCONFIG:
            if(_esp8266_ssid_framework_debug)
                os_printf("ESP8266 : SSID FRAMEWORK : CONFIG MODE = WEBCONFIG");
            break;

        default:
            break;
    }
}
