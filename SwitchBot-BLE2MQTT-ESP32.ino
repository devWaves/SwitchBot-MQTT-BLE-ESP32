/** SwitchBot-MQTT-BLE-ESP32:

  https://github.com/devWaves/SwitchBot-MQTT-BLE-ESP32

  **does not use/require switchbot hub

  Code can be installed using Arduino IDE for ESP32
  Allows for "unlimited" switchbots devices to be controlled via MQTT sent to ESP32. ESP32 will send BLE commands to switchbots and return MQTT responses to the broker
     *  I do not know where performance will be affected by number of devices
     ** This is an unofficial SwitchBot integration. User takes full responsibility with the use of this code**

  v3.2

    Created: on June 13 2021
        Author: devWaves

        Contributions from:
			HardcoreWR

  based off of the work from https://github.com/combatistor/ESP32_BLE_Gateway

  Notes:
    - Supports Home Assistant MQTT Discovery

    - Support bots and curtains and meters

    - It works for button press/on/off, set mode, set hold seconds

    - It works for curtain open/close/pause/position(%)

    - It can request status values (bots/curtain/meter: battery, mode, state, position, temp etc) using a "rescan" for all devices

    - It can request individual device status values (bots/curtain/meter: battery, mode, state, position, temp etc) using a "requestInfo"

    - Good for placing one ESP32 in a zone with 1 or more devices that has a bad bluetooth signal from your smart hub. MQTT will use Wifi to "boost" the bluetooth signal

    - ESP32 bluetooth is pretty strong and one ESP32 can work for entire house. The code will try around 60 times to connect/push button. It should not need this many but it depends on ESP32 bluetooth signal to switchbots. If one alone doesn't work, get another esp32 and place it in the problem area

    - OTA update added. Go to ESP32 IP address in browser. In Arduino IDE menu - Sketch / Export compile Binary . Upload the .bin file

    - Supports passwords on bot

    - Automatically rescan every X seconds

    - Automatically requestInfo X seconds after successful control command

    - Retry set/control command on busy response from bot/curtain until success

    - Get settings from bot (firmware, holdSecs, inverted, number of timers)

    - Add a defined delay between each set/control commands or per device

    - ESP32 will collect hold time from bots and automatically wait holdSecs+defaultBotWaitTime until next command is sent to bot

    - Retry on no response curtain or bot


  ESP32 will subscribe to MQTT 'set' topic for every configure device.
      - <ESPMQTTTopic>/bot/<name>/set
      - <ESPMQTTTopic>/curtain/<name>/set
      - <ESPMQTTTopic>/meter/<name>/set

    Send a payload to the 'set' topic of the device you want to control
      Strings:
        - "PRESS"
        - "ON"
        - "OFF"
        - "OPEN"
        - "CLOSE"
        - "PAUSE"

      Integer 0-100 (for curtain position) Example: 50
      Integer 0-100 (for setting bot hold seconds) Example: 5           (for bot only) Does the same thing as <ESPMQTTTopic>/setHold

      Strings:
        - "REQUESTINFO" or "GETINFO"                                    (for bot and curtain) Does the same thing as calling <ESPMQTTTopic>/requestInfo
        - "REQUESTSETTINGS" or "GETSETTINGS"                            (for bot only) Does the same thing as calling <ESPMQTTTopic>/requestSettings        requires getBotResponse = true
        - "MODEPRESS", "MODESWITCH", "MODEPRESSINV", "MODESWITCHINV"    (for bot only) Does the same thing as <ESPMQTTTopic>/setMode

                      ESP32 will respond with MQTT on 'status' topic for every configured device
                        - <ESPMQTTTopic>/bot/<name>/status
                        - <ESPMQTTTopic>/curtain/<name>/status
                        - <ESPMQTTTopic>/meter/<name>/status

                        Example payload:
                          - {"status":"connected"}
                          - {"status":"press"}
                          - {"status":"errorConnect"}
                          - {"status":"errorCommand"}
                          - {"status":"commandSent"}
                          - {"status":"busy", "value":3}
                          - {"status":"failed", "value":9}
                          - {"status":"success", "value":1}
                          - {"status":"success", "value":5}
                          - {"status":"success"}

                       ESP32 will respond with MQTT on 'state' topic for every configured device
                        - <ESPMQTTTopic>/bot/<name>/state
                        - <ESPMQTTTopic>/curtain/<name>/state

                        Example payload:
                          - "ON"
                          - "OFF"
                          - "OPEN"
                          - "CLOSE"

                        ESP32 will respond with MQTT on 'position' topic for every configured device
                        - <ESPMQTTTopic>/curtain/<name>/position

                        Example payload:
                          - {"pos":500}
                          - {"pos":100}
                          - {"pos":50}


  ESP32 will Subscribe to MQTT topic to rescan for all device information
      - <ESPMQTTTopic>/rescan

      send a JSON payload of how many seconds you want to rescan for
          example payloads =
            {"sec":30}
            {"sec":"30"}

  ESP32 will Subscribe to MQTT topic for device information
      - <ESPMQTTTopic>/requestInfo

      send a JSON payload of the device you want to control
          example payloads =
            {"id":"switchbotone"}

                      ESP32 will respond with MQTT on
                        - <ESPMQTTTopic>/#

                        Example attribute responses per device are detected:
                          - <ESPMQTTTopic>/bot/<name>/attributes
                          - <ESPMQTTTopic>/curtain/<name>/attributes
                          - <ESPMQTTTopic>/meter/<name>/attributes

                        Example payloads:
                          - {"rssi":-78,"mode":"Press","state":"OFF","batt":94}
                          - {"rssi":-66,"calib":true,"batt":55,"pos":50,"state":"open","light":1}
                          - {"rssi":-66,"scale":"c","batt":55,"C":"21.5","F":"70.7","hum":"65"}

                        Example attribute responses per device are detected:
                          - <ESPMQTTTopic>/bot/<name>/state
                          - <ESPMQTTTopic>/curtain/<name>/state
                          - <ESPMQTTTopic>/meter/<name>/state

                        Example payload:
                          - "ON"
                          - "OFF"
                          - "OPEN"
                          - "CLOSE"

                        ESP32 will respond with MQTT on 'position' topic for every configured device
                        - <ESPMQTTTopic>/curtain/<name>/position

                        Example payload:
                          - {"pos":500}
                          - {"pos":100}
                          - {"pos":50}

  // REQUESTSETTINGS WORKS FOR BOT ONLY - DOCUMENTATION NOT AVAILABLE ONLINE FOR CURTAIN
  ESP32 will Subscribe to MQTT topic for device settings information (requires getBotResponse = true)
      - <ESPMQTTTopic>/requestSettings

    send a JSON payload of the device you want to control
          example payloads =
            {"id":"switchbotone"}

                      ESP32 will respond with MQTT on
                        - <ESPMQTTTopic>/#

                      Example responses per device are detected:
                        - <ESPMQTTTopic>/bot/<name>/settings

                      Example payloads:
                         - {"firmware":4.9,"timers":0,"inverted":false,"hold":5}


  // SET HOLD TIME ON BOT
  ESP32 will Subscribe to MQTT topic setting hold time on bots
      - <ESPMQTTTopic>/setHold

    send a JSON payload of the device you want to control
          example payloads =
            {"id":"switchbotone", "hold":5}
            {"id":"switchbotone", "hold":"5"}

    ESP32 will respond with MQTT on
    - <ESPMQTTTopic>/#

                      ESP32 will respond with MQTT on 'status' topic for every configured device
                        - <ESPMQTTTopic>/bot/<name>/status
                        - <ESPMQTTTopic>/curtain/<name>/status
                        - <ESPMQTTTopic>/meter/<name>/status

                        Example reponses:
                          - <ESPMQTTTopic>/bot/<name>/status
                          - <ESPMQTTTopic>/curtain/<name>/status
                          - <ESPMQTTTopic>/meter/<name>/status

                        Example payload:
                          - {"status":"connected"}
                          - {"status":"errorConnect"}
                          - {"status":"errorCommand"}
                          - {"status":"commandSent"}
                          - {"status":"busy", "value":3}
                          - {"status":"failed", "value":9}
                          - {"status":"success", "value":1}
                          - {"status":"success", "value":5}
                          - {"status":"success"}


  // SET MODE ON BOT
  ESP32 will Subscribe to MQTT topic setting mode for bots
      - <ESPMQTTTopic>/setMode

    send a JSON payload of the device you want to control
          example payloads =
            {"id":"switchbotone", "mode":"MODEPRESS"}
            {"id":"switchbotone", "mode":"MODESWITCH"}
            {"id":"switchbotone", "mode":"MODEPRESSINV"}
            {"id":"switchbotone", "mode":"MODESWITCHINV"}

                      ESP32 will respond with MQTT on 'status' topic for every configured device
                        - <ESPMQTTTopic>/bot/<name>/status
                        - <ESPMQTTTopic>/curtain/<name>/status
                        - <ESPMQTTTopic>/meter/<name>/status

                        Example reponses:
                          - <ESPMQTTTopic>/bot/<name>/status
                          - <ESPMQTTTopic>/curtain/<name>/status
                          - <ESPMQTTTopic>/meter/<name>/status

                        Example payload:
                          - {"status":"connected"}
                          - {"status":"errorConnect"}
                          - {"status":"errorCommand"}
                          - {"status":"commandSent"}
                          - {"status":"busy", "value":3}
                          - {"status":"failed", "value":9}
                          - {"status":"success", "value":1}
                          - {"status":"success", "value":5}

  ESP32 will respond with MQTT on ESPMQTTTopic with ESP32 status
      - <ESPMQTTTopic>

      example payloads:
        {status":"idle"}
        {status":"scanning"}
        {status":"boot"}

  Errors that cannot be linked to a specific device will be published to
      - <ESPMQTTTopic>
*/

#include <NimBLEDevice.h>
#include <EspMQTTClient.h>
#include <ArduinoJson.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WebServer.h>
#include <ESPmDNS.h>
#include <Update.h>
#include <CRC32.h>
#include <ArduinoQueue.h>

/****************** CONFIGURATIONS TO CHANGE *******************/

/********** REQUIRED SETTINGS TO CHANGE **********/

/* Wifi Settings */
static const char* host = "esp32";                   //  Unique name for ESP32. The name detected by your router and MQTT. If you are using more then 1 ESPs to control different switchbots be sure to use unique hostnames. Host is the MQTT Client name and is used in MQTT topics
static const char* ssid = "SSID";                    //  WIFI SSID
static const char* password = "Password";            //  WIFI Password

/* MQTT Settings */
/* MQTT Client name is set to WIFI host from Wifi Settings*/
static const char* mqtt_host = "192.168.0.1";                       //  MQTT Broker server ip
static const char* mqtt_user = "switchbot";                         //  MQTT Broker username. If empty, no authentication will be used
static const char* mqtt_pass = "switchbot";                         //  MQTT Broker password
static const int mqtt_port = 1883;                                  //  MQTT Port
static std::string mqtt_main_topic = "switchbot";                   //  MQTT main topic
static const uint16_t mqtt_packet_size = 1024;

/* Switchbot Bot Settings */
static std::map<std::string, std::string> allBots = {
  /*{ "switchbotone", "xX:xX:xX:xX:xX:xX" },
    { "switchbottwo", "yY:yY:yY:yY:yY:yY" }*/
};

/* Switchbot Meter Settings */
static std::map<std::string, std::string> allMeters = {
  /*{ "meterone", "xX:xX:xX:xX:xX:xX" },
    { "metertwo", "yY:yY:yY:yY:yY:yY" }*/
};

/* Switchbot Curtain Settings */
static int curtainClosedPosition = 10;    // When 2 curtains are controlled (left -> right and right -> left) it's possible one of the curtains pushes one of the switchbots more open. Change this value to set a position where a curtain is still considered closed
static std::map<std::string, std::string> allCurtains = {
  /*{ "curtainone", "xX:xX:xX:xX:xX:xX" },
    { "curtaintwo", "yY:yY:yY:yY:yY:yY" }*/
};

/* Switchbot Bot/Meter/Curtain Passwords */
static std::map<std::string, std::string> allPasswords = {     // Set all the bot passwords (setup in app first). Ignore if passwords are not used
  /*{ "switchbotone", "switchbotonePassword" },
    { "switchbottwo", "switchbottwoPassword" }*/
};


/********** ADVANCED SETTINGS - ONLY NEED TO CHANGE IF YOU WANT TO TWEAK SETTINGS **********/

/* ESP32 LED Settings */
#define LED_PIN LED_BUILTIN                          // If your board doesn't have a defined LED_BUILTIN (You will get a compile error), comment this line out
//#define LED_PIN 2                                  // If your board doesn't have a defined LED_BUILTIN, uncomment this line out and replace 2 with the LED pin value
static bool ledHighEqualsON = true;                  // ESP32 board LED ON=HIGH (Default). If your ESP32 is turning OFF on scanning and turning ON while IDLE, then set this value to false
static bool ledOnBootScan = true;                    // Turn on LED during initial boot scan
static bool ledOnScan = true;                        // Turn on LED while scanning (non-boot)
static bool ledOnCommand = true;                     // Turn on LED while MQTT command is processing. If scanning, LED will blink after scan completes. You may not notice it, there is no delay after scan

/* Webserver Settings */
static bool useLoginScreen = false;                   //  use a basic login page to avoid unwanted access
static String otaUserId = "admin";                    //  user Id for OTA update. Ignore if useLoginScreen = false
static String otaPass = "admin";                      //  password for OTA update. Ignore if useLoginScreen = false
static WebServer server(80);                          //  default port 80

