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

//OPERATION RELATED
static ESP8266_SSID_FRAMEWORK_SSID_INPUT_MODE _input_mode;
static ESP8266_SSID_FRAMEWORK_CONFIG_MODE _config_mode;
static volatile uint8_t _esp8266_ssid_framework_wifi_connected = 0;
static uint8_t _led_gpio_pin;

//TIMER RELATED
os_timer_t _status_led_timer;
os_timer_t _wifi_connect_timer;

//SSID RELATED
static uint8_t _ssid_connect_retries;
static uint8_t _ssid_connect_retry_count;
static uint32_t _ssid_connect_retry_delay_ms;
static ESP8266_SSID_FRAMEWORK_HARDCODED_SSID_DETAILS _ssid_hardcoded_name_pwd;
static ESP8266_SSID_FRAMEWORK_FLASH_SSID_DETAILS _ssid_flash_name_pwd;
static ESP8266_SSID_FRAMEWORK_EEPROM_SSID_DETAILS _ssid_eeprom_name_pwd;
static uint8_t _ssid_gpio_trigger_pin;

//CB FUNCTIONS
static void (*_esp8266_ssid_framework_wifi_connected_user_cb)(void);

//END LOCAL LIBRARY VARIABLES/////////////////////////////

void ICACHE_FLASH_ATTR ESP8266_SSID_FRAMEWORK_SetDebug(uint8_t debug_on)
{
    //SET DEBUG PRINTF ON(1) OR OFF(0)

    _esp8266_ssid_framework_debug = debug_on;
}

void ICACHE_FLASH_ATTR ESP8266_SSID_FRAMEWORK_SetParameters(ESP8266_SSID_FRAMEWORK_SSID_INPUT_MODE input_mode,
                                                            ESP8266_SSID_FRAMEWORK_CONFIG_MODE config_mode,
                                                            void* user_data,
                                                            uint8_t retry_count,
                                                            uint32_t retry_delay_ms,
                                                            uint8_t gpio_led_pin)
{
    //SET THE SSID FRAMEWORK SSID INPUT MODE AND SSID CONFIGURATION MODE
    //VOID POINTER user_data INTERPRETED / CASTED AS PER MODE

    _input_mode = input_mode;
    _config_mode = config_mode;

    _led_gpio_pin = gpio_led_pin;

    _ssid_connect_retries = retry_count;
    _ssid_connect_retry_delay_ms = retry_delay_ms;

    switch(input_mode)
    {
        case ESP8266_SSID_FRAMEWORK_SSID_INPUT_HARDCODED:
            if(_esp8266_ssid_framework_debug)
                os_printf("ESP8266 : SSID FRAMEWORK : INPUT MODE = HARDCODED\n");
                ESP8266_SSID_FRAMEWORK_HARDCODED_SSID_DETAILS _ssid_hardcoded_name_pwd = *(ESP8266_SSID_FRAMEWORK_HARDCODED_SSID_DETAILS*)user_data;
            break;

        case ESP8266_SSID_FRAMEWORK_SSID_INPUT_FLASH:
            if(_esp8266_ssid_framework_debug)
                os_printf("ESP8266 : SSID FRAMEWORK : INPUT MODE = FLASH\n");
                ESP8266_SSID_FRAMEWORK_FLASH_SSID_DETAILS _ssid_flash_name_pwd = *(ESP8266_SSID_FRAMEWORK_FLASH_SSID_DETAILS*)user_data;
            break;

        case ESP8266_SSID_FRAMEWORK_SSID_INPUT_EEPROM:
            if(_esp8266_ssid_framework_debug)
                os_printf("ESP8266 : SSID FRAMEWORK : INPUT MODE = EEPROM\n");
                ESP8266_SSID_FRAMEWORK_EEPROM_SSID_DETAILS _ssid_eeprom_name_pwd = *(ESP8266_SSID_FRAMEWORK_EEPROM_SSID_DETAILS*)user_data;
            break;

        case ESP8266_SSID_FRAMEWORK_SSID_INPUT_INTERNAL:
            if(_esp8266_ssid_framework_debug)
                os_printf("ESP8266 : SSID FRAMEWORK : INPUT MODE = INTERNAL\n");
            break;

        case ESP8266_SSID_FRAMEWORK_SSID_INPUT_GPIO:
            if(_esp8266_ssid_framework_debug)
                os_printf("ESP8266 : SSID FRAMEWORK : INPUT MODE = GPIO\n");
                _ssid_gpio_trigger_pin = *(uint8_t*)user_data;
                //SET TRIGGER GPIO AS INPUT
                ESP8266_GPIO_Set_Direction(_ssid_gpio_trigger_pin, 0);
            break;

        default:
            break;
    }

    switch(config_mode)
    {
        case ESP8266_SSID_FRAMEWORK_CONFIG_SMARTCONFIG:
            if(_esp8266_ssid_framework_debug)
                os_printf("ESP8266 : SSID FRAMEWORK : CONFIG MODE = SMARTCONFIG\n");
                //ESP8266_SMARTCONFIG_SetDebug(1);
            break;

        case ESP8266_SSID_FRAMEWORK_CONFIG_WEBCONFIG:
            if(_esp8266_ssid_framework_debug)
                os_printf("ESP8266 : SSID FRAMEWORK : CONFIG MODE = WEBCONFIG\n");
            break;

        default:
            break;
    }

    //SET THE LED GPIO AS OUTPUT
    ESP8266_GPIO_Set_Direction(_led_gpio_pin, 1);

    //SET LED GPIO TIMER CB FUNCTION
    os_timer_setfn(&_status_led_timer, _esp8266_ssid_framework_led_toggle_cb, NULL);
}

