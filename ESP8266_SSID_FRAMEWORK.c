/*************************************************
* ESP8266 SSID CONNECT FRAMEWORK
* SOFT AP DETAILS
* //SSID = ESP8266 | PASSWORD = 123456789
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
* AUGUST 13 2017
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
    //
    //IMPORTANT : SINCE ESP8266 TAKES SOME TIME TO CONNECT TO WIFI, SET WIFI RETRY DELAY
    // TO ATLEAST 4000ms (4 SECONDS)!!
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
                _ssid_hardcoded_name_pwd = *(ESP8266_SSID_FRAMEWORK_HARDCODED_SSID_DETAILS*)user_data;
            break;

        case ESP8266_SSID_FRAMEWORK_SSID_INPUT_FLASH:
            if(_esp8266_ssid_framework_debug)
                os_printf("ESP8266 : SSID FRAMEWORK : INPUT MODE = FLASH\n");
                _ssid_flash_name_pwd = *(ESP8266_SSID_FRAMEWORK_FLASH_SSID_DETAILS*)user_data;
            break;

        case ESP8266_SSID_FRAMEWORK_SSID_INPUT_EEPROM:
            if(_esp8266_ssid_framework_debug)
                os_printf("ESP8266 : SSID FRAMEWORK : INPUT MODE = EEPROM\n");
                _ssid_eeprom_name_pwd = *(ESP8266_SSID_FRAMEWORK_EEPROM_SSID_DETAILS*)user_data;
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
    if(_input_mode == ESP8266_SSID_FRAMEWORK_SSID_INPUT_GPIO && ESP8266_GPIO_Get_Value(_ssid_gpio_trigger_pin) == 1)
    {
        //GPIO TRIGGERED. START SSID CONFIG
        if(_esp8266_ssid_framework_debug)
        {
            os_printf("ESP8266 : SSID FRAMEWORK : GPIO input triggered !\n");
        }
        _esp8266_ssid_framework_wifi_start_ssid_configuration();
    }
    else
    {
        //START WIFI CONNECTION ATTEMPT
        _esp8266_ssid_framework_wifi_start_connection_process(NULL);
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
                os_printf("ESP8266 : SSID FRAMEWORK : wifi connection tries finished\n", _ssid_connect_retry_count);
            }

            //DISARM THE TIMER
            os_timer_disarm(&_wifi_connect_timer);

            //START THE SSID CONFIGURATION PROCESS
            _esp8266_ssid_framework_wifi_start_ssid_configuration();
        }
        else
        {
            //ATTEMPT CONNECT TO WIFI AGAIN
            struct station_config* sconfig = (struct station_config*)pArg;
            if(sconfig != NULL)
            {
                wifi_station_set_config(sconfig);
            }
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
            _esp8266_ssid_framework_wifi_connected = 1;
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
        ESP8266_SMARTCONFIG_SetDebug(_esp8266_ssid_framework_debug);
        ESP8266_SMARTCONFIG_Initialize();
        ESP8266_SMARTCONFIG_Start();
    }
    else if(_config_mode == ESP8266_SSID_FRAMEWORK_CONFIG_WEBCONFIG)
    {
        //START TCP SERVER WEBCONFIG
        if(_esp8266_ssid_framework_debug)
        {
            os_printf("ESP8266 : SSID FRAMEWORK : Starting config = webconfig\n");
        }

        //START ESP8266 IN SOFTAP MODE
        _esp8266_ssid_framework_wifi_start_softap();
        if(_esp8266_ssid_framework_debug)
        {
            os_printf("ESP8266 : SSID FRAMEWORK : Starting SSID = ESP8266\n");
        }

        //START TCP SERVER - SOFTAP MODE
        ESP8266_TCP_SERVER_SetDebug(_esp8266_ssid_framework_debug);
        ESP8266_TCP_SERVER_Initialize(80, 1200, 1);
        ESP8266_TCP_SERVER_SetDataEndingString("\r\n\r\n");
        ESP8266_TCP_SERVER_SetCallbackFunctions(NULL, NULL, NULL, NULL, _esp8266_ssid_framework_tcp_server_post_data_cb);

        //REGISTER PATH CALLBACKS
        ESP8266_TCP_SERVER_PATH_CB_ENTRY config_path;
        config_path.path_string = "/config";
        config_path.path_cb_fn = _esp8266_ssid_framework_tcp_server_path_config_cb;
        config_path.path_found = 0;
        const char *config_page = {
           "HTTP/1.1 200 OK\r\n"
           "Connection: Closed\r\n"
           "Content-type: text/html"
           "\r\n\r\n"
           "<html><head><title>ESP8266 POWER OUTAGE LOGGER - Config</title>"
           "</head>"
           "<body>"
           "<h3>ESP8266 POWER OUTAGE LOGGER - Config</h3>"
           "<form action=\"/config\" method=\"POST\">"
           "SSID : <input type=\"text\" name=\"ssid\">"
           "<br><br>"
           "PASSWORD : <input type=\"text\" name=\"password\">"
           "<br><br>"
            "<input type=\"submit\" value=\"Save\">"
           "</form>"
           "</body></html>"
        };
        config_path.path_response = config_page;
        ESP8266_TCP_SERVER_RegisterUrlPathCb(config_path);

        ESP8266_TCP_SERVER_Start();

        //START MDNS(SOFTAP MODE)
        ESP8266_MDNS_SetDebug(_esp8266_ssid_framework_debug);
        ESP8266_MDNS_Initialize("esp8266", "esp8266", 80, 1);
    }
}

void ICACHE_FLASH_ATTR _esp8266_ssid_framework_wifi_start_connection_process(struct station_config* sconfig)
{
    //START WIFI CONNECTION PROCESS
    //IF CONNECTION FAILS AFTER CONFIGURED NUMBER OF TIMES, SSID CONFIGURAION STARTS

    struct station_config config;

    _esp8266_ssid_framework_wifi_connected = 0;
    _ssid_connect_retry_count = 1;

    wifi_set_opmode(STATION_MODE);
    os_delay_us(100);
    wifi_station_set_auto_connect(FALSE);
    wifi_station_set_reconnect_policy(FALSE);

    switch(_input_mode)
    {
        case ESP8266_SSID_FRAMEWORK_SSID_INPUT_GPIO:
        case ESP8266_SSID_FRAMEWORK_SSID_INPUT_INTERNAL:
            //DO NOTHING. SIMPLY ATTEMP TO CONNECT TO WIFI
            break;

        case ESP8266_SSID_FRAMEWORK_SSID_INPUT_HARDCODED:
            //CHECK IF ESP8266 HAS VALID INTERNAL STORED WIFI CREDENTIALS
            //IF PRESENT, USE THOSE TO ATTEMPT TO CONNECT TO WIFI
            //IF NOT PRESENT, USE THE HARDCODED SSID DATA
            if(wifi_station_get_config_default(&config))
            {
                os_printf("hardcoded : internal saved ssid = %s\n", config.ssid);

                //CHECK IF RETREIVED INTERNAL CONFIG VALID
                //IF NOT, USE HARDCODED SSID DETAILS
                if(strcmp(config.ssid, "") != 0)
                {
                    //USE INTERNAL CREDENTIALS
                    //SO DO NOTHING
                }
                else
                {
                    os_printf("hardcoded : internal saved ssid invalid. using hardcoded ssid\n");
                    os_printf("%s\n",_ssid_hardcoded_name_pwd.ssid_name);
                    os_printf("%s\n",_ssid_hardcoded_name_pwd.ssid_pwd);
                    //USE HARDCODED CREDENTIALS
                    os_memset(config.ssid, 0, ESP8266_SSID_FRAMEWORK_SSID_NAME_LEN);
                    os_memset(config.password, 0, ESP8266_SSID_FRAMEWORK_SSID_PSWD_LEN);
                    os_memcpy(&config.ssid, _ssid_hardcoded_name_pwd.ssid_name, strlen(_ssid_hardcoded_name_pwd.ssid_name));
                    os_memcpy(&config.password, _ssid_hardcoded_name_pwd.ssid_pwd, strlen(_ssid_hardcoded_name_pwd.ssid_pwd));
                    wifi_station_set_config(&config);
                }
            }
            else
            {
                //START WIFI WITH HARDCODED CREDENTIALS
                os_memset(config.ssid, 0, ESP8266_SSID_FRAMEWORK_SSID_NAME_LEN);
                os_memset(config.password, 0, ESP8266_SSID_FRAMEWORK_SSID_PSWD_LEN);
                os_memcpy(&config.ssid, _ssid_hardcoded_name_pwd.ssid_name, ESP8266_SSID_FRAMEWORK_SSID_NAME_LEN);
                os_memcpy(&config.password, _ssid_hardcoded_name_pwd.ssid_pwd, ESP8266_SSID_FRAMEWORK_SSID_PSWD_LEN);
            }
            break;

        case ESP8266_SSID_FRAMEWORK_SSID_INPUT_FLASH:
            //READ SSID/PSWD FROM SPECIFIED FLASH ADDRESS
            //USE THOSE TO ATTEMPT TO CONNECT TO SSID
            break;

        case ESP8266_SSID_FRAMEWORK_SSID_INPUT_EEPROM:
            //READ SSID/PSWD FROM SPECIFIED EEPROM ADDRESS
            //USE THOSE TO ATTEMPT TO CONNECT TO SSID
            break;
    }

    if(sconfig != NULL)
    {
        os_printf("------------ %s\n", sconfig->ssid);
        os_printf("------------ %s\n", sconfig->password);
        wifi_station_set_config(sconfig);
    }

    wifi_station_connect();
    wifi_station_dhcpc_start();

    //SETUP WIFI CONNECTION TIMER
    os_timer_setfn(&_wifi_connect_timer, _esp8266_ssid_framework_wifi_connect_timer_cb, sconfig);
    os_timer_arm(&_wifi_connect_timer, _ssid_connect_retry_delay_ms, 0);
}

void ICACHE_FLASH_ATTR _esp8266_ssid_framework_wifi_start_softap(void)
{
    //START SOFTAP ON ESP8266
    //SSID = ESP8266 | PASSWORD = 123456789

    struct softap_config config;

    wifi_set_opmode(SOFTAP_MODE);

    wifi_softap_get_config(&config);
    os_memset(config.ssid, 0, ESP8266_SSID_FRAMEWORK_SSID_NAME_LEN);
    os_memset(config.password, 0, ESP8266_SSID_FRAMEWORK_SSID_PSWD_LEN);
    os_memcpy(config.ssid, "ESP8266", 7);
    os_memcpy(config.password, "12345678", 8);
    config.authmode = AUTH_WEP;
    config.ssid_hidden = 0;
    config.max_connection = 2;
    wifi_softap_set_config_current(&config);
}

void ICACHE_FLASH_ATTR _esp8266_ssid_framework_tcp_server_path_config_cb(void)
{
    //CB FUNCTION FOR CONFIG PATH FOUND IN TCP SERVER REQUESTS

    if(_esp8266_ssid_framework_debug)
    {
        os_printf("ESP8266 : SSID FRAMEWORK : Config path found!!\n");
    }
}

void ICACHE_FLASH_ATTR _esp8266_ssid_framework_tcp_server_post_data_cb(char* data, uint16_t len, uint8_t post_flag)
{
    //CB FUNCTION FOR TCP SERVER FOR TCP DATA RECEIVED
    //POST FLAG = 1 : POST DATA
    //POST FLAG = 0 : GET DATA (IGNORE : PATH CB WILL HANDLE IT)

    if(post_flag)
    {
      if(_esp8266_ssid_framework_debug)
      {
          os_printf("ESP8266 : SSID FRAMEWORK : SSID configuration data received!\n");
      }

      //EXTRACT SSID NAME / PASSWORD
      char ssid_name[ESP8266_SSID_FRAMEWORK_SSID_NAME_LEN];
      char ssid_pswd[ESP8266_SSID_FRAMEWORK_SSID_PSWD_LEN];

      char* config_str = strstr(data, "ssid=");
      char* ptr1 = strchr(config_str, '=');
      char* ptr2 = strchr(config_str, '&');
      char* ptr3 = strstr(ptr2, "=");

      strncpy(ssid_name, ptr1 + 1, (ptr2 - ptr1 - 1));
      strncpy(ssid_pswd, ptr3 + 1, ESP8266_SSID_FRAMEWORK_SSID_PSWD_LEN);

      //NULL TERMINATE SSID NAME STRING
      ssid_name[(ptr2 - ptr1 - 1)] = '\0';

      if(strcmp(ssid_name, "") == 0 || strcmp(ssid_pswd, "") == 0)
      {
          //EITHER THE SSID OR PASSWORD EMPTY
          os_printf("ESP8266 : SSID FRAMEWORK : Either ssid or password empty\n");
          return;
      }

      os_printf("ESP8266 : SSID FRAMEWORK : SSID name : %s\n", ssid_name);
      os_printf("ESP8266 : SSID FRAMEWORK : SSID passsword : %s\n", ssid_pswd);
      os_printf("ESP8266 : SSID FRAMEWORK : Connecting to SSID ...\n", ssid_pswd);

      //DEPENDING ON INPUT MODE, SAVE THE CREDENTIALS
      if(_input_mode == ESP8266_SSID_FRAMEWORK_SSID_INPUT_HARDCODED ||
          _input_mode == ESP8266_SSID_FRAMEWORK_SSID_INPUT_GPIO ||
          _input_mode == ESP8266_SSID_FRAMEWORK_SSID_INPUT_INTERNAL)
      {
          //NO NEED TO SAVE, JUST RESTART WIFI CONNECTION PROCESS WITH NEW CREDENTIALS
          //LET THE ESP8266 CACHE THE CREDENTIALS INTERNALLY
          struct station_config sconfig;
          os_memset(sconfig.ssid, 0, ESP8266_SSID_FRAMEWORK_SSID_NAME_LEN);
          os_memset(sconfig.password, 0, ESP8266_SSID_FRAMEWORK_SSID_PSWD_LEN);
          os_memcpy(&sconfig.ssid, ssid_name, ESP8266_SSID_FRAMEWORK_SSID_NAME_LEN);
          os_memcpy(&sconfig.password, ssid_pswd, ESP8266_SSID_FRAMEWORK_SSID_PSWD_LEN);

          //STOP MDNS
          ESP8266_MDNS_Stop();

          //STOP TCP SERVER
          ESP8266_TCP_SERVER_Stop();

          //START WIFI CONNECTION ATTEMPT
          wifi_softap_dhcps_stop();
          _esp8266_ssid_framework_wifi_start_connection_process(&sconfig);

      }
      else if(_input_mode == ESP8266_SSID_FRAMEWORK_SSID_INPUT_FLASH)
      {
          //SAVE CREDENTILS IN FLASH LOCATION
      }
      else if(_input_mode == ESP8266_SSID_FRAMEWORK_SSID_INPUT_EEPROM)
      {
          //SAVE CREDENTIALS IN EEPROM LOCATION
      }
    }
}