/* Home Assistant Settings */
static bool home_assistant_mqtt_discovery = true;                    // Enable to publish Home Assistant MQTT Discovery config
static std::string home_assistant_mqtt_prefix = "homeassistant";     // MQTT Home Assistant prefix
static bool home_assistant_expose_seperate_curtain_position = true;  // When enabled, a seperate sensor will be added that will expose the curtain position. This is useful when using the Prometheus integration to graph curtain positions. The cover entity doesn't expose the position for Prometheus
static bool home_assistant_use_opt_mode = false;                     // For bots in switch mode assume on/off right away. Optimistic mode. (Icon will change in HA). If devices were already configured in HA, you need to delete them and reboot esp32

/* Switchbot General Settings */
static int tryConnecting = 60;          // How many times to try connecting to bot
static int trySending = 30;             // How many times to try sending command to bot
static int initialScan = 120;           // How many seconds to scan for bots on ESP reboot and autoRescan. Once all devices are found scan stops, so you can set this to a big number
static int infoScanTime = 60;           // How many seconds to scan for single device status updates
static int rescanTime = 600;            // Automatically rescan for device info every X seconds (default 10 min)
static int queueSize = 50;              // Max number of control/requestInfo/rescan MQTT commands stored in the queue. If you send more then queueSize, they will be ignored
static int defaultBotWaitTime = 2;      // wait at least X seconds between control command send to bots. ESP32 will detect if bot is in press mode with a hold time and will add hold time to this value per device
static int defaultCurtainWaitTime = 0;  // wait at least X seconds between control command send to curtains
static int waitForResponseSec = 5;      // How many seconds to wait for a bot/curtain response
static int noResponseRetryAmount = 5;   // How many times to retry if no response received

static bool autoRescan = true;                   // perform automatic rescan (uses rescanTime and initialScan). If (home_assistant_mqtt_discovery = true) the value set here is ignored. autoRescan is on for HA
static bool scanAfterControl = true;             // perform requestInfo after successful control command (uses botScanTime). If (home_assistant_mqtt_discovery = true) the value set here is ignored. scanAfterControl is on for HA
static bool waitBetweenControl = true;           // wait between commands sent to bot/curtain (avoids sending while bot is busy)
static bool getSettingsOnBoot = true;            // Currently only works for bot (curtain documentation not available but can probably be reverse engineered easily). Get bot extra settings values like firmware, holdSecs, inverted, number of timers. ***If holdSecs is available it is used by waitBetweenControl
static bool getBotResponse = true;               // get a response from the bot devices. A response of "success" means the most recent command was successful. A response of "busy" means the bot was busy when the command was sent
static bool getCurtainResponse = true;           // get a response from the curtain devices. A response of "success" means the most recent command was successful. A response of "busy" means the bot was busy when the command was sent
static bool retryBotOnBusy = true;               // Requires getBotResponse = true. if bot responds with busy, the last control command will retry until success
static bool retryCurtainOnBusy = true;           // Requires getCurtainResponse = true. if curtain responds with busy, the last control command will retry until success
static bool retryBotActionNoResponse = false;    // Retry if bot doesn't send a response. Bot default is false because no response can still mean the bot triggered.
static bool retryBotSetNoResponse = true;        // Retry if bot doesn't send a response when requesting settings (hold, firwmare etc) or settings hold/mode
static bool retryCurtainNoResponse = true;       // Retry if curtain doesn't send a response. Default is true. It shouldn't matter if curtain receives the same command twice (or multiple times)
static bool immediateBotStateUpdate = true;      // ESP32 will send ON/OFF state update as soon as MQTT is received. You can set this = false if not using Home Assistant Discovery.
static bool immediateCurtainStateUpdate = true;  // ESP32 will send OPEN/CLOSE and Position state update as soon as MQTT is received. You can set this = false if not using Home Assistant Discovery.

/* Switchbot Bot/Meter/Curtain scan interval */
/* Meters don't support commands so will be scanned every <int> interval automatically if scanAfterControl = true */
/* Requires scanAfterControl = true */
static std::map<std::string, int> botScanTime = {     // X seconds after a successful control command ESP32 will perform a requestInfo on the bot. If a "hold time" is set on the bot include that value + 5to10 secs. Hold time is auto added to default value. Default is 10+Hold sec if not in list
  /*{ "switchbotone", 10 },
    { "switchbottwo", 10 },
    { "curtainone", 20 },
    { "curtaintwo", 20 },
    { "meterone", 60 },
    { "metertwo", 60 }*/
};

/* Requires waitBetweenControl = true. Switchbot Bot/Curtain wait between control commands - overrides defaults */
/* for Bots: if defaultBotWaitTime is greater, defaultBotWaitTime is used */
/* for Curtains: if defaultCurtainWaitTime is greater, defaultBotWaitTime is used */
/* The ESP32 will wait at least X seconds before sending the next command to each device. 2+ different Bots can still be controlled together */
/* The ESP32 will automatically collect hold Secs for bots and will wait at least that long */
static std::map<std::string, int> botWaitBetweenControlTimes = {
  /*{ "switchbotone", 5 },
    { "switchbottwo", 5 },
    { "curtainone", 5 },
    { "curtaintwo", 5 }*/
};

/*************************************************************/

/* ANYTHING CHANGED BELOW THIS COMMENT MAY RESULT IN ISSUES - ALL SETTINGS TO CONFIGURE ARE ABOVE THIS LINE */

/*
   Login page
*/

static const String versionNum = "v3.2";
static const String loginIndex =
  "<form name='loginForm'>"
  "<table bgcolor='A09F9F' align='center' style='top: 250px;position: relative;width: 30%;'>"
  "<tr>"
  "<td colspan=2>"
  "<center><font size=5><b>SwitchBot ESP32 MQTT version: " + versionNum + "</b></font> <font size=1><b>(Unofficial)</b></font></center>"
  "<center><font size=3>Hostname/MQTT Client Name: " + std::string(host).c_str() + "</font></center>"
  "<center><font size=3>MQTT Main Topic: " + std::string(mqtt_main_topic).c_str() + "</font></center>"
  "<br>"
  "</td>"
  "<br>"
  "<br>"
  "</tr>"
  "<tr>"
  "<td>Username:</td>"
  "<td><input type='text' size=25 name='userid'><br></td>"
  "</tr>"
  "<br>"
  "<br>"
  "<tr>"
  "<td>Password:</td>"
  "<td><input type='Password' size=25 name='pwd'><br></td>"
  "<br>"
  "<br>"
  "</tr>"
  "<tr>"
  "<td><input type='submit' onclick='check(this.form)' value='Login'></td>"
  "</tr>"
  "</table>"
  "</form>"
  "<script>"
  "function check(form)"
  "{"
  "if(form.userid.value=='" + otaUserId + "' && form.pwd.value=='" + otaPass + "')"
  "{"
  "window.open('/serverIndex')"
  "}"
  "else"
  "{"
  " alert('Error Password or Username')/*displays error message*/"
  "}"
  "}"
  "</script>";

/*
   Server Index Page
*/

static const String serverIndex =
  "<link rel='stylesheet' href='https://code.jquery.com/ui/1.12.1/themes/base/jquery-ui.css'>"
  "<style> .ui-progressbar { position: relative; }"
  ".progress-label { position: absolute; width: 100%; text-align: center; top: 6px; font-weight: bold; text-shadow: 1px 1px 0 #fff; }</style>"
  "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
  "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
  "<script src='https://code.jquery.com/ui/1.12.1/jquery-ui.js'></script>"
  "<script>"
  "$( function() { var progressbar = $( \"#progressbar\" ), progressLabel = $( '.progress-label' );"
  "progressbar.progressbar({ value: '0', change: function() {"
  "progressLabel.text( progressbar.progressbar( 'value' ) + '%' );},"
  "complete: function() { progressLabel.text( 'Uploaded!' ); } });"
  "progressbar.find( '.ui-progressbar-value' ).css({'background': '#8fdbb2'});"
  "$('form').submit(function(e){"
  "e.preventDefault();"
  "var form = $('#upload_form')[0];"
  "var data = new FormData(form);"
  " $.ajax({"
  "url: '/update',"
  "type: 'POST',"
  "data: data,"
  "contentType: false,"
  "processData:false,"
  "xhr: function() {"
  "var xhr = new window.XMLHttpRequest();"
  "xhr.upload.addEventListener('progress', function(evt) {"
  "if (evt.lengthComputable) {"
  "var per = evt.loaded / evt.total;"
  "progressbar.progressbar( \"value\", Math.round(per*100) );"
  "}"
  "}, false);"
  "return xhr;"
  "},"
  "success:function(d, s) {"
  "console.log('success!')"
  "},"
  "error: function (a, b, c) {"
  "}"
  "});"
  "});"
  "});"
  "</script>"
  "<table bgcolor='A09F9F' align='center' style='top: 250px;position: relative;width: 30%;'>"
  "<tr>"
  "<td colspan=2>"
  "<center><font size=5><b>SwitchBot ESP32 MQTT version: " + versionNum + "</b></font> <font size=1><b>(Unofficial)</b></font></center>"
  "<center><font size=3>Hostname/MQTT Client Name: " + std::string(host).c_str() + "</font></center>"
  "<center><font size=3>MQTT Main Topic: " + std::string(mqtt_main_topic).c_str() + "</font></center>"
  "<br>"
  "</td>"
  "<br>"
  "<br>"
  "</tr>"
  "<tr>"
  "<td>Upload .bin file:</td>"
  "<td><input type='file' name='update'></td>"
  "</tr>"
  "<tr>"
  "<td colspan='2'><div id='progressbar'><div class='progress-label'>0%</div></div></td>"
  "</tr>"
  "<tr>"
  "<td colspan='2' align='center'><input type='submit' value='Update'></td>"
  "</tr>"
  "</table>"
  "</form>";

static EspMQTTClient client(
  ssid,
  password,
  mqtt_host,                            //  MQTT Broker server ip
  mqtt_user,                            //  Can be omitted if not needed
  mqtt_pass,                            //  Can be omitted if not needed
  host,                                 //  Client name that uniquely identify your device
  mqtt_port                             //  MQTT Port
);

static bool home_assistant_discovery_set_up = false;
static std::string manufacturer = "WonderLabs SwitchBot";
static std::string curtainModel = "Curtain";
static std::string curtainName = "WoCurtain";
static std::string botModel = "Bot";
static std::string botName = "WoHand";
static std::string meterModel = "Meter";
static std::string meterName = "WoSensorTH";

static int ledONValue = HIGH;
static int ledOFFValue = LOW;

void scanEndedCB(NimBLEScanResults results);
void rescanEndedCB(NimBLEScanResults results);
void initialScanEndedCB(NimBLEScanResults results);
static std::map<std::string, NimBLEAdvertisedDevice*> allSwitchbotsDev = {};
static std::map<std::string, long> rescanTimes = {};
static std::map<std::string, std::string> allSwitchbots;
static std::map<std::string, std::string> allSwitchbotsOpp;
static std::map<std::string, bool> discoveredDevices = {};
static std::map<std::string, bool> botsInPressMode = {};
static std::map<std::string, int> botHoldSecs = {};
static std::map<std::string, long> lastCommandSent = {};
static std::map<std::string, std::string> deviceTypes;
static NimBLEScan* pScan;
static bool isRescanning = false;
static bool processing = false;
static bool initialScanComplete = false;
static bool lastCommandWasBusy = false;
static bool deviceHasBooted = false;
static bool gotSettings = false;
static bool lastCommandSentPublished = false;

static std::string ESPMQTTTopic = mqtt_main_topic + "/" + std::string(host);
//static std::string esp32Topic = ESPMQTTTopic + "/ESP32";
static std::string lastWillStr = ESPMQTTTopic + "/lastwill";
static const char* lastWill = lastWillStr.c_str();
static std::string botTopic = ESPMQTTTopic + "/bot/";
static std::string curtainTopic = ESPMQTTTopic + "/curtain/";
static std::string meterTopic = ESPMQTTTopic + "/meter/";
static std::string rescanStdStr = ESPMQTTTopic + "/rescan";
//static std::string controlStdStr = ESPMQTTTopic + "/control";
static std::string controlStdStr = ESPMQTTTopic + "/#/#/set";
static std::string requestInfoStdStr = ESPMQTTTopic + "/requestInfo";
static std::string requestSettingsStdStr = ESPMQTTTopic + "/requestSettings";
static std::string setModeStdStr = ESPMQTTTopic + "/setMode";
static std::string setHoldStdStr = ESPMQTTTopic + "/setHold";
static const String rescanTopic = rescanStdStr.c_str();
static const String controlTopic = controlStdStr.c_str();
static const String requestInfoTopic = requestInfoStdStr.c_str();
static const String requestSettingsTopic = requestSettingsStdStr.c_str();
static const String setModeTopic = setModeStdStr.c_str();
static const String setHoldTopic = setHoldStdStr.c_str();

static byte bArrayPress[] = {0x57, 0x01};
static byte bArrayOn[] = {0x57, 0x01, 0x01};
static byte bArrayOff[] = {0x57, 0x01, 0x02};
static byte bArrayOpen[] =  {0x57, 0x0F, 0x45, 0x01, 0x05, 0xFF, 0x00};
static byte bArrayClose[] = {0x57, 0x0F, 0x45, 0x01, 0x05, 0xFF, 0x64};
static byte bArrayPause[] = {0x57, 0x0F, 0x45, 0x01, 0x00, 0xFF};
static byte bArrayPos[] =  {0x57, 0x0F, 0x45, 0x01, 0x05, 0xFF, NULL};
static byte bArrayGetSettings[] = {0x57, 0x02};
static byte bArrayHoldSecs[] = {0x57, 0x0F, 0x08, NULL };
static byte bArrayBotMode[] = {0x57, 0x03, 0x64, NULL, NULL};