void ICACHE_FLASH_ATTR ESP8266_SSID_FRAMEWORK_SetCbFunctions(void (*wifi_connected_cb)(void))
{
    //SET THE USER CB FUNCTION TO BE CALLED WHEN THE WIFI IS CONNECTED

    _esp8266_ssid_framework_wifi_connected_user_cb = wifi_connected_cb;

    if(_esp8266_ssid_framework_debug)
    {
        os_printf("ESP8266 : SSID FRAMEWORK : Wifi connected user cb function set !\n");
    }
}

void ICACHE_FLASH_ATTR ESP8266_SSID_FRAMEWORK_Initialize(void)
{
    //START THE SSID FRAMEWORK WITH THE SET PARAMETERS

    if(_esp8266_ssid_framework_debug)
    {
        os_printf("ESP8266 : SSID FRAMEWORK : Running !\n");
    }

    //START LED TOGGLE @ 250ms
    os_timer_arm(&_status_led_timer, 250, 1);

    //SET WIFI EVENTS FUNCTION
    wifi_set_event_handler_cb(_esp8266_ssid_framework_wifi_event_handler_cb);

    //SPECIAL CASE: CHECK FOR GPIO TRIGGER IF FRAMEWORK INPUT = GPIO
    if(ESP8266_GPIO_Get_Value(_ssid_gpio_trigger_pin) == 1)
    {
        if(_esp8266_ssid_framework_debug)
        {
            os_printf("ESP8266 : SSID FRAMEWORK : GPIO input triggered !\n");
        }
        _esp8266_ssid_framework_wifi_start_ssid_configuration();
    }
    else
    {
        //START WIFI CONNECTION PROCESS

        _esp8266_ssid_framework_wifi_start_connection_process();
    }
}

void ICACHE_FLASH_ATTR _esp8266_ssid_framework_led_toggle_cb(void* pArg)
{
    //STATUS LED TOGGLE TIMER CB FUNCTION

    if(ESP8266_GPIO_Get_Value(_led_gpio_pin))
        ESP8266_GPIO_Set_Value(_led_gpio_pin, 0);
    else
        ESP8266_GPIO_Set_Value(_led_gpio_pin, 1);
}

void ICACHE_FLASH_ATTR _esp8266_ssid_framework_wifi_connect_timer_cb(void* pArg)
{
    //WIFI CONNECTION TIMER CB FUNCTION
    if(_esp8266_ssid_framework_wifi_connected == 0)
    {
        //WIFI NOT CONNECTED
        if(_esp8266_ssid_framework_debug)
        {
            os_printf("ESP8266 : SSID FRAMEWORK : wifi connection fail. Try #%u\n", _ssid_connect_retry_count);
        }
        _ssid_connect_retry_count++;

        //ARM THE TIMER
        os_timer_arm(&_wifi_connect_timer, _ssid_connect_retry_delay_ms, 0);

        if(_ssid_connect_retry_count > _ssid_connect_retries)
        {
            //ALL TRIES EXPIRED
            if(_esp8266_ssid_framework_debug)
            {
                os_printf("ESP8266 : SSID FRAMEWORK : wifi connection tries finished", _ssid_connect_retry_count);
            }

            //DISARM THE TIMER
            os_timer_disarm(&_wifi_connect_timer);

            //START THE SSID CONFIGURATION PROCESS
            _esp8266_ssid_framework_wifi_start_ssid_configuration();
        }
        else
        {
            //ATTEMPT CONNECT TO WIFI AGAIN
            wifi_station_connect();
        }
    }
    else
    {
        //WIFI CONNECTED
        //DISARM TIMER
        os_timer_disarm(&_wifi_connect_timer);
    }
}

