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

//HTML DATA RELEATED
static char* _config_page_html;
char* _user_data_ptrs[ESP8266_SSID_FRAMEWORK_CUSTOM_FIELD_MAX_COUNT];
static ESP8266_SSID_FRAMEWORK_CONFIG_USER_FIELD_GROUP* _custom_user_field_group;
static char* _project_name;

//SSID RELATED
static uint8_t _ssid_connect_retries;
static uint8_t _ssid_connect_retry_count;
static uint32_t _ssid_connect_retry_delay_ms;
static ESP8266_SSID_FRAMEWORK_HARDCODED_SSID_DETAILS _ssid_hardcoded_name_pwd;
static ESP8266_SSID_FRAMEWORK_FLASH_SSID_DETAILS _ssid_flash_name_pwd;
static ESP8266_SSID_FRAMEWORK_EEPROM_SSID_DETAILS _ssid_eeprom_name_pwd;
static uint8_t _ssid_gpio_trigger_pin;

//CB FUNCTIONS
static void (*_esp8266_ssid_framework_wifi_connected_user_cb)(char**);
//END LOCAL LIBRARY VARIABLES/////////////////////////////

void ICACHE_FLASH_ATTR ESP8266_SSID_FRAMEWORK_SetDebug(uint8_t debug_on)
{
    //SET DEBUG PRINTF ON(1) OR OFF(0)

    _esp8266_ssid_framework_debug = debug_on;
}