static byte bArrayPressPass[] = {0x57, 0x11, NULL, NULL, NULL, NULL};
static byte bArrayOnPass[] = {0x57, 0x11, NULL , NULL, NULL, NULL, 0x01};
static byte bArrayOffPass[] = {0x57, 0x11, NULL, NULL, NULL, NULL, 0x02};
static byte bArrayGetSettingsPass[] = {0x57, 0x12, NULL, NULL, NULL, NULL};
static byte bArrayHoldSecsPass[] = {0x57, 0x1F, NULL, NULL, NULL, NULL, 0x08, NULL };
//static byte bArrayBotModePass[] = {0x57, 0x13, 0x64, NULL, NULL, NULL, NULL, NULL};     // Other github documentation shows this to be the array for setting mode with password (firmware 4.5, 4.6)
static byte bArrayBotModePass[] = {0x57, 0x13, NULL, NULL, NULL, NULL, 0x64, NULL};       // The proper array to use for setting mode with password (firmware 4.9)

struct to_lower {
  int operator() ( int ch )
  {
    return std::tolower ( ch );
  }
};

struct QueueCommand {
  std::string payload;
  std::string topic;
  std::string device;
};

ArduinoQueue<QueueCommand> commandQueue(queueSize);

static long lastOnlinePublished = 0;
static long lastRescan = 0;
static long lastScanCheck = 0;
static int currentRetry = 0;
static bool noResponse = false;
static bool waitForResponse = false;

void publishLastwillOnline() {
  if ((millis() - lastOnlinePublished) > 30000) {
    if (client.isConnected()) {
      client.publish(lastWill, "online", true);
      lastOnlinePublished = millis();
    }
  }
}

void publishHomeAssistantDiscoveryBotConfig(std::string deviceName, std::string deviceMac, bool optimistic) {
  std::transform(deviceMac.begin(), deviceMac.end(), deviceMac.begin(), ::toupper);
  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/battery/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Battery\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_battery\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"dev_cla\":\"battery\"," +
                 + "\"unit_of_meas\": \"%\", " +
                 + "\"value_template\":\"{{ value_json.batt }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/linkquality/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Linkquality\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_linkquality\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"icon\":\"mdi:signal\"," +
                 + "\"unit_of_meas\": \"rssi\", " +
                 + "\"value_template\":\"{{ value_json.rssi }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/binary_sensor/" + deviceName + "/inverted/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Inverted\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "inverted\"," +
                 + "\"stat_t\":\"~/settings\"," +
                 + "\"pl_on\":true," +
                 + "\"pl_off\":false," +
                 + "\"value_template\":\"{{ value_json.inverted }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/mode/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Mode\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_mode\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"value_template\":\"{{ value_json.mode }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/firmware/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Firmware\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_firmware\"," +
                 + "\"stat_t\":\"~/settings\"," +
                 + "\"value_template\":\"{{ value_json.firmware }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/holdsecs/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " HoldSecs\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_holdsecs\"," +
                 + "\"stat_t\":\"~/settings\"," +
                 + "\"value_template\":\"{{ value_json.hold }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/timers/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Timers\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_timers\"," +
                 + "\"stat_t\":\"~/settings\"," +
                 + "\"value_template\":\"{{ value_json.timers }}\"}").c_str(), true);

  std::string optiString;
  if (optimistic) {
    optiString = "true";
  }
  else {
    optiString = "false";
  }

  client.publish((home_assistant_mqtt_prefix + "/switch/" + deviceName + "/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\", " +
                 + "\"name\":\"" + deviceName + " Switch\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "\", " +
                 + "\"stat_t\":\"~/state\", " +
                 + "\"opt\":" + optiString + ", " +
                 + "\"cmd_t\": \"~/set\" }").c_str(), true);

}

void publishHomeAssistantDiscoveryCurtainConfig(std::string deviceName, std::string deviceMac) {
  std::transform(deviceMac.begin(), deviceMac.end(), deviceMac.begin(), ::toupper);
  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/battery/config").c_str(), ("{\"~\":\"" + (curtainTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Battery\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + curtainModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_battery\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"dev_cla\":\"battery\"," +
                 + "\"unit_of_meas\": \"%\", " +
                 + "\"value_template\":\"{{ value_json.batt }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/linkquality/config").c_str(), ("{\"~\":\"" + (curtainTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Linkquality\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + curtainModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_linkquality\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"icon\":\"mdi:signal\"," +
                 + "\"unit_of_meas\": \"rssi\", " +
                 + "\"value_template\":\"{{ value_json.rssi }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/illuminance/config").c_str(), ("{\"~\":\"" + (curtainTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Illuminance\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + curtainModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_illuminance\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"dev_cla\":\"illuminance\"," +
                 + "\"value_template\":\"{{ value_json.light }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/binary_sensor/" + deviceName + "/calibrated/config").c_str(), ("{\"~\":\"" + (curtainTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Calibrated\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + curtainModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_calibrated\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"pl_on\":true," +
                 + "\"pl_off\":false," +
                 + "\"value_template\":\"{{ value_json.calib }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/cover/" + deviceName + "/config").c_str(), ("{\"~\":\"" + (curtainTopic + deviceName) + "\", " +
                 + "\"name\":\"" + deviceName + " Curtain\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + curtainModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "\", " +
                 + "\"dev_cla\":\"curtain\", " +
                 + "\"stat_t\":\"~/state\", " +
                 + "\"stat_open\": \"OPEN\", " +
                 + "\"stat_clsd\": \"CLOSE\", " +
                 + "\"pl_stop\":\"PAUSE\", " +
                 + "\"pos_open\": 100, " +
                 + "\"pos_clsd\": 0, " +
                 + "\"cmd_t\": \"~/set\", " +
                 + "\"pos_t\":\"~/position\", " +
                 + "\"pos_tpl\":\"{{ value_json.pos }}\", " +
                 + "\"set_pos_t\": \"~/set\", " +
                 + "\"set_pos_tpl\": \"{{ position }}\" }").c_str(), true);
  if (home_assistant_expose_seperate_curtain_position) {
    client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/position/config").c_str(), ("{\"~\":\"" + (curtainTopic + deviceName) + "\"," +
                   + "\"name\":\"" + deviceName + " Position\"," +
                   + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + curtainModel + "\",\"name\": \"" + deviceName + "\" }," +
                   + "\"avty_t\": \"" + lastWill + "\"," +
                   + "\"uniq_id\":\"switchbot_" + deviceMac + "_position\"," +
                   + "\"stat_t\":\"~/position\"," +
                   + "\"unit_of_meas\": \"%\", " +
                   + "\"value_template\":\"{{ value_json.pos }}\"}").c_str(), true);
  }
}

void publishHomeAssistantDiscoveryMeterConfig(std::string deviceName, std::string deviceMac) {
  std::transform(deviceMac.begin(), deviceMac.end(), deviceMac.begin(), ::toupper);
  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/battery/config").c_str(), ("{\"~\":\"" + (meterTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Battery\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + meterModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_battery\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"dev_cla\":\"battery\"," +
                 + "\"unit_of_meas\": \"%\", " +
                 + "\"value_template\":\"{{ value_json.batt }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/linkquality/config").c_str(), ("{\"~\":\"" + (meterTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Linkquality\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + meterModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_linkquality\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"icon\":\"mdi:signal\"," +
                 + "\"unit_of_meas\": \"rssi\", " +
                 + "\"value_template\":\"{{ value_json.rssi }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/temperature/config").c_str(), ("{\"~\":\"" + (meterTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Temperature\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + meterModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_temperature\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"dev_cla\":\"temperature\", " +
                 + "\"unit_of_meas\": \"°C\", " +
                 + "\"value_template\":\"{{ value_json.C }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/humidity/config").c_str(), ("{\"~\":\"" + (meterTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Humidity\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + meterModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_humidity\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"dev_cla\":\"humidity\", " +
                 + "\"unit_of_meas\": \"%\", " +
                 + "\"value_template\":\"{{ value_json.hum }}\"}").c_str(), true);
}

class ClientCallbacks : public NimBLEClientCallbacks {

    void onConnect(NimBLEClient* pClient) {
      Serial.println("Connected");
      pClient->updateConnParams(120, 120, 0, 60);
    };

    void onDisconnect(NimBLEClient* pClient) {
    };

    bool onConnParamsUpdateRequest(NimBLEClient* pClient, const ble_gap_upd_params* params) {
      if (params->itvl_min < 24) { /** 1.25ms units */
        return false;
      } else if (params->itvl_max > 40) { /** 1.25ms units */
        return false;
      } else if (params->latency > 2) { /** Number of intervals allowed to skip */
        return false;
      } else if (params->supervision_timeout > 100) { /** 10ms units */
        return false;
      }

      return true;
    };

    uint32_t onPassKeyRequest() {
      Serial.println("Client Passkey Request");
      return 123456;
    };

    bool onConfirmPIN(uint32_t pass_key) {
      Serial.print("The passkey YES/NO number: ");
      Serial.println(pass_key);
      return true;
    };

    void onAuthenticationComplete(ble_gap_conn_desc* desc) {
      if (!desc->sec_state.encrypted) {
        Serial.println("Encrypt connection failed - disconnecting");
        NimBLEDevice::getClientByID(desc->conn_handle)->disconnect();
        return;
      }
    };
};

bool unsubscribeToNotify(NimBLEClient* pClient) {
  NimBLERemoteService* pSvc = nullptr;
  NimBLERemoteCharacteristic* pChr = nullptr;

  pSvc = pClient->getService("cba20d00-224d-11e6-9fb8-0002a5d5c51b"); // custom device service
  if (pSvc) {    /** make sure it's not null */
    pChr = pSvc->getCharacteristic("cba20003-224d-11e6-9fb8-0002a5d5c51b"); // custom characteristic to notify
  }
  if (pChr) {    /** make sure it's not null */
    if (pChr->canNotify()) {
      if (!pChr->unsubscribe()) {
        return false;
      }
    }
  }
  else {
    Serial.println("CUSTOM notify service not found.");
    return false;
  }
  Serial.println("unsubscribed to notify");
  return true;
}

bool subscribeToNotify(NimBLEAdvertisedDevice* advDeviceToUse) {
  NimBLEClient* pClient = NimBLEDevice::getClientByPeerAddress(advDeviceToUse->getAddress());
  NimBLERemoteService* pSvc = nullptr;
  NimBLERemoteCharacteristic* pChr = nullptr;

  pSvc = pClient->getService("cba20d00-224d-11e6-9fb8-0002a5d5c51b"); // custom device service
  if (pSvc) {    /** make sure it's not null */
    pChr = pSvc->getCharacteristic("cba20003-224d-11e6-9fb8-0002a5d5c51b"); // custom characteristic to notify
  }
  if (pChr) {    /** make sure it's not null */
    if (pChr->canNotify()) {
      if (!pChr->subscribe(true, notifyCB)) {
        return false;
      }
    }
  }
  else {
    Serial.println("CUSTOM notify service not found.");
    return false;
  }
  Serial.println("subscribed to notify");
  return true;
}

bool writeSettings(NimBLEAdvertisedDevice* advDeviceToUse) {
  NimBLEClient* pClient = NimBLEDevice::getClientByPeerAddress(advDeviceToUse->getAddress());
  NimBLERemoteService* pSvc = nullptr;
  NimBLERemoteCharacteristic* pChr = nullptr;

  pSvc = pClient->getService("cba20d00-224d-11e6-9fb8-0002a5d5c51b"); // custom device service
  if (pSvc) {    /** make sure it's not null */
    pChr = pSvc->getCharacteristic("cba20002-224d-11e6-9fb8-0002a5d5c51b"); // custom characteristic to write
  }
  if (pChr) {    /** make sure it's not null */
    if (pChr->canWrite()) {
      std::string aPass = "";
      std::map<std::string, std::string>::iterator itU = allSwitchbotsOpp.find(advDeviceToUse->getAddress());
      if (itU != allSwitchbotsOpp.end())
      {
        aPass = getPass(itU->second.c_str());
      }
      uint8_t aPassCRC[4];
      if (aPass != "") {
        uint32_t aCRC = getPassCRC(aPass);
        for (int i = 0; i < 4; ++i)
        {
          aPassCRC[i] = ((uint8_t*)&aCRC)[3 - i];
        }

        byte bArray[] = {0x57, 0x12, aPassCRC[0] , aPassCRC[1] , aPassCRC[2]  , aPassCRC[3]}; // write to get settings of device
        if (pChr->writeValue(bArray, 6)) {
          Serial.print("Wrote new value to: ");
          Serial.println(pChr->getUUID().toString().c_str());
        }
        else {
          return false;
        }
      }
      else {
        byte bArray[] = {0x57, 0x02}; // write to get settings of device
        if (pChr->writeValue(bArray, 2)) {
          Serial.print("Wrote new value to: ");
          Serial.println(pChr->getUUID().toString().c_str());
        }
        else {
          return false;
        }
      }
    }
    else {
      Serial.println("CUSTOM write service not found.");
      return false;
    }
    Serial.println("Success! subscribed and got settings");
    return true;
  }
}

/** Define a class to handle the callbacks when advertisments are received */
class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
      Serial.print("Advertised Device found: ");
      Serial.println(advertisedDevice->toString().c_str());
      if (ledOnScan) {
        digitalWrite(LED_PIN, ledONValue);
      }
      std::string advStr = advertisedDevice->getAddress().toString().c_str();
      std::map<std::string, std::string>::iterator itS = allSwitchbotsOpp.find(advStr);

      bool gotAllStatus = false;

      publishLastwillOnline();

      if (itS != allSwitchbotsOpp.end())
      {
        if (advertisedDevice->isAdvertisingService(NimBLEUUID("cba20d00-224d-11e6-9fb8-0002a5d5c51b")))
        {
          std::map<std::string, NimBLEAdvertisedDevice*>::iterator itY = allSwitchbotsDev.find(advStr);
          if (itY == allSwitchbotsDev.end())
          {
            Serial.println("Adding Our Service ... ");
            Serial.println(itS->second.c_str());
            std::string aValueString = advertisedDevice->getServiceData(0);
            gotAllStatus = callForInfoAdvDev(advertisedDevice, aValueString);
            if (gotAllStatus) {
              allSwitchbotsDev.insert ( std::pair<std::string, NimBLEAdvertisedDevice*>(advStr, advertisedDevice) );
            }
            Serial.println("Assigned advDevService");

          }
        }
      }
      else {
        NimBLEDevice::addIgnored(advStr);
      }
      if (allSwitchbotsDev.size() == allBots.size() + allCurtains.size() + allMeters.size()) {
        Serial.println("Stopping Scan found devices ... ");
        NimBLEDevice::getScan()->stop();
      }
    };

    bool callForInfoAdvDev(NimBLEAdvertisedDevice* advDev,  std::string aValueString) {
      Serial.println("callForInfoAdvDev");
      std::string aDevice;
      std::string aState;
      std::string deviceStateTopic;
      std::string deviceAttrTopic;
      std::string deviceMac = advDev->getAddress().toString();
      std::map<std::string, bool>::iterator itM = discoveredDevices.find(deviceMac);
      int aLength = aValueString.length();
      StaticJsonDocument<200> doc;
      char aBuffer[200];
      std::map<std::string, std::string>::iterator itS = allSwitchbotsOpp.find(advDev->getAddress().toString().c_str());
      if (itS != allSwitchbotsOpp.end())
      {
        aDevice = itS->second.c_str();
      }
      else {
        return false;
      }

      std::string deviceName;
      itS = deviceTypes.find(advDev->getAddress().toString().c_str());
      if (itS != deviceTypes.end())
      {
        deviceName = itS->second.c_str();
      }

      Serial.printf("deviceName: %s\n", deviceName.c_str());

      doc["rssi"] = advDev->getRSSI();

      if (deviceName == botName) {
        if (aLength < 3) {
          return false;
        }
        deviceStateTopic = botTopic + aDevice + "/state";
        deviceAttrTopic = botTopic + aDevice + "/attributes";

        uint8_t byte1 = (uint8_t) aValueString[1];
        uint8_t byte2 = (uint8_t) aValueString[2];

        bool isSwitch = (byte1 & 0b10000000);
        std::string aMode = isSwitch ? "Switch" : "Press"; // Whether the light switch Add-on is used or not
        if (isSwitch) {
          std::map<std::string, bool>::iterator itP = botsInPressMode.find(deviceMac);
          if (itP != botsInPressMode.end())
          {
            botsInPressMode.erase(deviceMac);
          }
          aState = (byte1 & 0b01000000) ? "OFF" : "ON"; // Mine is opposite, not sure why
        }
        else {

          botsInPressMode.insert (std::pair<std::string, bool>(deviceMac, true));
          aState = "OFF";
        }
        int battLevel = byte2 & 0b01111111; // %

        doc["mode"] = aMode;
        doc["state"] = aState;
        doc["batt"] = battLevel;

        if (home_assistant_mqtt_discovery) {
          if (itM == discoveredDevices.end()) {
            Serial.printf("Publishing MQTT Discovery for %s (%s)\n", aDevice.c_str(), deviceMac.c_str());
            if (home_assistant_use_opt_mode) {
              publishHomeAssistantDiscoveryBotConfig(aDevice, deviceMac, isSwitch);
            }
            else {
              publishHomeAssistantDiscoveryBotConfig(aDevice, deviceMac, false);
            }
            discoveredDevices.insert( std::pair<std::string, bool>(deviceMac, true) );
          }
        }

      }
      else if (deviceName == meterName) {
        if (aLength < 6) {
          return false;
        }
        deviceStateTopic = meterTopic + aDevice + "/state";
        deviceAttrTopic = meterTopic + aDevice + "/attributes";

        uint8_t byte2 = (uint8_t) aValueString[2];
        uint8_t byte3 = (uint8_t) aValueString[3];
        uint8_t byte4 = (uint8_t) aValueString[4];
        uint8_t byte5 = (uint8_t) aValueString[5];

        int tempSign = (byte4 & 0b10000000) ? 1 : -1;
        float tempC = tempSign * ((byte4 & 0b01111111) + ((byte3 & 0b00001111) / 10.0));
        float tempF = (tempC * 9 / 5.0) + 32;
        tempF = round(tempF * 10) / 10.0;
        bool tempScale = (byte5 & 0b10000000) ;
        std::string str1 = (tempScale == true) ? "f" : "c";
        doc["scale"] = str1;
        int battLevel = (byte2 & 0b01111111);
        doc["batt"] = battLevel;
        doc["C"] = serialized(String(tempC, 1));
        doc["F"] = serialized(String(tempF, 1));
        int humidity = byte5 & 0b01111111;
        doc["hum"] = humidity;
        aState = String(tempC, 1).c_str();

        if (home_assistant_mqtt_discovery) {
          if (itM == discoveredDevices.end()) {
            Serial.printf("Publishing MQTT Discovery for %s (%s)\n", aDevice.c_str(), deviceMac.c_str());
            publishHomeAssistantDiscoveryMeterConfig(aDevice, deviceMac);
            discoveredDevices.insert( std::pair<std::string, bool>(deviceMac, true) );
          }
        }

        std::map<std::string, long>::iterator itW = rescanTimes.find(deviceMac);
        if (itW != rescanTimes.end())
        {
          rescanTimes.erase(deviceMac);
        }
        Serial.printf("Adding %s to rescanTimes...\n", deviceMac.c_str());
        rescanTimes.insert ( std::pair<std::string, long>(deviceMac, millis()));

      }
      else if (deviceName == curtainName) {
        if (aLength < 5) {
          return false;
        }
        std::string devicePosTopic = curtainTopic + aDevice + "/position";
        deviceStateTopic = curtainTopic + aDevice + "/state";
        deviceAttrTopic = curtainTopic + aDevice + "/attributes";

        uint8_t byte1 = (uint8_t) aValueString[1];
        uint8_t byte2 = (uint8_t) aValueString[2];
        uint8_t byte3 = (uint8_t) aValueString[3];
        uint8_t byte4 = (uint8_t) aValueString[4];

        bool calibrated = byte1 & 0b01000000;;
        int battLevel = byte2 & 0b01111111;
        int currentPosition = 100 - (byte3 & 0b01111111);
        int lightLevel = (byte4 >> 4) & 0b00001111;
        aState = "OPEN";

        doc["calib"] = calibrated;
        doc["batt"] = battLevel;
        doc["pos"] = currentPosition;
        if (currentPosition <= curtainClosedPosition)
          aState = "CLOSE";
        doc["state"] = aState;
        doc["light"] = lightLevel;

        if (home_assistant_mqtt_discovery) {
          if (itM == discoveredDevices.end()) {
            Serial.printf("Publishing MQTT Discovery for %s (%s)\n", aDevice.c_str(), deviceMac.c_str());
            publishHomeAssistantDiscoveryCurtainConfig(aDevice, deviceMac);
            discoveredDevices.insert( std::pair<std::string, bool>(deviceMac, true) );
          }
        }
        StaticJsonDocument<50> docPos;
        docPos["pos"] = currentPosition;
        serializeJson(docPos, aBuffer);
        client.publish(devicePosTopic.c_str(), aBuffer, true);
      }
      else {
        return false;
      }
      Serial.println("serializing");
      serializeJson(doc, aBuffer);
      client.publish(deviceStateTopic.c_str(), aState.c_str(), true);
      client.publish(deviceAttrTopic.c_str(), aBuffer, true);
      Serial.println("published");
      return true;
    };
};

void initialScanEndedCB(NimBLEScanResults results) {
  Serial.println("initialScanEndedCB");
  if (ledOnBootScan) {
    digitalWrite(LED_PIN, ledOFFValue);
  }
  initialScanComplete = true;

  Serial.println("Scan Ended");
  client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"idle\"}");
}

void scanEndedCB(NimBLEScanResults results) {
  if (ledOnScan || ledOnCommand) {
    digitalWrite(LED_PIN, ledOFFValue);
  }
  Serial.println("Scan Ended");
  client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"idle\"}");
}

void rescanEndedCB(NimBLEScanResults results) {
  if (ledOnScan || ledOnCommand) {
    digitalWrite(LED_PIN, ledOFFValue);
  }
  isRescanning = false;
  lastRescan = millis();
  Serial.println("ReScan Ended");
  client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"idle\"}");
}