void ICACHE_FLASH_ATTR _esp8266_ssid_framework_wifi_event_handler_cb(System_Event_t* event)
{
    //INTERNAL CB FUNCTION FOR WIFI EVENTS

    switch(event->event)
    {
        case EVENT_STAMODE_CONNECTED:
            if(_esp8266_ssid_framework_debug)
            {
                os_printf("ESP8266 : SSID FRAMEWORK : wifi event CONNECTED\n");
            }
            break;
        case EVENT_STAMODE_DISCONNECTED:
            _esp8266_ssid_framework_wifi_connected = 0;
            if(_esp8266_ssid_framework_debug)
            {
                os_printf("ESP8266 : SSID FRAMEWORK : wifi event DISCONNECTED\n");
            }
            break;
        case EVENT_STAMODE_AUTHMODE_CHANGE:
           if(_esp8266_ssid_framework_debug)
            {
                os_printf("ESP8266 : SSID FRAMEWORK : wifi event AUTHMODE_CHANGE\n");
            }
            break;
        case EVENT_STAMODE_GOT_IP:
            //DEVICE CONNECTED TO WIFI. CALL USER CB FUNCTION
            //TO START THE APPLICATION
            //CALL USER CB IF NOT NULL
            _esp8266_ssid_framework_wifi_connected = 1;
            //STOP STATUS LED TOGGLING
            os_timer_disarm(&_status_led_timer);
            //TURN OFF LED
            ESP8266_GPIO_Set_Value(_led_gpio_pin, 0);
            if(_esp8266_ssid_framework_wifi_connected_user_cb != NULL)
            {
                (*_esp8266_ssid_framework_wifi_connected_user_cb)();
            }
            break;
        case EVENT_SOFTAPMODE_STACONNECTED:
            if(_esp8266_ssid_framework_debug)
            {
                os_printf("ESP8266 : SSID FRAMEWORK : wifi event SOFTAP STA CONNECTED\n");
            }
            break;
        case EVENT_SOFTAPMODE_STADISCONNECTED:
            if(_esp8266_ssid_framework_debug)
            {
                os_printf("ESP8266 : SSID FRAMEWORK : wifi event SOFTAO STA DISCONNECTED\n");
            }
            break;
        default:
            if(_esp8266_ssid_framework_debug)
            {
                os_printf("ESP8266 : SSID FRAMEWORK : Unknow wifi event %d\n", event->event);
            }
    }
}

void ICACHE_FLASH_ATTR _esp8266_ssid_framework_wifi_start_ssid_configuration(void)
{
    //START THE SSID CONFIGURATION BASED ON CONFIG MODE

    if(_config_mode == ESP8266_SSID_FRAMEWORK_CONFIG_SMARTCONFIG)
    {
        //START SMARTCONFIG

        if(_esp8266_ssid_framework_debug)
        {
            os_printf("ESP8266 : SSID FRAMEWORK : Starting config = smartconfig\n");
        }
        ESP8266_SMARTCONFIG_SetDebug(1);
        ESP8266_SMARTCONFIG_Initialize();
        ESP8266_SMARTCONFIG_Start();
    }
    else if(_config_mode == ESP8266_SSID_FRAMEWORK_CONFIG_WEBCONFIG)
    {
        //START WEBCONFIG

        if(_esp8266_ssid_framework_debug)
        {
            os_printf("ESP8266 : SSID FRAMEWORK : Starting config = webconfig\n");
        }
    }
}

void ICACHE_FLASH_ATTR _esp8266_ssid_framework_wifi_start_connection_process(void)
{
    //START WIFI CONNECTION PROCESS
    //IF CONNECTION FAILS AFTER CONFIGURED NUMBER OF TIMES, SSID CONFIGURAION STARTS

    wifi_set_opmode(STATION_MODE);
    wifi_station_set_auto_connect(FALSE);
    wifi_station_set_reconnect_policy(FALSE);

    _esp8266_ssid_framework_wifi_connected = 0;
    _ssid_connect_retry_count = 1;

    //SETUP WIFI CONNECTION TIMER
    os_timer_setfn(&_wifi_connect_timer, _esp8266_ssid_framework_wifi_connect_timer_cb, NULL);
    os_timer_arm(&_wifi_connect_timer, _ssid_connect_retry_delay_ms, 0);

    wifi_station_connect();
}