void ICACHE_FLASH_ATTR ESP8266_SSID_FRAMEWORK_SetParameters(ESP8266_SSID_FRAMEWORK_SSID_INPUT_MODE input_mode,
                                                            ESP8266_SSID_FRAMEWORK_CONFIG_MODE config_mode,
                                                            void* user_data,
                                                            ESP8266_SSID_FRAMEWORK_CONFIG_USER_FIELD_GROUP* user_field_data,
                                                            uint8_t retry_count,
                                                            uint32_t retry_delay_ms,
                                                            uint8_t gpio_led_pin,
																														char* project_name;)
{
    //SET THE SSID FRAMEWORK SSID INPUT MODE AND SSID CONFIGURATION MODE
    //VOID POINTER user_data INTERPRETED / CASTED AS PER MODE
    //
    //IMPORTANT : SINCE ESP8266 TAKES SOME TIME TO CONNECT TO WIFI, SET WIFI RETRY DELAY
    // TO ATLEAST 4000ms (4 SECONDS)!!

    _custom_user_field_group = user_field_data;

    if(user_field_data!= NULL && user_field_data->custom_fields_count > ESP8266_SSID_FRAMEWORK_CUSTOM_FIELD_MAX_COUNT)
    {
      if(_esp8266_ssid_framework_debug)
          os_printf("ESP8266 : SSID FRAMEWORK : Max %u custom fields allowed! Aborting\n", ESP8266_SSID_FRAMEWORK_CUSTOM_FIELD_MAX_COUNT);
      return;
    }

    _input_mode = input_mode;
    _config_mode = config_mode;

    _led_gpio_pin = gpio_led_pin;

    _ssid_connect_retries = retry_count;
    _ssid_connect_retry_delay_ms = retry_delay_ms;

		_project_name = project_name;

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

void ICACHE_FLASH_ATTR ESP8266_SSID_FRAMEWORK_SetCbFunctions(void (*wifi_connected_cb)(char**))
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

    //ALLOCATE BUFFER FOR CONFIG PAGE HTML
    _config_page_html = (char*)os_zalloc(3000);

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
                (*_esp8266_ssid_framework_wifi_connected_user_cb)(_user_data_ptrs);
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
        config_path.path_string = ESP8266_SSID_FRAMEWORK_WEBCONFIG_PATH_STRING;
        config_path.path_cb_fn = _esp8266_ssid_framework_tcp_server_path_config_cb;
        config_path.path_found = 0;

        //GENERATE THE CONFIG PAGE HTML
				strcpy(_config_page_html, "HTTP/1.1 200 OK\r\n"
                                  "Connection: Closed\r\n"
                                  "Content-type: text/html"
                                  "\r\n\r\n"
																	"<!DOCTYPE html>"
																	"<html>"
																	"<head>"
																	"<link rel=\"stylesheet\" href=\"https://maxcdn.bootstrapcdn.com/bootstrap/4.0.0-beta/css/bootstrap.min.css\">"
																	"<title>ESP8266 Web Config</title>"
																	"</head>"
																	"<body>"
																	"<div class=\"p-2 m-0 bg-dark text-white\">"
																	"<div class=\"container\">"
																	"<div class=\"row\">"
																	"<div class=\"col-md-12\">"
																	"<h1 class=\"\">ESP8266 Web Config</h1>"
																	"</div>"
																	"</div>"
																	"<div class=\"row\">"
																	"<div class=\"col-md-12 py-1\">"
																	"<h2 class=\"\">&lt;");
				//ADD PROJECT NAME
				strcpy(_config_page_html[os_strlen(_config_page_html)], _project_name);
				strcpy(_config_page_html[os_strlen(_config_page_html)], "&gt;</h2>");

				//ADD COMMON CONFIGURATION
				strcpy(_config_page_html[os_strlen(_config_page_html)], "</div>"
																																"</div>"
																																"</div>"
																																"</div>"
																																"<div class=\"p-2 m-0 bg-dark text-white\">"
																																"<div class=\"container\">"
																																"<div class=\"row\">"
																																"<div class=\"col-md-12 py-0 my-0\">"
																																"<p class=\"lead\"><i>Ankit Bhatnagar"
																																"<br>ankit.bhatnagarindia@gmail.com</i></p>"
																																"</div>"
																																"</div>"
																																"</div>"
																																"</div>"
																																"<div class=\"py-0\">"
																																"<form class=\"\\&quot;form-inline\\&quot;\" method=\"\\&quot;post\\&quot;\" action=\"\\&quot;/config\\&quot;\">"
																																"<div class=\"container py-3\">"
																																"<div class=\"row\">"
																																"<div class=\"col-md-6 border border-dark\">"
																																"<p class=\"lead\"><b>Common</b></p>"
																																"<input type=\"text\" name=\"ssid\" class=\"form-control my-2 w-75\" placeholder=\"SSID\">"
																																"<input type=\"text\" name=\"password\" class=\"form-control my-2 w-75\" placeholder=\"PASSWORD\">"
																																"<input type=\"submit\" class=\"btn my-3 text-center btn-success btn-sm w-50\"> </div>"
																																"<div class=\"col-md-6 border border-dark\">"
																																"<p class=\"lead\"><b>Project Specific</b></p>");

      //ADD CUSTOM CONFIG FIELDS IF ANY
      if(_custom_user_field_group != NULL &&_custom_user_field_group->custom_fields_count != 0)
      {
          //USER CUSTOM FIELDS PRESENT
          uint8_t i = 0;
          char* row_line = (char*)os_zalloc(200);
					const char* row_format_string = "<input type=\"text\" name=\"%s\" class=\"form-control w-75 my-2\" placeholder=\"%s\">";
          while(i < _custom_user_field_group->custom_fields_count)
          {
              os_sprintf(row_line, row_format_string,
                                (_custom_user_field_group->custom_fields + i)->custom_field_name,
                                (_custom_user_field_group->custom_fields + i)->custom_field_label);
              strcpy(_config_page_html + len_occupied, row_line);
              len_occupied += strlen(row_line);

              i++;
          }
          os_free(row_line);
      }

      //ADD MORE HTML
			strcpy(_config_page_html[os_strlen(_config_page_html)], "</div>"
																															"</div>"
																															"</div>"
																															"</form>"
																															"</div>"
																															"<div class=\"bg-dark py-3\">"
																															"<div class=\"container\">"
																															"<div class=\"row\">"
																															"<div class=\"col-md-12\">"
																															"<h3 class=\"text-white\" contenteditable=\"true\">System Stats</h3>"
																															"</div>"
																															"</div>"
																															"<div class=\"row\">"
																															"<div class=\"col-md-12 text-white py-0\">"
																															"<ul class=\"py-0\">");

			//ADD SYSTEM PARAMS SECTION
			strcpy(_config_page_html[os_strlen(_config_page_html)], "<li>CPU Frequency :&nbsp;</li>"
																															"<li>MAC Address :&nbsp;</li>"
																															"<li>Flash Chip ID :&nbsp;</li>"
																															"<li>Flash Map :&nbsp;</li>");

			//ADD ENDING HTML
			strcpy(_config_page_html[os_strlen(_config_page_html)], "</ul>"
																															"</div>"
																															"</div>"
																															"</div>"
																															"</div>"
																															"</body>"
																															"</html>");

        config_path.path_response = _config_page_html;
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
        wifi_station_set_config(sconfig);
        os_printf("ssid = %s**\n", sconfig->ssid);
        os_printf("pswd = %s**\n", sconfig->password);
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

      //EXTRACT SSID NAME / PASSWORD / CUSTOM FIELDS (IF ANY)
      char* ssid_name = (char*)os_zalloc(ESP8266_SSID_FRAMEWORK_SSID_NAME_LEN);
      char* ssid_pswd = (char*)os_zalloc(ESP8266_SSID_FRAMEWORK_SSID_PSWD_LEN);
      char* user_ptrs[ESP8266_SSID_FRAMEWORK_CUSTOM_FIELD_MAX_COUNT];

      char* config_str = strstr(data, "ssid=");
      char* ptr1 = strtok(config_str, "&");
      char* ptr2 = strtok(NULL, "&");

      uint8_t i = 0;
      if(_custom_user_field_group != NULL)
      {
          while (i < _custom_user_field_group->custom_fields_count)
          {
              user_ptrs[i] = strtok(NULL, "&");
              i++;
          }
      }

      //EXTRACT SSID / PSWD
      strtok(ptr1, "=");
      char* ssid = strtok(NULL, "=");

      strtok(ptr2, "=");
      char* pswd = strtok(NULL, "=");

      //EXTRACT CUSTOM FIELDS DATA
      i = 0;
      if(_custom_user_field_group != NULL)
      {
          while (i < _custom_user_field_group->custom_fields_count)
          {
              strtok(user_ptrs[i], "=");
              _user_data_ptrs[i] = strtok(NULL, "=");
              i++;
          }
      }

      strncpy(ssid_name, ssid, ESP8266_SSID_FRAMEWORK_SSID_NAME_LEN);
      strncpy(ssid_pswd, pswd, ESP8266_SSID_FRAMEWORK_SSID_PSWD_LEN);

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
          struct station_config config;
          os_memset(config.ssid, 0, ESP8266_SSID_FRAMEWORK_SSID_NAME_LEN);
          os_memset(config.password, 0, ESP8266_SSID_FRAMEWORK_SSID_PSWD_LEN);
          os_memcpy(&config.ssid, ssid_name, ESP8266_SSID_FRAMEWORK_SSID_NAME_LEN);
          os_memcpy(&config.password, ssid_pswd, ESP8266_SSID_FRAMEWORK_SSID_PSWD_LEN);

          //STOP MDNS
          ESP8266_MDNS_Stop();

          //STOP TCP SERVER
          ESP8266_TCP_SERVER_Stop();

          //START WIFI CONNECTION ATTEMPT
          wifi_softap_dhcps_stop();
          _esp8266_ssid_framework_wifi_start_connection_process(&config);

          os_free(ssid_name);
          os_free(ssid_pswd);
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