std::string getPass(std::string aDevice) {
  std::map<std::string, std::string>::iterator itS = allPasswords.find(aDevice);
  std::string aPass = "";
  if (itS != allPasswords.end())
  {
    aPass = itS->second;
  }
  return aPass;
}

uint32_t getPassCRC(std::string aDevice) {
  const uint8_t * byteBuffer = (const unsigned char *)(aDevice.c_str());
  CRC32 crc;
  for (size_t i = 0; i < aDevice.length(); i++)
  {
    crc.update(byteBuffer[i]);
  }
  uint32_t checksum = crc.finalize();
  return checksum;
}

static ClientCallbacks clientCB;

void setup () {
  if (ledHighEqualsON) {
    ledONValue = HIGH;
    ledOFFValue = LOW;
  }
  else {
    ledONValue = LOW;
    ledOFFValue = HIGH;
  }

  pinMode (LED_PIN, OUTPUT);
  // Connect to WiFi network
  WiFi.begin(ssid, password);
  Serial.println("");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  /*use mdns for host name resolution*/
  if (!MDNS.begin(host)) { //http://esp32.local
    Serial.println("Error setting up MDNS responder!");
    while (1) {
      delay(1000);
    }
  }
  Serial.println("mDNS responder started");
  /*return index page which is stored in serverIndex */
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    if (useLoginScreen) {
      server.send(200, "text/html", loginIndex);
    }
    else {
      server.send(200, "text/html", serverIndex);
    }
  });
  server.on("/serverIndex", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      Serial.printf("Update: %s\n", upload.filename.c_str());
      if (!Update.begin(UPDATE_SIZE_UNKNOWN)) { //start with max available size
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_WRITE) {
      /* flashing firmware to ESP*/
      if (Update.write(upload.buf, upload.currentSize) != upload.currentSize) {
        Update.printError(Serial);
      }
    } else if (upload.status == UPLOAD_FILE_END) {
      if (Update.end(true)) { //true to set the size to the current progress
        Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
      } else {
        Update.printError(Serial);
      }
    }
  });
  server.begin();

  client.setMqttReconnectionAttemptDelay(100);
  client.enableLastWillMessage(lastWill, "offline");
  client.setKeepAlive(60);
  client.setMaxPacketSize(mqtt_packet_size);

  static std::map<std::string, std::string> allBotsTemp = {};
  static std::map<std::string, std::string> allCurtainsTemp = {};
  static std::map<std::string, std::string> allMetersTemp = {};

  std::map<std::string, std::string>::iterator it = allBots.begin();
  std::string anAddr;

  while (it != allBots.end())
  {
    anAddr = it->second;
    std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
    allSwitchbotsOpp.insert ( std::pair<std::string, std::string>(anAddr.c_str(), it->first) );
    allSwitchbots.insert ( std::pair<std::string, std::string>(it->first, anAddr.c_str()) );
    deviceTypes.insert ( std::pair<std::string, std::string>(anAddr.c_str(), botName) );
    allBotsTemp.insert ( std::pair<std::string, std::string>(it->first, anAddr.c_str()) );
    it++;
  }
  allBots = allBotsTemp;

  it = allCurtains.begin();
  while (it != allCurtains.end())
  {
    anAddr = it->second;
    std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
    allSwitchbotsOpp.insert ( std::pair<std::string, std::string>(anAddr.c_str(), it->first) );
    allSwitchbots.insert ( std::pair<std::string, std::string>(it->first, anAddr.c_str()) );
    deviceTypes.insert ( std::pair<std::string, std::string>(anAddr.c_str(), curtainName) );
    allCurtainsTemp.insert ( std::pair<std::string, std::string>(it->first, anAddr.c_str()) );
    it++;
  }
  allCurtains = allCurtainsTemp;

  it = allMeters.begin();
  while (it != allMeters.end())
  {
    anAddr = it->second;
    std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
    allSwitchbotsOpp.insert ( std::pair<std::string, std::string>(anAddr.c_str(), it->first) );
    allSwitchbots.insert ( std::pair<std::string, std::string>(it->first, anAddr.c_str()) );
    deviceTypes.insert ( std::pair<std::string, std::string>(anAddr.c_str(), meterName) );
    allMetersTemp.insert ( std::pair<std::string, std::string>(it->first, anAddr.c_str()) );
    it++;
  }
  allMeters = allMetersTemp;

  Serial.begin(115200);
  Serial.println("Starting NimBLE Client");
  NimBLEDevice::init("");
  NimBLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM |*/ BLE_SM_PAIR_AUTHREQ_SC);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  //NimBLEDevice::setScanFilterMode(2);
  pScan = NimBLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  pScan->setInterval(45);
  pScan->setWindow(15);
  pScan->setDuplicateFilter(true);
  pScan->setActiveScan(true);
}

void rescan(int seconds) {
  lastRescan = millis();
  while (pScan->isScanning()) {
    delay(50);
  }
  allSwitchbotsDev = {};
  pScan->clearResults();
  lastRescan = millis();
  isRescanning = true;
  delay(50);
  client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"scanning\"}");
  delay(50);
  if (ledOnScan) {
    digitalWrite(LED_PIN, ledONValue);
  }
  pScan->start(seconds, rescanEndedCB);
}

void rescanFind(std::string aMac) {
  if (isRescanning) {
    return;
  }
  while (pScan->isScanning()) {
    delay(50);
  }
  allSwitchbotsDev.erase(aMac);
  pScan->erase(NimBLEAddress(aMac));
  delay(100);
  client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"scanning\"}");
  delay(50);
  if (ledOnScan) {
    digitalWrite(LED_PIN, ledONValue);
  }
  pScan->start(infoScanTime, scanEndedCB, true);
}

void getAllBotSettings() {
  if (client.isConnected() && initialScanComplete) {
    if (ledOnBootScan) {
      digitalWrite(LED_PIN, ledONValue);
    }

    Serial.println("In all get bot settings...");
    gotSettings = true;
    std::map<std::string, std::string>::iterator itT = allBots.begin();
    std::string aDevice;
    while (itT != allBots.end())
    {
      aDevice = itT->first;
      processing = true;
      controlMQTT(aDevice, "REQUESTSETTINGS");
      processing = false;
      itT++;
    }
    if (ledOnBootScan) {
      digitalWrite(LED_PIN, ledOFFValue);
    }
    Serial.println("Added all get bot settings...");
  }
}

void loop () {
  server.handleClient();
  client.loop();

  publishLastwillOnline();

  if (isRescanning) {
    lastRescan = millis();
  }
  if (!processing && !(pScan->isScanning()) && !isRescanning) {
    if (getSettingsOnBoot && !gotSettings ) {
      getAllBotSettings();
    }
    if (processQueue()) {
      if (autoRescan) {
        recurringRescan();
      }
      if (scanAfterControl) {
        recurringScan();
      }
    }
  }
  delay(1);
}

void recurringRescan() {
  if (isRescanning) {
    lastRescan = millis();
    return;
  }
  if ((millis() - lastRescan) >= (rescanTime * 1000)) {
    if (!processing && !(pScan->isScanning()) && !isRescanning) {
      rescan(initialScan);
    }
    else {
      long tempVal = (millis() - ((rescanTime * 1000) - 5000));
      if (tempVal < 0) {
        tempVal = 0;
      }
      lastRescan = tempVal;
    }
  }
}

void recurringScan() {
  if ((millis() - lastScanCheck) >= 200) {
    if (!rescanTimes.empty()) {
      std::map<std::string, long>::iterator it = rescanTimes.begin();
      std::map<std::string, std::string>::iterator itB;
      std::map<std::string, int>::iterator itS;

      std::string anAddr = it->first;
      while (it != rescanTimes.end())
      {
        itB = allSwitchbotsOpp.find(anAddr);
        itS = botScanTime.find(itB->second);
        long lastTime = it->second;
        int aDefault = 10;
        long scanTime = aDefault; //default if not in list
        if (itS != botScanTime.end())
        {
          scanTime = itS->second;
        }
        std::map<std::string, bool>::iterator itP = botsInPressMode.find(anAddr);
        if (itP != botsInPressMode.end())
        {
          std::map<std::string, int>::iterator itH = botHoldSecs.find(anAddr);
          if (itH != botHoldSecs.end())
          {
            int holdTimePlus = (itH->second) + aDefault;
            if (holdTimePlus > scanTime) {
              scanTime =  holdTimePlus;
            }
          }
        }
        if ((millis() - lastTime) >= (scanTime * 1000)) {
          if (!processing && !(pScan->isScanning()) && !isRescanning) {
            rescanFind(it->first);
            rescanTimes.erase(it->first);
            delay(100);
            it = rescanTimes.begin();
          }
        }
        else {
          it++;
        }
      }
    }
    lastScanCheck = millis();
  }
}


void processRequest(std::string macAdd, std::string aName, const char * command, std::string deviceTopic) {
  int count = 1;
  std::map<std::string, NimBLEAdvertisedDevice*>::iterator itS = allSwitchbotsDev.find(macAdd);
  NimBLEAdvertisedDevice* advDevice = nullptr;
  if (itS != allSwitchbotsDev.end())
  {
    advDevice =  itS->second;
  }
  bool shouldContinue = (advDevice == nullptr);
  while (shouldContinue) {
    if (count > 3) {
      shouldContinue = false;
    }
    else {
      if (pScan->isScanning()) {
        while (pScan->isScanning()) {
          delay(10);
        }
      }
      if (ledOnScan) {
        digitalWrite(LED_PIN, ledONValue);
      }
      pScan->start(10 * count, scanEndedCB, true);
      delay(500);
      while (pScan->isScanning()) {
        delay(10);
      }
      itS = allSwitchbotsDev.find(macAdd);
      if (itS != allSwitchbotsDev.end())
      {
        advDevice =  itS->second;
      }
      shouldContinue = (advDevice == nullptr);
      count++;
    }
  }
  if (advDevice == nullptr)
  {
    StaticJsonDocument<100> doc;
    char aBuffer[100];
    doc["id"] = aName.c_str();
    doc["status"] = "errorLocatingDevice";
    serializeJson(doc, aBuffer);
    client.publish((deviceTopic + "/status").c_str(), aBuffer);
  }
  else {
    sendToDevice(advDevice, aName, command, deviceTopic);
  }
}

bool waitToProcess(QueueCommand aCommand) {
  bool wait = false;
  long waitTimeLeft = 0;
  if (waitBetweenControl) {
    if (aCommand.payload != "REQUESTINFO" && aCommand.payload != "GETINFO" && (aCommand.topic != (ESPMQTTTopic + "/requestInfo"))) {
      std::string anAddr;
      std::map<std::string, std::string>::iterator itY = allSwitchbots.find(aCommand.device.c_str());
      if (itY != allSwitchbots.end())
      {
        anAddr = itY->second;
        std::map<std::string, long>::iterator itZ = lastCommandSent.find(anAddr);
        if (itZ != lastCommandSent.end())
        {
          wait = true;
          long lastTime = itZ->second;
          int waitForSecs = 0;
          std::string deviceName;
          std::map<std::string, std::string>::iterator itK = deviceTypes.find(anAddr);
          if (itK != deviceTypes.end())
          {
            deviceName = itK->second.c_str();
          }
          if (deviceName == botName) {
            waitForSecs = defaultBotWaitTime;
          }
          else if (deviceName == curtainName) {
            waitForSecs = defaultCurtainWaitTime;
          }

          std::map<std::string, bool>::iterator itP = botsInPressMode.find(anAddr);
          if (itP != botsInPressMode.end())
          {
            std::map<std::string, int>::iterator itH = botHoldSecs.find(anAddr);
            if (itH != botHoldSecs.end())
            {
              int holdTimePlus = (itH->second) + defaultBotWaitTime;
              if (holdTimePlus > waitForSecs) {
                waitForSecs =  holdTimePlus;
              }
            }
          }
          std::map<std::string, int>::iterator itV = botWaitBetweenControlTimes.find(aCommand.device.c_str());
          if (itV != botWaitBetweenControlTimes.end())
          {
            int waitTimePlus = (itV->second);
            if (waitTimePlus > waitForSecs) {
              waitForSecs =  waitTimePlus;
            }
          }
          if ((millis() - lastTime) >= (waitForSecs * 1000)) {
            wait = false;

          }
          else {
            waitTimeLeft = (waitForSecs * 1000) - (millis() - lastTime);
          }
        }
      }
    }
  }
  if (wait) {
    Serial.print("Control for device: ");
    Serial.print(aCommand.device.c_str());
    Serial.print(" will wait ");
    Serial.print(waitTimeLeft);
    Serial.println(" millisecondSeconds");
  }
  return wait;
}

bool processQueue() {
  processing = true;
  struct QueueCommand aCommand;
  while (!commandQueue.isEmpty()) {
    if (ledOnCommand) {
      digitalWrite(LED_PIN, ledONValue);
    }
    if (!waitForResponse) {
      currentRetry++;
      bool skipDequeue = false;
      bool requeue = false;
      bool skip = false;
      aCommand = commandQueue.getHead();
      bool getSettingsAfter = false;
      std::string requestDevice = aCommand.device.c_str();
      if ((aCommand.topic == ESPMQTTTopic + "/rescan") && isRescanning) {
        commandQueue.dequeue();
      }
      else {
        if ( pScan->isScanning() || isRescanning ) {
          return false;
        }

        processing = true;
        Serial.print("Received something on ");
        Serial.println(aCommand.topic.c_str());
        Serial.println(aCommand.device.c_str());
        if (aCommand.topic == ESPMQTTTopic + "/control") {
          processing = true;
          if (isBotDevice(aCommand.device.c_str()))
          {
            if ((strcmp(aCommand.payload.c_str(), "OFF") == 0)) {
              std::map<std::string, std::string>::iterator itN = allBots.find(aCommand.device);
              std::string anAddr = itN->second;
              std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
              std::map<std::string, bool>::iterator itP = botsInPressMode.find(anAddr);
              if (itP != botsInPressMode.end())
              {
                skip = true;
              }
            }
          }
          if (!skip) {
            if (waitToProcess(aCommand)) {
              commandQueue.enqueue(aCommand);
            }
            else {
              if (ledOnCommand) {
                digitalWrite(LED_PIN, ledONValue);
              }
              noResponse = true;
              bool shouldContinue = true;
              long timeSent = millis();
              controlMQTT(aCommand.device, aCommand.payload);
              if (isBotDevice(aCommand.device.c_str()))
              {
                if (getBotResponse) {
                  while (noResponse && shouldContinue )
                  {
                    waitForResponse = true;
                    //Serial.println("waiting for response...");
                    if ((millis() - timeSent) > (waitForResponseSec * 1000)) {
                      shouldContinue = false;
                    }
                  }
                }
                waitForResponse = false;
                bool isNum = is_number(aCommand.payload.c_str());
                if (isNum && !lastCommandWasBusy && getBotResponse) {
                  getSettingsAfter = true;
                }
                if (lastCommandWasBusy && retryBotOnBusy) {
                  requeue = true;
                  lastCommandWasBusy = false;
                  getSettingsAfter = false;
                }

                else if ((retryBotActionNoResponse && noResponse && (currentRetry <= noResponseRetryAmount)) || (retryBotSetNoResponse && noResponse && (currentRetry <= noResponseRetryAmount) && ((strcmp(aCommand.payload.c_str(), "REQUESTSETTINGS") == 0) || (strcmp(aCommand.payload.c_str(), "GETSETTINGS") == 0)
                         || (strcmp(aCommand.payload.c_str(), "MODEPRESS") == 0) || (strcmp(aCommand.payload.c_str(), "MODEPRESSINV") == 0) || (strcmp(aCommand.payload.c_str(), "MODESWITCH") == 0) || (strcmp(aCommand.payload.c_str(), "MODESWITCHINV") == 0) || isNum ))) {
                  Serial.print("current retry...");
                  Serial.println(currentRetry);
                  skipDequeue = true;
                  lastCommandWasBusy = false;
                  getSettingsAfter = false;
                }
              }
              else if (isCurtainDevice(aCommand.device.c_str()))
              {
                if (getCurtainResponse) {
                  while (noResponse && shouldContinue )
                  {
                    waitForResponse = true;
                    Serial.println("waiting for response...");
                    if ((millis() - timeSent) > (waitForResponseSec * 1000)) {
                      shouldContinue = false;
                    }
                  }
                }
                waitForResponse = false;
                if (lastCommandWasBusy && retryCurtainOnBusy) {
                  requeue = true;
                  lastCommandWasBusy = false;
                }
                else if (retryCurtainNoResponse && noResponse && (currentRetry <= noResponseRetryAmount)) {
                  Serial.print("current retry...");
                  Serial.println(currentRetry);
                  skipDequeue = true;
                  lastCommandWasBusy = false;
                }
              }
              noResponse = false;
            }
          }
        }
        else if (aCommand.topic == ESPMQTTTopic + "/requestInfo") {
          if (ledOnCommand) {
            digitalWrite(LED_PIN, ledONValue);
          }
          requestInfoMQTT(aCommand.payload);
        }
        else if (aCommand.topic == ESPMQTTTopic + "/rescan") {
          if (ledOnCommand) {
            digitalWrite(LED_PIN, ledONValue);
          }
          rescanMQTT(aCommand.payload);
        }
        if (requeue) {
          currentRetry = 0;
          commandQueue.enqueue(aCommand);
        }
        if (!skipDequeue) {
          currentRetry = 0;
          commandQueue.dequeue();
        }
        lastCommandWasBusy = false;
        if (getSettingsAfter && !skip) {
          processing = true;
          noResponse = true;
          bool shouldContinue = true;
          int count = 0;
          bool sendInitial = true;
          while (sendInitial || (lastCommandWasBusy && retryBotOnBusy) || retryBotSetNoResponse && noResponse && (count <= noResponseRetryAmount)) {
            sendInitial = false;
            count++;
            shouldContinue = true;
            long timeSent = millis();
            controlMQTT(requestDevice, "REQUESTSETTINGS");

            while (noResponse && shouldContinue )
            {
              waitForResponse = true;
              //Serial.println("waiting for response...");
              if ((millis() - timeSent) > (waitForResponseSec * 1000)) {
                shouldContinue = false;
              }
            }
            waitForResponse = false;
          }
        }
      }
    }
  }
  if (ledOnCommand) {
    digitalWrite(LED_PIN, ledOFFValue);
  }
  processing = false;
  return true;
}

void sendToDevice(NimBLEAdvertisedDevice * advDevice, std::string aName, const char * command, std::string deviceTopic) {

  NimBLEAdvertisedDevice* advDeviceToUse = advDevice;
  std::string addr = advDeviceToUse->getAddress().toString();
  //std::transform(addr.begin(), addr.end(), addr.begin(), ::toupper);
  addr = addr.c_str();
  std::string deviceStateTopic = deviceTopic + "/state";
  deviceTopic = deviceTopic + "/status";

  if ((advDeviceToUse != nullptr) && (advDeviceToUse != NULL))
  {
    char aBuffer[100];
    StaticJsonDocument<100> doc;
    //    doc["id"] = aName.c_str();
    if (strcmp(command, "requestInfo") == 0 || strcmp(command, "REQUESTINFO") == 0 || strcmp(command, "GETINFO") == 0) {
      bool isSuccess = requestInfo(advDeviceToUse);
      if (!isSuccess) {
        doc["status"] = "errorRequestInfo";
        serializeJson(doc, aBuffer);
        client.publish(deviceTopic.c_str(),  aBuffer);
      }
      return;
    }
    bool isConnected = false;
    int count = 0;
    bool shouldContinue = true;
    while (shouldContinue) {
      if (count > 1) {
        delay(50);
      }
      isConnected = connectToServer(advDeviceToUse);
      count++;
      if (isConnected) {
        shouldContinue = false;
        doc["status"] = "connected";
        serializeJson(doc, aBuffer);
        client.publish(deviceTopic.c_str(),  aBuffer);
      }
      else {
        if (count > tryConnecting) {
          shouldContinue = false;
          doc["status"] = "errorConnect";
          serializeJson(doc, aBuffer);
          client.publish(deviceTopic.c_str(),  aBuffer);
        }
      }
    }
    count = 0;
    if (isConnected) {
      shouldContinue = true;
      bool isSuccess;
      while (shouldContinue) {
        if (count > 1) {
          delay(50);
        }
        isSuccess = sendCommand(advDeviceToUse, command, count);
        count++;
        if (isSuccess) {
          delay(100);
          shouldContinue = false;
          if (!lastCommandSentPublished) {
            StaticJsonDocument<100> doc;
            char aBuffer[100];
            doc["status"] = "commandSent";
            serializeJson(doc, aBuffer);
            client.publish(deviceTopic.c_str(), aBuffer);
          }
          lastCommandSentPublished = false;
          if (strcmp(command, "REQUESTSETTINGS") != 0 && strcmp(command, "GETSETTINGS") != 0) {
            bool isNum = is_number(command);
            bool scanAfterNum = true;
            std::string aDevice = "";
            std::map<std::string, std::string>::iterator itI = allSwitchbotsOpp.find(addr);
            aDevice = itI->second;
            if (isNum && isBotDevice(aDevice)) {
              scanAfterNum = false;
            }
            if (isNum) {
              int aVal;
              sscanf(command, "%d", &aVal);
              if (aVal < 0) {
                aVal = 0;
              }
              else if (aVal > 100) {
                aVal = 100;
              }
              if (isCurtainDevice(aDevice)) {
                std::string devicePosTopic = deviceTopic + aDevice + "/position";
                StaticJsonDocument<50> docPos;
                char aBuffer[100];
                docPos["pos"] = aVal;
                serializeJson(docPos, aBuffer);
                client.publish(devicePosTopic.c_str(), aBuffer);
              }
            }
            else {
              std::map<std::string, bool>::iterator itP = botsInPressMode.find(addr);
              if (itP != botsInPressMode.end())
              {
                client.publish(deviceStateTopic.c_str(), "OFF");
              }
              else {
                client.publish(deviceStateTopic.c_str(), command);
              }
            }
            if (scanAfterControl && scanAfterNum) {
              std::map<std::string, long>::iterator itW = rescanTimes.find(addr);
              if (itW != rescanTimes.end())
              {
                rescanTimes.erase(addr);
              }
              rescanTimes.insert ( std::pair<std::string, long>(addr, millis()));
            }
          }
        }
        else {
          if (count > trySending) {
            shouldContinue = false;
            doc["status"] = "errorCommand";
            serializeJson(doc, aBuffer);
            client.publish(deviceTopic.c_str(),  aBuffer);
          }
        }
      }
    }
  }
  Serial.println("Done sendCommand...");
}

bool is_number(const std::string & s)
{
  std::string::const_iterator it = s.begin();
  while (it != s.end() && std::isdigit(*it)) ++it;
  return !s.empty() && it == s.end();
}

void controlMQTT(std::string device, std::string payload) {
  processing = true;
  Serial.println("Processing Control MQTT...");

  std::string deviceAddr = "";
  std::string deviceTopic;
  std::string anAddr;

  Serial.print("Device: ");
  Serial.println(device.c_str());
  Serial.print("Device value: ");
  Serial.println(payload.c_str());

  std::map<std::string, std::string>::iterator itS = allBots.find(device.c_str());
  if (itS != allBots.end())
  {
    anAddr = itS->second;
    std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
    deviceAddr = anAddr.c_str();
    deviceTopic = botTopic;
  }
  itS = allCurtains.find(device.c_str());
  if (itS != allCurtains.end())
  {
    anAddr = itS->second;
    std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
    deviceAddr = anAddr.c_str();
    deviceTopic = curtainTopic;
  }
  itS = allMeters.find(device.c_str());
  if (itS != allMeters.end())
  {
    anAddr = itS->second;
    std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
    deviceAddr = anAddr.c_str();
    deviceTopic = meterTopic;
  }

  if (deviceAddr != "") {
    bool isNum = is_number(payload.c_str());
    deviceTopic = deviceTopic + device;
    if (isNum) {
      int aVal;
      sscanf(payload.c_str(), "%d", &aVal);
      if (aVal < 0) {
        payload = "0";
      }
      else if (aVal > 100) {
        payload = "100";
      }
      processRequest(deviceAddr, device, payload.c_str(), deviceTopic);
    }
    else {
      if ((strcmp(payload.c_str(), "PRESS") == 0) || (strcmp(payload.c_str(), "ON") == 0) || (strcmp(payload.c_str(), "OFF") == 0) || (strcmp(payload.c_str(), "OPEN") == 0) || (strcmp(payload.c_str(), "CLOSE") == 0) || (strcmp(payload.c_str(), "PAUSE") == 0)
          || (strcmp(payload.c_str(), "REQUESTSETTINGS") == 0) || (strcmp(payload.c_str(), "REQUESTINFO") == 0) || (strcmp(payload.c_str(), "GETSETTINGS") == 0) || (strcmp(payload.c_str(), "GETINFO") == 0)
          || (strcmp(payload.c_str(), "MODEPRESS") == 0) || (strcmp(payload.c_str(), "MODEPRESSINV") == 0) || (strcmp(payload.c_str(), "MODESWITCH") == 0) || (strcmp(payload.c_str(), "MODESWITCHINV") == 0)) {
        processRequest(deviceAddr, device, payload.c_str(), deviceTopic);
      }
      else {
        char aBuffer[100];
        StaticJsonDocument<100> docOut;
        docOut["status"] = "errorJSONValue";
        serializeJson(docOut, aBuffer);
        Serial.println("Parsing failed = value not a valid command");
        client.publish(ESPMQTTTopic.c_str(), aBuffer);
      }
    }
  }
  else {
    char aBuffer[100];
    StaticJsonDocument<100> docOut;
    docOut["status"] = "errorJSONDevice";
    serializeJson(docOut, aBuffer);
    Serial.println("Parsing failed = device not from list");
    client.publish(ESPMQTTTopic.c_str(), aBuffer);
  }

  delay(100);
  client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"idle\"}");
}

void rescanMQTT(std::string payload) {
  isRescanning = true;
  processing = true;
  Serial.println("Processing Rescan MQTT...");
  StaticJsonDocument<100> docIn;
  deserializeJson(docIn, payload);

  if (docIn == nullptr) { //Check for errors in parsing
    Serial.println("Parsing failed");
    char aBuffer[100];
    StaticJsonDocument<100> docOut;
    docOut["status"] = "errorParsingJSON";
    serializeJson(docOut, aBuffer);
    client.publish(ESPMQTTTopic.c_str(), aBuffer);
  }
  else {
    int value = docIn["sec"];
    String secString = String(value);
    if (secString.c_str() != "") {
      bool isNum = is_number(secString.c_str());
      if (isNum) {
        int aVal;
        sscanf(secString.c_str(), "%d", &aVal);
        if (aVal < 0) {
          return;
        }
        else if (aVal > 300) {
          aVal = 300;
        }
        rescan(aVal);
      }
      else {
        char aBuffer[100];
        StaticJsonDocument<100> docOut;
        docOut["status"] = "errorJSONValue";
        serializeJson(docOut, aBuffer);
        Serial.println("Parsing failed = device not from list");
        client.publish(ESPMQTTTopic.c_str(), aBuffer);
      }
    }
  }
  processing = false;
}

void requestInfoMQTT(std::string payload) {
  processing = true;
  Serial.println("Processing Request Info MQTT...");
  StaticJsonDocument<100> docIn;
  deserializeJson(docIn, payload);

  if (docIn == nullptr) { //Check for errors in parsing
    Serial.println("Parsing failed");
    char aBuffer[100];
    StaticJsonDocument<100> docOut;
    docOut["status"] = "errorParsingJSON";
    serializeJson(docOut, aBuffer);
    client.publish(ESPMQTTTopic.c_str(), aBuffer);
  }
  else {
    const char * aName = docIn["id"]; //Get sensor type value
    Serial.print("Device: ");
    Serial.println(aName);

    std::string deviceAddr = "";
    std::string deviceTopic;
    std::string anAddr;

    if (aName != nullptr) {
      std::map<std::string, std::string>::iterator itS = allBots.find(aName);
      if (itS != allBots.end())
      {
        anAddr = itS->second;
        std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
        deviceAddr = anAddr.c_str();
        deviceTopic = botTopic;
      }
      itS = allCurtains.find(aName);
      if (itS != allCurtains.end())
      {
        anAddr = itS->second;
        std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
        deviceAddr = anAddr.c_str();
        deviceTopic = curtainTopic;
      }
      itS = allMeters.find(aName);
      if (itS != allMeters.end())
      {
        anAddr = itS->second;
        std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
        deviceAddr = anAddr.c_str();
        deviceTopic = meterTopic;
      }
    }
    if (deviceAddr != "") {
      deviceTopic = deviceTopic + aName;
      processRequest(deviceAddr, aName, "requestInfo", deviceTopic);
    }
    else {
      char aBuffer[100];
      StaticJsonDocument<100> docOut;
      docOut["status"] = "errorJSONId";
      serializeJson(docOut, aBuffer);
      Serial.println("Parsing failed = device not from list");
      client.publish(ESPMQTTTopic.c_str(), aBuffer);
    }
  }
  processing = false;
}

void onConnectionEstablished() {
  std::string anAddr;
  std::string aDevice;
  std::map<std::string, std::string>::iterator it;

  if (!deviceHasBooted) {
    deviceHasBooted = true;
    if (ledOnBootScan) {
      digitalWrite(LED_PIN, ledONValue);
    }
    client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"boot\"}");
    delay(100);
  }

  client.publish(lastWill, "online", true);

  if (!initialScanComplete) {
    client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"scanning\"}");
    pScan->start(initialScan, initialScanEndedCB, true);
  }

  it = allCurtains.begin();
  while (it != allCurtains.end())
  {
    std::string deviceStr ;
    aDevice = it->first.c_str();

    client.subscribe((curtainTopic + aDevice + "/set").c_str(), [aDevice] (const String & payload)  {
      Serial.println("Control MQTT Received...");
      if (!commandQueue.isFull()) {

        struct QueueCommand queueCommand;
        queueCommand.payload = payload.c_str();
        queueCommand.topic = ESPMQTTTopic + "/control";
        queueCommand.device = aDevice;
        commandQueue.enqueue(queueCommand);
      }
      else {
        client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"errorQueueFull\"}");
      }
    });

    it++;
  }

  it = allBots.begin();
  while (it != allBots.end())
  {
    std::string deviceStr ;
    aDevice = it->first.c_str();

    client.subscribe((botTopic + aDevice + "/set").c_str(), [aDevice] (const String & payload)  {
      Serial.println("Control MQTT Received...");
      if (!commandQueue.isFull()) {
        if (immediateBotStateUpdate && isBotDevice(aDevice)) {
          std::string deviceStateTopic = botTopic + aDevice + "/state";
          std::map<std::string, std::string>::iterator itP = allBots.find(aDevice);
          if (itP != allBots.end())
          {
            std::string aMac = itP->second.c_str();
            std::map<std::string, bool>::iterator itZ = botsInPressMode.find(aMac);
            if (itZ != botsInPressMode.end())
            {
              client.publish(deviceStateTopic.c_str(), "OFF");
            }
            else {
              if ((strcmp(payload.c_str(), "OFF") == 0)) {
                client.publish(deviceStateTopic.c_str(), "OFF");
              } else if ((strcmp(payload.c_str(), "ON") == 0)) {
                client.publish(deviceStateTopic.c_str(), "ON");
              }
            }
          }
        }

        else if (immediateCurtainStateUpdate && isCurtainDevice(aDevice)) {
          std::string deviceStateTopic = curtainTopic + aDevice + "/state";
          std::string devicePosTopic = curtainTopic + aDevice + "/position";
          std::map<std::string, std::string>::iterator itP = allCurtains.find(aDevice);
          if (itP != allCurtains.end())
          {
            std::string aMac = itP->second.c_str();
            bool isNum = is_number(payload.c_str());
            if (isNum) {
              int aVal;
              sscanf(payload.c_str(), "%d", &aVal);
              if (aVal < 0) {
                aVal = 0;
              }
              else if (aVal > 100) {
                aVal = 100;
              }
              StaticJsonDocument<50> docPos;
              char aBuffer[100];
              docPos["pos"] = aVal;
              serializeJson(docPos, aBuffer);
              client.publish(devicePosTopic.c_str(), aBuffer);
            }
            else if ((strcmp(payload.c_str(), "OPEN") == 0))  {
              client.publish(deviceStateTopic.c_str(), "OPEN");
            } else if ((strcmp(payload.c_str(), "CLOSE") == 0))  {
              client.publish(deviceStateTopic.c_str(), "CLOSE");
            } else if ((strcmp(payload.c_str(), "PAUSE") == 0)) {
              client.publish(deviceStateTopic.c_str(), "PAUSE");
            }
          }
        }

        struct QueueCommand queueCommand;
        queueCommand.payload = payload.c_str();
        queueCommand.topic = ESPMQTTTopic + "/control";
        queueCommand.device = aDevice;
        commandQueue.enqueue(queueCommand);
      }
      else {
        client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"errorQueueFull\"}");
      }
    });

    it++;
  }

  it = allMeters.begin();
  while (it != allMeters.end())
  {
    std::string deviceStr ;
    aDevice = it->first.c_str();

    client.subscribe((meterTopic + aDevice + "/set").c_str(), [aDevice] (const String & payload)  {
      Serial.println("Control MQTT Received...");
      if (!commandQueue.isFull()) {
        struct QueueCommand queueCommand;
        queueCommand.payload = payload.c_str();
        queueCommand.topic = ESPMQTTTopic + "/control";
        queueCommand.device = aDevice;
        commandQueue.enqueue(queueCommand);
      }
      else {
        client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"errorQueueFull\"}");
      }
    });

    it++;
  }

  client.subscribe(requestInfoTopic, [] (const String & payload)  {
    Serial.println("Request Info MQTT Received...");
    if (!commandQueue.isFull()) {
      struct QueueCommand queueCommand;
      queueCommand.payload = payload.c_str();
      queueCommand.topic = ESPMQTTTopic + "/requestInfo";
      commandQueue.enqueue(queueCommand);
    }
    else {
      client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"errorQueueFull\"}");
    }
  });

  client.subscribe(requestSettingsTopic, [] (const String & payload)  {
    Serial.println("Request Settings MQTT Received...");
    if (!commandQueue.isFull()) {
      StaticJsonDocument<100> docIn;
      deserializeJson(docIn, payload.c_str());
      const char * aDevice = docIn["id"];
      struct QueueCommand queueCommand;
      queueCommand.payload = "REQUESTSETTINGS";
      queueCommand.topic = ESPMQTTTopic + "/control";
      queueCommand.device = aDevice;
      commandQueue.enqueue(queueCommand);
    }
    else {
      client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"errorQueueFull\"}");
    }
  });

  client.subscribe(setModeTopic, [] (const String & payload)  {
    Serial.println("setMode  MQTT Received...");
    if (!commandQueue.isFull()) {
      StaticJsonDocument<100> docIn;
      deserializeJson(docIn, payload.c_str());
      const char * aDevice = docIn["id"];
      const char * aMode = docIn["mode"];
      struct QueueCommand queueCommand;
      queueCommand.payload = aMode;
      queueCommand.topic = ESPMQTTTopic + "/control";
      queueCommand.device = aDevice;
      commandQueue.enqueue(queueCommand);
    }
    else {
      client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"errorQueueFull\"}");
    }
  });

  client.subscribe(setHoldTopic, [] (const String & payload)  {
    Serial.println("setHold MQTT Received...");
    if (!commandQueue.isFull()) {
      StaticJsonDocument<100> docIn;
      deserializeJson(docIn, payload.c_str());
      const char * aDevice = docIn["id"];
      int aHold = docIn["hold"];
      String holdString = String(aHold);
      struct QueueCommand queueCommand;
      queueCommand.payload = holdString.c_str();
      queueCommand.topic = ESPMQTTTopic + "/control";
      queueCommand.device = aDevice;
      commandQueue.enqueue(queueCommand);
    }
    else {
      client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"errorQueueFull\"}");
    }
  });

  client.subscribe(rescanTopic, [] (const String & payload)  {
    Serial.println("Rescan MQTT Received...");
    if (!commandQueue.isFull()) {
      struct QueueCommand queueCommand;
      queueCommand.payload = payload.c_str();
      queueCommand.topic = ESPMQTTTopic + "/rescan";
      commandQueue.enqueue(queueCommand);
    }
    else {
      client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"errorQueueFull\"}");
    }
  });
}

bool connectToServer(NimBLEAdvertisedDevice * advDeviceToUse) {
  Serial.println("Try to connect. Try a reconnect first...");
  NimBLEClient* pClient = nullptr;
  if (NimBLEDevice::getClientListSize()) {

    pClient = NimBLEDevice::getClientByPeerAddress(advDeviceToUse->getAddress());
    if (pClient) {
      if (!pClient->connect(advDeviceToUse, false)) {
        Serial.println("Reconnect failed");
      }
      else {
        Serial.println("Reconnected client");
      }
    }
    else {
      pClient = NimBLEDevice::getDisconnectedClient();
    }
  }
  if (!pClient) {
    if (NimBLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS) {
      Serial.println("Max clients reached - no more connections available");
      return false;
    }
    pClient = NimBLEDevice::createClient();
    Serial.println("New client created");
    pClient->setClientCallbacks(&clientCB, false);
    pClient->setConnectionParams(12, 12, 0, 51);
    pClient->setConnectTimeout(10);

  }
  if (!pClient->isConnected()) {
    if (!pClient->connect(advDeviceToUse)) {
      NimBLEDevice::deleteClient(pClient);
      Serial.println("Failed to connect, deleted client");
      return false;
    }
  }
  Serial.print("Connected to: ");
  Serial.println(pClient->getPeerAddress().toString().c_str());
  Serial.print("RSSI: ");
  Serial.println(pClient->getRssi());
  return true;
}


bool sendCommandBytesNoResponse(NimBLERemoteCharacteristic * pChr, byte * bArray, int aSize ) {
  if (pChr == nullptr) {
    return false;
  }
  return pChr->writeValue(bArray, aSize, false);
}

bool sendCommandBytesWithResponse(NimBLERemoteCharacteristic * pChr, byte * bArray, int aSize ) {
  if (pChr == nullptr) {
    return false;
  }
  return pChr->writeValue(bArray, aSize, true);
}

bool sendCurtainCommandBytes(NimBLERemoteCharacteristic * pChr, byte * bArray, int aSize ) {
  if (pChr == nullptr) {
    return false;
  }
  if (getCurtainResponse) {
    return sendCommandBytesWithResponse(pChr, bArray, aSize);
  }
  else {
    return sendCommandBytesNoResponse(pChr, bArray, aSize);
  }
}

bool sendBotCommandBytes(NimBLERemoteCharacteristic * pChr, byte * bArray, int aSize ) {
  if (pChr == nullptr) {
    return false;
  }
  if (getBotResponse) {
    return sendCommandBytesWithResponse(pChr, bArray, aSize);
  }
  else {
    return sendCommandBytesNoResponse(pChr, bArray, aSize);
  }
}

bool isBotDevice (std::string aDevice) {
  std::map<std::string, std::string>::iterator itS = allBots.find(aDevice);
  if (itS != allBots.end())
  {
    return true;
  }
  return false;
}

bool isCurtainDevice (std::string aDevice) {
  std::map<std::string, std::string>::iterator itS = allCurtains.find(aDevice);
  if (itS != allCurtains.end())
  {
    return true;
  }
  return false;
}

bool sendCommand(NimBLEAdvertisedDevice * advDeviceToUse, const char * type, int attempts) {
  if (advDeviceToUse == nullptr) {
    return false;
  }
  Serial.println("Sending command...");
  std::string anAddr = advDeviceToUse->getAddress();
  if (!NimBLEDevice::getClientListSize()) {
    return false;
  }
  NimBLEClient* pClient = NimBLEDevice::getClientByPeerAddress(anAddr);
  if (!pClient) {
    return false;
  }

  bool tryConnect = !(pClient->isConnected());
  int count = 1;
  while (tryConnect  || !pClient ) {
    if (count > 20) {
      Serial.println("Failed to connect for sending command");
      return false;
    }
    count++;
    Serial.println("Attempt to send command. Not connecting. Try connecting...");
    tryConnect = !(connectToServer(advDeviceToUse));
    if (!tryConnect) {
      pClient = NimBLEDevice::getClientByPeerAddress(anAddr);
    }
  }
  bool returnValue = true;
  std::string aPass = "";
  std::string aDevice = "";
  std::map<std::string, std::string>::iterator itU = allSwitchbotsOpp.find(anAddr);
  if (itU != allSwitchbotsOpp.end())
  {
    aDevice = itU->second.c_str();
    aPass = getPass(aDevice);
  }
  if (isBotDevice(aDevice))
  {
    if (getBotResponse) {
      returnValue = subscribeToNotify(advDeviceToUse);
    }
  }
  else if (isCurtainDevice(aDevice))
  {
    if (getCurtainResponse) {
      returnValue = subscribeToNotify(advDeviceToUse);
    }
  }
  bool skipWaitAfter = false;
  if (returnValue) {
    NimBLERemoteService* pSvc = nullptr;
    NimBLERemoteCharacteristic* pChr = nullptr;
    pSvc = pClient->getService("cba20d00-224d-11e6-9fb8-0002a5d5c51b");
    if (pSvc) {
      pChr = pSvc->getCharacteristic("cba20002-224d-11e6-9fb8-0002a5d5c51b");
    }
    if (pChr) {
      if (pChr->canWrite()) {
        bool wasSuccess = false;
        bool isNum = is_number(type);
        uint8_t aPassCRC[4];
        if (aPass != "") {
          uint32_t aCRC = getPassCRC(aPass);
          for (int i = 0; i < 4; ++i)
          {
            aPassCRC[i] = ((uint8_t*)&aCRC)[3 - i];
          }
        }
        if (isNum) {
          int aVal;
          sscanf(type, "%d", &aVal);
          if (isBotDevice(aDevice)) {
            skipWaitAfter = true;
            if (aPass == "") {
              Serial.println("Num is for a bot device - no pass");
              byte anArray[4];
              for (int i = 0; i < 4; i++) {
                if (i == 3) {
                  anArray[i] = aVal;
                }
                else {
                  anArray[i] = bArrayHoldSecs[i];
                }
              }
              wasSuccess = sendBotCommandBytes(pChr, anArray, 4);
            }
            else {
              byte anArray[8];
              for (int i = 0; i < 8; i++) {
                if ((i >= 2) && (i <= 5)) {
                  anArray[i] = aPassCRC[i - 2];
                }
                else if (i == 7) {
                  anArray[i] = aVal;
                }
                else {
                  anArray[i] = bArrayHoldSecsPass[i];
                }
              }
              wasSuccess = sendBotCommandBytes(pChr, anArray , 8);
            }
          }
          else if (isCurtainDevice(aDevice)) {
            byte anArray[7];
            for (int i = 0; i < 7; i++) {
              if (i == 6) {
                anArray[i] = (100 - aVal);
              }
              else {
                anArray[i] = bArrayPos[i];
              }
            }
            wasSuccess = sendCurtainCommandBytes(pChr, anArray , 7);
          }
        }
        else {
          if (strcmp(type, "PRESS") == 0) {
            if (aPass == "") {
              wasSuccess = sendBotCommandBytes(pChr, bArrayPress, 2);
            }
            else {
              byte anArray[6];
              for (int i = 0; i < 6; i++) {
                if ((i >= 2) && (i <= 5)) {
                  anArray[i] = aPassCRC[i - 2];
                }
                else {
                  anArray[i] = bArrayPressPass[i];
                }
              }
              wasSuccess = sendBotCommandBytes(pChr, anArray , 6);
            }
          }
          else if (strcmp(type, "ON") == 0) {
            if (aPass == "") {
              wasSuccess = sendBotCommandBytes(pChr, bArrayOn, 3);
            }
            else {
              byte anArray[7];
              for (int i = 0; i < 7; i++) {
                if (i >= 2 &&  i <= 5) {
                  anArray[i] = aPassCRC[i - 2];
                }
                else {
                  anArray[i] = bArrayOnPass[i];
                }
              }
              wasSuccess = sendBotCommandBytes(pChr, anArray , 7);
            }
          }
          else if (strcmp(type, "OFF") == 0) {
            if (aPass == "") {
              wasSuccess = sendBotCommandBytes(pChr, bArrayOff, 3);
            }
            else {
              byte anArray[7];
              for (int i = 0; i < 7; i++) {
                if (i >= 2 &&  i <= 5) {
                  anArray[i] = aPassCRC[i - 2];
                }
                else {
                  anArray[i] = bArrayOffPass[i];
                }
              }
              wasSuccess = sendBotCommandBytes(pChr, anArray , 7);
            }
          }
          else if (strcmp(type, "OPEN") == 0) {
            wasSuccess = sendCurtainCommandBytes(pChr, bArrayOpen, 7);
          }
          else if (strcmp(type, "CLOSE") == 0) {
            wasSuccess = sendCurtainCommandBytes(pChr, bArrayClose, 7);
          }
          else if (strcmp(type, "PAUSE") == 0) {
            wasSuccess = sendCurtainCommandBytes(pChr, bArrayPause, 6);
          }
          else if (strcmp(type, "GETSETTINGS") == 0 || strcmp(type, "REQUESTSETTINGS") == 0) {
            skipWaitAfter = true;
            if (aPass == "") {
              wasSuccess = sendBotCommandBytes(pChr, bArrayGetSettings, 2);
            }
            else {
              byte anArray[6];
              for (int i = 0; i < 6; i++) {
                if ((i >= 2) && (i <= 5)) {
                  anArray[i] = aPassCRC[i - 2];
                }
                else {
                  anArray[i] = bArrayGetSettingsPass[i];
                }
              }
              wasSuccess = sendBotCommandBytes(pChr, anArray , 6);
            }
          }
          else if (strcmp(type, "MODEPRESS") == 0) {
            if (aPass == "") {
              byte anArray[4];
              for (int i = 0; i < 4; i++) {
                if (i == 3) {
                  anArray[i] = 0x00;
                }
                else {
                  anArray[i] = bArrayBotMode[i];
                }
              }
              wasSuccess = sendBotCommandBytes(pChr, anArray , 4);
            }
            else {
              byte anArray[8];
              for (int i = 0; i < 8; i++) {
                if ((i >= 2) && (i <= 5)) {
                  anArray[i] = aPassCRC[i - 2];
                }
                else if (i == 7) {
                  anArray[i] = 0x00;
                }
                else {
                  anArray[i] = bArrayBotModePass[i];
                }
              }
              wasSuccess = sendBotCommandBytes(pChr, anArray , 8);
            }
          }
          else if (strcmp(type, "MODEPRESSINV") == 0) {
            if (aPass == "") {
              byte anArray[4];
              for (int i = 0; i < 4; i++) {
                if (i == 3) {
                  anArray[i] = 0x01;
                }
                else {
                  anArray[i] = bArrayBotMode[i];
                }
              }
              wasSuccess = sendBotCommandBytes(pChr, anArray , 4);
            }
            else {
              byte anArray[8];
              for (int i = 0; i < 8; i++) {
                if ((i >= 2) && (i <= 5)) {
                  anArray[i] = aPassCRC[i - 2];
                }
                else if (i == 7) {
                  anArray[i] = 0x01;
                }
                else {
                  anArray[i] = bArrayBotModePass[i];
                }
              }
              wasSuccess = sendBotCommandBytes(pChr, anArray , 8);
            }
          }
          else if (strcmp(type, "MODESWITCH") == 0) {
            if (aPass == "") {
              byte anArray[4];
              for (int i = 0; i < 4; i++) {
                if (i == 3) {
                  anArray[i] = 0x10;
                }
                else {
                  anArray[i] = bArrayBotMode[i];
                }
              }
              wasSuccess = sendBotCommandBytes(pChr, anArray , 4);
            }
            else {
              byte anArray[8];
              for (int i = 0; i < 8; i++) {
                if ((i >= 2) && (i <= 5)) {
                  anArray[i] = aPassCRC[i - 2];
                }
                else if (i == 7) {
                  anArray[i] = 0x10;
                }
                else {
                  anArray[i] = bArrayBotModePass[i];
                }
              }
              wasSuccess = sendBotCommandBytes(pChr, anArray , 8);
            }
          }
          else if (strcmp(type, "MODESWITCHINV") == 0) {
            if (aPass == "") {
              byte anArray[4];
              for (int i = 0; i < 4; i++) {
                if (i == 3) {
                  anArray[i] = 0x11;
                }
                else {
                  anArray[i] = bArrayBotMode[i];
                }
              }
              wasSuccess = sendBotCommandBytes(pChr, anArray , 4);
            }
            else {
              byte anArray[8];
              for (int i = 0; i < 8; i++) {
                if ((i >= 2) && (i <= 5)) {
                  anArray[i] = aPassCRC[i - 2];
                }
                else if (i == 7) {
                  anArray[i] = 0x11;
                }
                else {
                  anArray[i] = bArrayBotModePass[i];
                }
              }
              wasSuccess = sendBotCommandBytes(pChr, anArray , 8);
            }
          }
          if (wasSuccess) {
            Serial.print("Wrote new value to: ");
            Serial.println(pChr->getUUID().toString().c_str());
          }
          else {
            returnValue = false;
          }
        }
      }
      else {
        returnValue = false;
      }
    }
    else {
      Serial.println("CUSTOM write service not found.");
      returnValue = false;
    }
  }
  if (!returnValue) {
    if (attempts >= 10) {
      Serial.println("Sending failed. Disconnecting client");
      pClient->disconnect();
    } return false;
  }
  pClient->disconnect();
  Serial.println("Success! Command sent/received to/from SwitchBot");
  if (!skipWaitAfter) {
    std::map<std::string, long>::iterator itZ = lastCommandSent.find(anAddr);
    if (itZ != lastCommandSent.end())
    {
      lastCommandSent.erase(anAddr);
    }
    lastCommandSent.insert (std::pair<std::string, long>(anAddr, millis()));
  }
  return true;
}

bool getGeneric(NimBLEAdvertisedDevice * advDeviceToUse) {
  NimBLEClient* pClient = NimBLEDevice::getClientByPeerAddress(advDeviceToUse->getAddress());
  NimBLERemoteService* pSvc = nullptr;
  NimBLERemoteCharacteristic* pChr = nullptr;;

  pSvc = pClient->getService((uint16_t) 0x1800); // GENERIC ACCESS service
  if (pSvc) {    /** make sure it's not null */
    pChr = pSvc->getCharacteristic((uint16_t) 0x2a00); // DEVICE NAME characteristic
  }

  if (pChr) {    /** make sure it's not null */
    if (pChr->canRead()) {
      Serial.print(pChr->getUUID().toString().c_str());
      Serial.print(" Value: ");
      Serial.println(pChr->readValue().c_str()); // should return WoHand
      deviceTypes.insert ( std::pair<std::string, std::string>(advDeviceToUse->getAddress().toString().c_str(), pChr->readValue().c_str()) );
      return true;
    }
  }
  return false;
}

bool requestInfo(NimBLEAdvertisedDevice * advDeviceToUse) {
  if (advDeviceToUse == nullptr) {
    return false;
  }
  Serial.println("Requesting info...");
  rescanFind(advDeviceToUse->getAddress().toString().c_str());
  return true;
}

void notifyCB(NimBLERemoteCharacteristic * pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  Serial.println("notifyCB");
  noResponse = false;
  std::string aDevice;
  std::string aState;
  std::string deviceSettingsTopic;
  std::string deviceAttrTopic;
  std::string deviceStatusTopic;
  std::string deviceMac = pRemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress();
  std::map<std::string, bool>::iterator itM = discoveredDevices.find(deviceMac);
  std::map<std::string, std::string>::iterator itS = allSwitchbotsOpp.find(deviceMac);

  if (itS != allSwitchbotsOpp.end())
  {
    aDevice = itS->second.c_str();
  }
  else {
    return;
  }

  char aBuffer[200];
  std::string deviceName;
  itS = deviceTypes.find(deviceMac.c_str());
  if (itS != deviceTypes.end())
  {
    deviceName = itS->second.c_str();
  }

  Serial.printf("deviceName: %s\n", deviceName.c_str());

  if (deviceName == botName) {
    deviceStatusTopic = botTopic + aDevice + "/status";
    deviceSettingsTopic = botTopic + aDevice + "/settings";
    deviceAttrTopic = botTopic + aDevice + "/attributes";

    if (!lastCommandSentPublished) {
      StaticJsonDocument<50> statDoc;
      statDoc["status"] = "commandSent";
      serializeJson(statDoc, aBuffer);
      client.publish(deviceStatusTopic.c_str(), aBuffer);
      lastCommandSentPublished = true;
    }

    if (length == 1) {
      StaticJsonDocument<50> statDoc;
      uint8_t byte1 = pData[0];
      Serial.print("The response value from bot set mode or holdSecs: ");
      Serial.println(byte1);
      if (byte1 == 3) {
        statDoc["status"] = "busy";
        lastCommandWasBusy = true;
      }
      //SUCCESS == 1 when setting hold secs or mode
      else if (byte1 == 1) {
        statDoc["status"] = "success";
        lastCommandWasBusy = false;
      } else {
        statDoc["status"] = "failed";
        lastCommandWasBusy = false;
      }
      statDoc["value"] = byte1;
      serializeJson(statDoc, aBuffer);
      client.publish(deviceStatusTopic.c_str(), aBuffer);
    }
    if (length == 3) {
      StaticJsonDocument<50> statDoc;
      uint8_t byte1 = pData[0];
      Serial.print("The response value from bot action: ");
      Serial.println(byte1);
      if (byte1 == 3) {
        statDoc["status"] = "busy";
        lastCommandWasBusy = true;
      }
      //SUCCESS == 1 (on/off) or == 5 (press) for bot
      else if (byte1 == 1 || byte1 == 5) {
        statDoc["status"] = "success";
        lastCommandWasBusy = false;
      } else {
        statDoc["status"] = "failed";
        lastCommandWasBusy = false;
      }
      statDoc["value"] = byte1;
      serializeJson(statDoc, aBuffer);
      client.publish(deviceStatusTopic.c_str(), aBuffer);
    }
    else if (length == 13) {
      StaticJsonDocument<50> statDoc;
      statDoc["status"] = "success";
      lastCommandWasBusy = false;
      serializeJson(statDoc, aBuffer);
      client.publish(deviceStatusTopic.c_str(), aBuffer);

      /**** THESE SETTINGS ARE ALSO COLLECTED BY A SCAN SO IT IS REDUNDANT. Commented out because of RSSI. The rest works****/
      /*
            StaticJsonDocument<100> attDoc;
            std::map<std::string, NimBLEAdvertisedDevice*>::iterator itS = allSwitchbotsDev.find(deviceMac);
            NimBLEAdvertisedDevice* advDevice = nullptr;
            if (itS != allSwitchbotsDev.end())
            {
              advDevice =  itS->second;
            }

            //RSSI doesn't always work for some reason here. It can return 0
            attDoc["rssi"] = advDevice->getRSSI();

            uint8_t byte1 = pData[0];
            uint8_t byte2 = pData[1];

            bool isSwitch = bitRead(pData[9], 4);
            std::string aMode = isSwitch ? "Switch" : "Press"; // Whether the light switch Add-on is used or not
            if (isSwitch) {
              std::map<std::string, bool>::iterator itP = botsInPressMode.find(deviceMac);
              if (itP != botsInPressMode.end())
              {
                botsInPressMode.erase(deviceMac);
              }
              aState = (byte1 & 0b01000000) ? "OFF" : "ON"; // Mine is opposite, not sure why
            }
            else {
              botsInPressMode.insert (std::pair<std::string, bool>(deviceMac, true));
              aState = "OFF";
            }
            int battLevel = byte2 & 0b01111111; // %

            attDoc["mode"] = aMode;
            attDoc["state"] = aState;
            attDoc["batt"] = battLevel;
            serializeJson(attDoc, aBuffer);
            client.publish(deviceAttrTopic.c_str(), aBuffer, true);
            client.publish(deviceStateTopic.c_str(), aState.c_str(), true);*/
      /***************************************/
      StaticJsonDocument<100> settDoc;
      float fwVersion = pData[2] / 10.0;
      settDoc["firmware"] = serialized(String(fwVersion, 1));
      int timersNumber = pData[8];
      settDoc["timers"] = timersNumber;
      bool inverted = bitRead(pData[9], 0) ;
      settDoc["inverted"] = inverted;
      int holdSecs = pData[10];
      settDoc["hold"] = holdSecs;

      serializeJson(settDoc, aBuffer);
      client.publish(deviceSettingsTopic.c_str(), aBuffer, true);

      std::map<std::string, int>::iterator itZ = botHoldSecs.find(deviceMac);
      if (itZ != botHoldSecs.end())
      {
        botHoldSecs.erase(deviceMac);
      }
      botHoldSecs.insert (std::pair<std::string, int>(deviceMac, holdSecs));
    }
  }

  else if (deviceName == curtainName) {
    deviceStatusTopic = curtainTopic + aDevice + "/status";
    deviceSettingsTopic = curtainTopic + aDevice + "/settings";
    deviceAttrTopic = curtainTopic + aDevice + "/attributes";

    if (!lastCommandSentPublished) {
      StaticJsonDocument<50> statDoc;
      statDoc["status"] = "commandSent";
      serializeJson(statDoc, aBuffer);
      client.publish(deviceStatusTopic.c_str(), aBuffer);
      lastCommandSentPublished = true;
    }
    if (length < 3) {
      return;
    }
    else if (length == 3) {
      StaticJsonDocument<50> statDoc;
      uint8_t byte1 = pData[0];

      Serial.print("The response value from curtain: ");
      Serial.println(byte1);
      if (byte1 == 3) {
        statDoc["status"] = "busy";
        lastCommandWasBusy = true;
      }
      //SUCCESS == 1 or == 5 for curtain ????? just assuming based on bot
      else if (byte1 == 1 || byte1 == 5) {
        statDoc["status"] = "success";
        lastCommandWasBusy = false;
      } else {
        statDoc["status"] = "failed";
        lastCommandWasBusy = false;
      }
      statDoc["value"] = byte1;
      serializeJson(statDoc, aBuffer);
      client.publish(deviceStatusTopic.c_str(), aBuffer);
    }
  }

  NimBLEClient* pClient = nullptr;
  if (NimBLEDevice::getClientListSize()) {
    pClient = NimBLEDevice::getClientByPeerAddress(deviceMac);
    if (pClient) {
      if (!pClient->isConnected()) {
        unsubscribeToNotify(pClient);
        pClient->disconnect();
      }
    }
  }
}
