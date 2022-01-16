/** SwitchBot-MQTT-BLE-ESP32:

  https://github.com/devWaves/SwitchBot-MQTT-BLE-ESP32

  **does not use/require switchbot hub

  Code can be installed using Arduino IDE OR using Visual Studio Code PlatformIO
  	-For Arduino IDE - Use only the SwitchBot-BLE2MQTT-ESP32.ino file
	-For Visual Studio Code PlatformIO - Use the src/SwitchBot-BLE2MQTT-ESP32.cpp and platformio.ini files
  Allows for "unlimited" switchbots devices to be controlled via MQTT sent to ESP32. ESP32 will send BLE commands to switchbots and return MQTT responses to the broker
     ** I do not know where performance will be affected by number of devices **
     ** This is an unofficial SwitchBot integration. User takes full responsibility with the use of this code **

  v6.6

    Created: on Jan 16 2022
        Author: devWaves

        Contributions from:
                HardcoreWR

  based off of the work from https://github.com/combatistor/ESP32_BLE_Gateway

  Notes:
    - Supports Home Assistant MQTT Discovery

    - Support bots, curtains, temp meters, contact sensors, and motion sensors

    - It works for button press/on/off, set mode, set hold seconds

    - It works for curtain open/close/pause/position(%)

    - It can request status values (bots/curtain/meter/motion/contact: battery, mode, state, position, temp etc) using a "rescan" for all devices

    - It can request individual device status values (bots/curtain/meter/motion/contact: battery, mode, state, position, temp etc) using a "requestInfo"

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

    - holdPress = set bot hold value, then call press (without disconnecting in between)

    - Set/Control is prioritized over scanning. While scanning, if a set/control command is received scanning is stopped and resumed later

    - ESP32 can simulate ON/OFF for devices when bot is in PRESS mode. (Cannot guarantee it will always be accurate)

  - If you only have bots/curtain/meters the ESP32 will only scan when needed and requested. If you include motion or contact sensors the ESP32 will scan all the time


  <ESPMQTTTopic> = <mqtt_main_topic>/<host>
      - Default = switchbot/esp32

  ESP32 will subscribe to MQTT 'set' topic for every configure device.
      - <ESPMQTTTopic>/bot/<name>/set
      - <ESPMQTTTopic>/curtain/<name>/set
      - <ESPMQTTTopic>/meter/<name>/set
      - <ESPMQTTTopic>/contact/<name>/set
      - <ESPMQTTTopic>/motion/<name>/set

    Send a payload to the 'set' topic of the device you want to control
      Strings:
        - "PRESS"
        - "ON"
        - "OFF"
        - "OPEN"
        - "CLOSE"
        - "PAUSE"
        - "STATEOFF"    (Only for bots in simulated ON/OFF mode)
        - "STATEON"     (Only for bots in simulated ON/OFF mode)

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
                          - {"status":"connected", "command":"ON"}
                          - {"status":"errorConnect", "command":"ON"}
                          - {"status":"errorCommand", "command":"NOTVALID"}
                          - {"status":"commandSent", "command":"ON"}
                          - {"status":"busy", "value":3, "command":"ON"}
                          - {"status":"failed", "value":9, "command":"ON"}
                          - {"status":"success", "value":1, "command":"ON"}
                          - {"status":"success", "value":5, "command":"PRESS"}
                          - {"status":"success", "command":"REQUESTINFO"}

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
                          - {"pos":0}
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

      send a JSON payload of the device you want to requestInfo
          example payloads =
            {"id":"switchbotone"}

                      ESP32 will respond with MQTT on
                        - <ESPMQTTTopic>/#

                        Example attribute responses per device are detected:
                          - <ESPMQTTTopic>/bot/<name>/attributes
                          - <ESPMQTTTopic>/curtain/<name>/attributes
                          - <ESPMQTTTopic>/meter/<name>/attributes
                          - <ESPMQTTTopic>/contact/<name>/attributes
                          - <ESPMQTTTopic>/motion/<name>/attributes

                        Example response payloads:
                          - {"rssi":-78,"mode":"Press","state":"OFF","batt":94}
                          - {"rssi":-66,"calib":true,"batt":55,"pos":50,"state":"open","light":1}
                          - {"rssi":-66,"scale":"c","batt":55,"C":"21.5","F":"70.7","hum":"65"}
                          - {"rssi":-77,"batt":89,"motion":"NO MOTION","led":"OFF","sensedistance":"LONG","light":"DARK"}
                          - {"rssi":-76,"batt":91,"motion":"NO MOTION","contact":"CLOSED","light":"DARK","incount":1,"outcount":3,"buttoncount":4}

                        Example attribute responses per device are detected:
                          - <ESPMQTTTopic>/bot/<name>/state
                          - <ESPMQTTTopic>/curtain/<name>/state
                          - <ESPMQTTTopic>/meter/<name>/state
                          - <ESPMQTTTopic>/contact/<name>/state            (contact sensor has motion and contact. state = contact)
                          - <ESPMQTTTopic>/motion/<name>/state

                        Example payload:
                          - "ON"
                          - "OFF"
                          - "OPEN"
                          - "CLOSE"

                        ESP32 will respond with MQTT on 'position' topic for every configured device
                        - <ESPMQTTTopic>/curtain/<name>/position

                        Example response payload:
                          - {"pos":0}
                          - {"pos":100}
                          - {"pos":50}

                        Example topic responses specific to motion/contact sensors:
                          - <ESPMQTTTopic>/motion/<name>/motion           Example response payload: "MOTION", "NO MOTION"
                          - <ESPMQTTTopic>/motion/<name>/illuminance        Example response payload: "LIGHT", "DARK"
                          - <ESPMQTTTopic>/contact/<name>/contact         Example response payload: "OPEN", "CLOSED"
                          - <ESPMQTTTopic>/contact/<name>/motion          Example response payload: "MOTION", "NO MOTION"
                          - <ESPMQTTTopic>/contact/<name>/illuminance       Example response payload: "LIGHT", "DARK"
                          - <ESPMQTTTopic>/contact/<name>/in            Example response payload: "IDLE", "ENTERED"
                          - <ESPMQTTTopic>/contact/<name>/out           Example response payload: "IDLE", "EXITED"
                          - <ESPMQTTTopic>/contact/<name>/button                    Example response payload: "IDLE", "PUSHED"

                        Note:   You can use the button on the contact sensor to trigger other non-switchbot devices from your smarthub
                                When <ESPMQTTTopic>/contact/<name>/button = "PUSHED"


  // REQUESTSETTINGS WORKS FOR BOT ONLY - DOCUMENTATION NOT AVAILABLE ONLINE FOR CURTAIN
  ESP32 will Subscribe to MQTT topic for device settings information (requires getBotResponse = true)
      - <ESPMQTTTopic>/requestSettings

    send a JSON payload of the device you want to requestSettings
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

    send a JSON payload of the device you want to set hold
          example payloads =
            {"id":"switchbotone", "hold":5}
            {"id":"switchbotone", "hold":"5"}

    ESP32 will respond with MQTT on
    - <ESPMQTTTopic>/#

                      ESP32 will respond with MQTT on 'status' topic for every configured device
                        - <ESPMQTTTopic>/bot/<name>/status

                        Example reponses:
                          - <ESPMQTTTopic>/bot/<name>/status

                        Example payload:
                          - {"status":"connected", "command":"5"}
                          - {"status":"errorConnect", "command":"5"}
                          - {"status":"errorCommand", "command":"NOTVALID"}
                          - {"status":"commandSent", "command":"5"}
                          - {"status":"busy", "value":3, "command":"5"}
                          - {"status":"failed", "value":9, "command":"5"}
                          - {"status":"success", "value":1, "command":"5"}
                          - {"status":"success", "value":5, "command":"5"}
                          - {"status":"success", "command":"REQUESTSETTINGS"}

  // holdPress = set bot hold value, then call press on bot (without disconnecting in between)
  ESP32 will Subscribe to MQTT topic to action a holdPress on bots
      - <ESPMQTTTopic>/holdPress

    send a JSON payload of the device you want to set hold then press
          example payloads =
            {"id":"switchbotone", "hold":5}
            {"id":"switchbotone", "hold":"5"}

    ESP32 will respond with MQTT on
    - <ESPMQTTTopic>/#

                      ESP32 will respond with MQTT on 'status' topic for every configured device
                        - <ESPMQTTTopic>/bot/<name>/status

                        Example reponses:
                          - <ESPMQTTTopic>/bot/<name>/status

                        Example response payload:
                          - {"status":"connected", "command":"5"}
                          - {"status":"errorConnect", "command":"5"}
                          - {"status":"errorCommand", "command":"NOTVALID"}
                          - {"status":"commandSent", "command":"5"}
                          - {"status":"busy", "value":3, "command":"5"}
                          - {"status":"failed", "value":9, "command":"5"}
                          - {"status":"success", "value":1, "command":"5"}
                          - {"status":"success", "value":5, "command":"5"}
                          - {"status":"success", "command":"REQUESTSETTINGS"}
                          - {"status":"connected", "command":"PRESS"}
                          - {"status":"errorConnect", "command":"PRESS"}
                          - {"status":"commandSent", "command":"PRESS"}
                          - {"status":"busy", "value":3, "command":"PRESS"}
                          - {"status":"failed", "value":9, "command":"PRESS"}
                          - {"status":"success", "value":1, "command":"PRESS"}
                          - {"status":"success", "value":5, "command":"PRESS"}


  // SET MODE ON BOT
  ESP32 will Subscribe to MQTT topic setting mode for bots
      - <ESPMQTTTopic>/setMode

    send a JSON payload of the device you want to set mode
          example payloads =
            {"id":"switchbotone", "mode":"MODEPRESS"}
            {"id":"switchbotone", "mode":"MODESWITCH"}
            {"id":"switchbotone", "mode":"MODEPRESSINV"}
            {"id":"switchbotone", "mode":"MODESWITCHINV"}

                      ESP32 will respond with MQTT on 'status' topic for every configured device
                        - <ESPMQTTTopic>/bot/<name>/status

                        Example reponses:
                          - <ESPMQTTTopic>/bot/<name>/status

                        Example payload:
                          - {"status":"connected", "command":"MODEPRESS"}
                          - {"status":"errorConnect", "command":"MODEPRESS"}
                          - {"status":"errorCommand", "command":"NOTVALID"}
                          - {"status":"commandSent", "command":"MODEPRESS"}
                          - {"status":"busy", "value":3, "command":"MODEPRESS"}
                          - {"status":"failed", "value":9, "command":"MODEPRESS"}
                          - {"status":"success", "value":1, "command":"MODEPRESS"}
                          - {"status":"success", "value":5, "command":"MODEPRESS"}

  ESP32 will respond with MQTT on ESPMQTTTopic with ESP32 status
      - <ESPMQTTTopic>

      example payloads:
        {status":"idle"}
        {status":"scanning"}
        {status":"boot"}
	{status":"controlling"}
	{status":"getsettings"}

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
static const char* host = "esp32";                          //  Unique name for ESP32. The name detected by your router and MQTT. If you are using more then 1 ESPs to control different switchbots be sure to use unique hostnames. Host is the MQTT Client name and is used in MQTT topics
static const char* ssid = "SSID";                           //  WIFI SSID
static const char* password = "Password";                   //  WIFI Password

/* MQTT Settings */
/* MQTT Client name is set to WIFI host from Wifi Settings*/
static const char* mqtt_host = "192.168.0.1";                       //  MQTT Broker server ip
static const char* mqtt_user = "switchbot";                         //  MQTT Broker username. If empty or NULL, no authentication will be used
static const char* mqtt_pass = "switchbot";                         //  MQTT Broker password
static const int mqtt_port = 1883;                                  //  MQTT Port
static const std::string mqtt_main_topic = "switchbot";             //  MQTT main topic

/* Switchbot Bot Settings */
static std::map<std::string, std::string> allBots = {
  /*{ "switchbotone", "xX:xX:xX:xX:xX:xX" },
    { "switchbottwo", "yY:yY:yY:yY:yY:yY" }*/
};

/* Switchbot Curtain Settings */
static const int curtainClosedPosition = 10;    // When 2 curtains are controlled (left -> right and right -> left) it's possible one of the curtains pushes one of the switchbots more open. Change this value to set a position where a curtain is still considered closed
static std::map<std::string, std::string> allCurtains = {
  /*{ "curtainone", "xX:xX:xX:xX:xX:xX" },
    { "curtaintwo", "yY:yY:yY:yY:yY:yY" }*/
};

/* Switchbot Meter Settings */
static std::map<std::string, std::string> allMeters = {
  /*{ "meterone", "xX:xX:xX:xX:xX:xX" },
    { "metertwo", "yY:yY:yY:yY:yY:yY" }*/
};

/* Switchbot Contact Sensor Settings */
static std::map<std::string, std::string> allContactSensors = {
  /*{ "contactone", "xX:xX:xX:xX:xX:xX" },
    { "contacttwo", "yY:yY:yY:yY:yY:yY" }*/
};

/* Switchbot Motion Sensor Settings */
static std::map<std::string, std::string> allMotionSensors = {
  /*{ "motionone", "xX:xX:xX:xX:xX:xX" },
    { "motiontwo", "yY:yY:yY:yY:yY:yY" }*/
};

/* Switchbot Bot Passwords */
static std::map<std::string, std::string> allPasswords = {     // Set all the bot passwords (setup in app first). Ignore if passwords are not used
  /*{ "switchbotone", "switchbotonePassword" },
    { "switchbottwo", "switchbottwoPassword" }*/
};

/* Switchbot Bot Device Types - OPTIONAL */
/* Options include: "switch", "light", "button" */
static std::map<std::string, std::string> allBotTypes = {     // OPTIONAL - (DEFAULTS to "switch" if bot is not in list) - Will create HA entities for device types
 /* { "switchbotone", "switch" },
    { "switchbottwo", "light" },
    { "switchbotthree", "button" }*/
};

      /*** Bots in PRESS mode to simulate ON/OFF - ESP32 will try to keep track of the ON/OFF state of your device while in PRESS mode***/
      // Add bots while in PRESS mode that will simulate ON/OFF. Default state will be used if no MQTT retained on state topic
      // false = default state = OFF
      // true = default state = ON
      // If the state is incorrect, call set STATEOFF or STATEON
      static std::map<std::string, bool> botsSimulateONOFFinPRESSmode = {
        /*{ "switchbotone", false },
          { "switchbottwo", false }*/
      };

      //Add bots OFF hold time for simulated ON/OFF, if not in list, the current hold value will be used. Device must be in botsSimulateONOFFinPRESSmode list
      static std::map<std::string, int> botsSimulatedOFFHoldTimes = {
        /*{ "switchbotone", 0 },
          { "switchbottwo", 10 }*/
      };

      //Add bots ON hold time for simulated ON/OFF, if not in list, the current hold value will be used. Device must be in botsSimulateONOFFinPRESSmode list
      static std::map<std::string, int> botsSimulatedONHoldTimes = {
        /*{ "switchbotone", 15 },
          { "switchbottwo", 1}*/
      };
      /********************************************/


/********** ADVANCED SETTINGS - ONLY NEED TO CHANGE IF YOU WANT TO TWEAK SETTINGS **********/


/* ESP32 LED Settings */
//#define LED_BUILTIN 2                              // If your board doesn't have a defined LED_BUILTIN, uncomment this line and replace 2 with the LED pin value
#define LED_PIN LED_BUILTIN                          // If your board doesn't have a defined LED_BUILTIN (You will get a compile error), uncomment the line above
static const bool ledHighEqualsON = true;            // ESP32 board LED ON=HIGH (Default). If your ESP32 LED is turning OFF on scanning and turning ON while IDLE, then set this value to false
static const bool ledOnBootScan = true;              // Turn on LED during initial boot scan
static const bool ledOnScan = true;                  // Turn on LED while scanning (non-boot)
static const bool ledOnCommand = true;               // Turn on LED while MQTT command is processing. If scanning, LED will blink after scan completes. You may not notice it, there is no delay after scan

/* Webserver Settings */
static const bool useLoginScreen = false;            //  use a basic login popup to avoid unwanted access
static const String otaUserId = "admin";             //  user Id for OTA update. Ignore if useLoginScreen = false
static const String otaPass = "admin";               //  password for OTA update. Ignore if useLoginScreen = false
static WebServer server(80);                         //  default port 80

/* Home Assistant Settings */
static const bool home_assistant_mqtt_discovery = true;                    // Enable to publish Home Assistant MQTT Discovery config
static const std::string home_assistant_mqtt_prefix = "homeassistant";     // MQTT Home Assistant prefix
static const bool home_assistant_expose_seperate_curtain_position = true;  // When enabled, a seperate sensor will be added that will expose the curtain position. This is useful when using the Prometheus integration to graph curtain positions. The cover entity doesn't expose the position for Prometheus
static const bool home_assistant_use_opt_mode = false;                     // For bots in switch mode assume on/off right away. Optimistic mode. (Icon will change in HA). If devices were already configured in HA, you need to delete them and reboot esp32

/* Switchbot General Settings */
static const int tryConnecting = 60;                  // How many times to try connecting to bot
static const int trySending = 30;                     // How many times to try sending command to bot
static const int initialScan = 120;                   // How many seconds to scan for bots on ESP reboot and autoRescan. Once all devices are found scan stops, so you can set this to a big number
static const int infoScanTime = 60;                   // How many seconds to scan for single device status updates
static const int rescanTime = 600;                    // Automatically rescan for device info of all devices every X seconds (default 10 min)
static const int queueSize = 50;                      // Max number of control/requestInfo/rescan MQTT commands stored in the queue. If you send more then queueSize, they will be ignored
static const int defaultBotWaitTime = 2;              // wait at least X seconds between control command send to bots. ESP32 will detect if bot is in press mode with a hold time and will add hold time to this value per device
static const int defaultCurtainWaitTime = 0;          // wait at least X seconds between control command send to curtains
static const int waitForResponseSec = 20;             // How many seconds to wait for a bot/curtain response
static const int noResponseRetryAmount = 5;           // How many times to retry if no response received
static const int defaultScanAfterControlSecs = 10;    // Default How many seconds to wait for state/status update call after set/control command. *override with botScanTime list
static const int defaultMeterScanSecs = 60;           // Default Scan/MQTT Update for meter temp sensors every X seconds. *override with botScanTime list
static const int defaultMotionScanSecs = 60;          // Default Scan/MQTT Update for motion sensors every X seconds. *override with botScanTime list
static const int defaultContactScanSecs = 60;         // Default Scan/MQTT Update for contact temp sensors every X seconds. *override with botScanTime list
static const int waitForMQTTRetainMessages = 5;       // Only for bots in simulated ON/OFF: On boot ESP32 will look for retained MQTT state messages for X secs, otherwise default state is used
static const int missedDataResend = 120;              // If a motion or contact is somehow missed while controlling bots, send the MQTT messages within X secs of it occuring as a backup. requires sendBackupMotionContact = true

static const bool sendBackupMotionContact = true;         // Compares last contact/motion time value from switchbot contact/motion devices against what the esp32 received. If ESP32 missed one while controlling bots, it will send a motion/contact message after
static const bool autoRescan = true;                      // perform automatic rescan (uses rescanTime and initialScan).
static const bool scanAfterControl = true;                // perform requestInfo after successful control command (uses botScanTime).
static const bool waitBetweenControl = true;              // wait between commands sent to bot/curtain (avoids sending while bot is busy)
static const bool getSettingsOnBoot = true;               // Currently only works for bot (curtain documentation not available but can probably be reverse engineered easily). Get bot extra settings values like firmware, holdSecs, inverted, number of timers. ***If holdSecs is available it is used by waitBetweenControl
static const bool getBotResponse = true;                  // get a response from the bot devices. A response of "success" means the most recent command was successful. A response of "busy" means the bot was busy when the command was sent
static const bool getCurtainResponse = true;              // get a response from the curtain devices. A response of "success" means the most recent command was successful. A response of "busy" means the bot was busy when the command was sent
static const bool retryBotOnBusy = true;                  // Requires getBotResponse = true. if bot responds with busy, the last control command will retry until success
static const bool retryCurtainOnBusy = true;              // Requires getCurtainResponse = true. if curtain responds with busy, the last control command will retry until success
static const bool retryBotActionNoResponse = false;       // Retry if bot doesn't send a response. Bot default is false because no response can still mean the bot triggered.
static const bool retryBotSetNoResponse = true;           // Retry if bot doesn't send a response when requesting settings (hold, firwmare etc) or settings hold/mode
static const bool retryCurtainNoResponse = true;          // Retry if curtain doesn't send a response. Default is true. It shouldn't matter if curtain receives the same command twice (or multiple times)
static const bool immediateBotStateUpdate = true;         // ESP32 will send ON/OFF state update as soon as MQTT is received. You can set this = false if not using Home Assistant Discovery.
static const bool immediateCurtainStateUpdate = true;     // ESP32 will send OPEN/CLOSE and Position state update as soon as MQTT is received. You can set this = false if not using Home Assistant Discovery.
static const bool assumeNoResponseMeansSuccess = true;    // Only for bots in simulated ON/OFF: If the ESP32 does not receive a response after sending command (after noResponseRetryAmount reached and retryBotActionNoResponse = true) assume it worked and change state

static const bool printSerialOutputForDebugging = false;  // Only set to true when you want to debug an issue from Arduino IDE. Lots of Serial output from scanning can crash the ESP32

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

static const String versionNum = "v6.6";

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
  mqtt_host,
  (mqtt_user == NULL || strlen(mqtt_user) < 1) ? NULL : mqtt_user,
  (mqtt_user == NULL || strlen(mqtt_user) < 1) ? NULL : mqtt_pass,
  host,
  mqtt_port
);

static const uint16_t mqtt_packet_size = 1024;
static const bool home_assistant_discovery_set_up = false;
static const std::string manufacturer = "WonderLabs SwitchBot";
static const std::string curtainModel = "Curtain";
static const std::string curtainName = "WoCurtain";
static const std::string botModel = "Bot";
static const std::string botName = "WoHand";
static const std::string meterModel = "Meter";
static const std::string meterName = "WoSensorTH";
static const std::string contactModel = "Contact";
static const std::string contactName = "WoContact";
static const std::string motionModel = "Motion";
static const std::string motionName = "WoMotion";

static int ledONValue = HIGH;
static int ledOFFValue = LOW;
void scanEndedCB(NimBLEScanResults results);
void rescanEndedCB(NimBLEScanResults results);
void initialScanEndedCB(NimBLEScanResults results);
bool isBotDevice(std::string aDevice);
bool isMeterDevice(std::string aDevice);
bool isCurtainDevice(std::string aDevice);
bool isMotionDevice(std::string aDevice);
bool isContactDevice(std::string aDevice);
bool processQueue();
void startForeverScan();
void recurringMeterScan();
uint32_t getPassCRC(std::string aDevice);
bool is_number(const std::string & s);
bool controlMQTT(std::string device, std::string payload, bool disconnectAfter);
bool sendCommand(NimBLEAdvertisedDevice * advDeviceToUse, const char * type, int attempts, bool disconnectAfter);
bool sendToDevice(NimBLEAdvertisedDevice * advDevice, std::string aName, const char * command, std::string deviceTopic, bool disconnectAfter);
bool requestInfo(NimBLEAdvertisedDevice * advDeviceToUse);
bool connectToServer(NimBLEAdvertisedDevice * advDeviceToUse);
void rescanMQTT(std::string payload);
void requestInfoMQTT(std::string payload);
void recurringScan();
void recurringRescan();
void notifyCB(NimBLERemoteCharacteristic * pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
std::string getPass(std::string aDevice);
bool shouldMQTTUpdateForDevice(std::string anAddr);
static std::map<std::string, NimBLEAdvertisedDevice*> allSwitchbotsDev = {};
static std::map<std::string, NimBLEAdvertisedDevice*> allSwitchbotsScanned = {};
static std::map<std::string, long> rescanTimes = {};
static std::map<std::string, long> lastScanTimes = {};
static std::map<std::string, bool> botsSimulatedStates = {};
static std::map<std::string, std::string> motionStates = {};
static std::map<std::string, std::string> contactStates = {};
static std::map<std::string, long> lastMotions = {};
static std::map<std::string, long> lastContacts = {};
static std::map<std::string, std::string> illuminanceStates = {};
static std::map<std::string, std::string> ledStates = {};
static std::map<std::string, int> outCounts = {};
static std::map<std::string, int> entranceCounts = {};
static std::map<std::string, int> buttonCounts = {};
static std::map<std::string, std::string> lastCommandSentStrings = {};
static std::map<std::string, std::string> allSwitchbots;
static std::map<std::string, std::string> allSwitchbotsOpp;
static std::map<std::string, bool> discoveredDevices = {};
static std::map<std::string, bool> botsInPressMode = {};
static std::map<std::string, bool> botsToWaitFor = {};
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
static bool forceRescan = false;
//static bool waitForDeviceCreation = false;
static bool overrideScan = false;
static char aBuffer[200];
static const std::string ESPMQTTTopic = mqtt_main_topic + "/" + std::string(host);
static const std::string esp32Topic = ESPMQTTTopic + "/esp32";
static const std::string rssiStdStr = esp32Topic + "/rssi";
static const std::string lastWillStr = ESPMQTTTopic + "/lastwill";
static const char* lastWill = lastWillStr.c_str();
static const std::string botTopic = ESPMQTTTopic + "/bot/";
static const std::string curtainTopic = ESPMQTTTopic + "/curtain/";
static const std::string meterTopic = ESPMQTTTopic + "/meter/";
static const std::string contactTopic = ESPMQTTTopic + "/contact/";
static const std::string motionTopic = ESPMQTTTopic + "/motion/";
static const std::string rescanStdStr = ESPMQTTTopic + "/rescan";
//static std::string controlStdStr = ESPMQTTTopic + "/control";
static const std::string controlStdStr = ESPMQTTTopic + "/#/#/set";
static const std::string requestInfoStdStr = ESPMQTTTopic + "/requestInfo";
static const std::string requestSettingsStdStr = ESPMQTTTopic + "/requestSettings";
static const std::string setModeStdStr = ESPMQTTTopic + "/setMode";
static const std::string setHoldStdStr = ESPMQTTTopic + "/setHold";
static const std::string holdPressStdStr = ESPMQTTTopic + "/holdPress";
static const String rescanTopic = rescanStdStr.c_str();
static const String controlTopic = controlStdStr.c_str();
static const String requestInfoTopic = requestInfoStdStr.c_str();
static const String requestSettingsTopic = requestSettingsStdStr.c_str();
static const String setModeTopic = setModeStdStr.c_str();
static const String setHoldTopic = setHoldStdStr.c_str();
static const String holdPressTopic = holdPressStdStr.c_str();

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
  bool disconnectAfter;
  bool priority;
  int currentTry;
};

ArduinoQueue<QueueCommand> commandQueue(queueSize);

static long lastOnlinePublished = 0;
static long lastRescan = 0;
static long lastScanCheck = 0;
static bool noResponse = false;
static bool waitForResponse = false;
static std::string lastDeviceControlled = "";

void publishLastwillOnline() {
  if ((millis() - lastOnlinePublished) > 30000) {
    if (client.isConnected()) {
      client.publish(lastWill, "online", true);
      lastOnlinePublished = millis();
      String rssi = String(WiFi.RSSI());
      client.publish(rssiStdStr.c_str(), rssi.c_str());
    }
  }
}

void publishHomeAssistantDiscoveryESPConfig() {
  String wifiMAC = String(WiFi.macAddress());
  client.publish((home_assistant_mqtt_prefix + "/sensor/" + host + "/linkquality/config").c_str(), ("{\"~\":\"" + esp32Topic + "\"," +
                 + "\"name\":\"" + host + " Linkquality\"," +
                 + "\"device\": {\"identifiers\":[\"switchbotesp_" + host + "_" + wifiMAC.c_str() + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + "ESP32" + "\",\"name\": \"" + host + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbotesp_" + host + "_" + wifiMAC.c_str() + "_linkquality\"," +
                 + "\"stat_t\":\"~/rssi\"," +
                 + "\"icon\":\"mdi:signal\"," +
                 + "\"unit_of_meas\": \"rssi\"}").c_str(), true);
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
                 + "\"icon\":\"mdi:cog\"," +
                 + "\"pl_on\":true," +
                 + "\"pl_off\":false," +
                 + "\"value_template\":\"{{ value_json.inverted }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/mode/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Mode\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_mode\"," +
                 + "\"icon\":\"mdi:cog\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"value_template\":\"{{ value_json.mode }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/firmware/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Firmware\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_firmware\"," +
                 + "\"icon\":\"mdi:cog\"," +
                 + "\"stat_t\":\"~/settings\"," +
                 + "\"value_template\":\"{{ value_json.firmware }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/holdsecs/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " HoldSecs\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_holdsecs\"," +
                 + "\"icon\":\"mdi:cog\"," +
                 + "\"stat_t\":\"~/settings\"," +
                 + "\"value_template\":\"{{ value_json.hold }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/timers/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Timers\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_timers\"," +
                 + "\"icon\":\"mdi:cog\"," +
                 + "\"stat_t\":\"~/settings\"," +
                 + "\"value_template\":\"{{ value_json.timers }}\"}").c_str(), true);

  std::string optiString;
  if (optimistic) {
    optiString = "true";
  }
  else {
    optiString = "false";
  }

  std::string aType = "switch";
  std::map<std::string, std::string>::iterator itS = allBotTypes.find(deviceName.c_str());
  if (itS != allBotTypes.end())
  {
    std::string aTypeTemp = itS->second;
    std::transform(aTypeTemp.begin(), aTypeTemp.end(), aTypeTemp.begin(), to_lower());
    if (strcmp(aTypeTemp.c_str(), "light") == 0) {
      aType = "light";
    }
    else if (strcmp(aTypeTemp.c_str(), "button") == 0) {
      aType = "button";
    }
  }

  if (strcmp(aType.c_str(), "light") == 0) {
    client.publish((home_assistant_mqtt_prefix + "/light/" + deviceName + "/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\", " +
                   + "\"name\":\"" + deviceName + " Light\"," +
                   + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
                   + "\"avty_t\": \"" + lastWill + "\"," +
                   + "\"uniq_id\":\"switchbot_" + deviceMac + "\", " +
                   + "\"stat_t\":\"~/state\", " +
                   + "\"opt\":" + optiString + ", " +
                   + "\"cmd_t\": \"~/set\" }").c_str(), true);
  }
  else if (strcmp(aType.c_str(), "button") == 0) {
    client.publish((home_assistant_mqtt_prefix + "/button/" + deviceName + "/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\", " +
                   + "\"name\":\"" + deviceName + " Button\"," +
                   + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
                   + "\"avty_t\": \"" + lastWill + "\"," +
                   + "\"uniq_id\":\"switchbot_" + deviceMac + "\", " +
                   + "\"cmd_t\": \"~/set\" }").c_str(), true);
  }
  else {
    client.publish((home_assistant_mqtt_prefix + "/switch/" + deviceName + "/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\", " +
                   + "\"name\":\"" + deviceName + " Switch\"," +
                   + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
                   + "\"avty_t\": \"" + lastWill + "\"," +
                   + "\"uniq_id\":\"switchbot_" + deviceMac + "\", " +
                   + "\"stat_t\":\"~/state\", " +
                   + "\"opt\":" + optiString + ", " +
                   + "\"cmd_t\": \"~/set\" }").c_str(), true);
  }
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
                 + "\"icon\":\"mdi:cog\"," +
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
                 + "\"stat_stopped\": \"PAUSE\", " +
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


void publishHomeAssistantDiscoveryContactConfig(std::string deviceName, std::string deviceMac) {
  std::transform(deviceMac.begin(), deviceMac.end(), deviceMac.begin(), ::toupper);
  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/battery/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Battery\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_battery\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"dev_cla\":\"battery\"," +
                 + "\"unit_of_meas\": \"%\", " +
                 + "\"value_template\":\"{{ value_json.batt }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/linkquality/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Linkquality\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_linkquality\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"icon\":\"mdi:signal\"," +
                 + "\"unit_of_meas\": \"rssi\", " +
                 + "\"value_template\":\"{{ value_json.rssi }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/contact/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Contact\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_contact\"," +
                 + "\"icon\":\"mdi:door\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"value_template\":\"{{ value_json.contact }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/binary_sensor/" + deviceName + "/motion/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Motion\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_motion\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"dev_cla\":\"motion\"," +
                 + "\"pl_on\":\"MOTION\"," +
                 + "\"pl_off\":\"NO MOTION\"," +
                 + "\"value_template\":\"{{ value_json.motion }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/binary_sensor/" + deviceName + "/in/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " In\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_in\"," +
                 + "\"icon\":\"mdi:motion-sensor\"," +
                 + "\"stat_t\":\"~/in\"," +
                 + "\"pl_on\":\"ENTERED\"," +
                 + "\"pl_off\":\"IDLE\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/binary_sensor/" + deviceName + "/out/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Out\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_out\"," +
                 + "\"icon\":\"mdi:exit-run\"," +
                 + "\"stat_t\":\"~/out\"," +
                 + "\"pl_on\":\"EXITED\"," +
                 + "\"pl_off\":\"IDLE\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/binary_sensor/" + deviceName + "/button/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Button\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_button\"," +
                 + "\"stat_t\":\"~/button\"," +
                 + "\"icon\":\"mdi:gesture-tap-button\"," +
                 + "\"pl_on\":\"PUSHED\"," +
                 + "\"pl_off\":\"IDLE\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/binary_sensor/" + deviceName + "/illuminance/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Illuminance\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "__illuminance\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"dev_cla\":\"light\"," +
                 + "\"pl_on\":\"BRIGHT\"," +
                 + "\"pl_off\":\"DARK\"," +
                 + "\"value_template\":\"{{ value_json.light }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/lastmotion/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " LastMotion\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_lastmotion\"," +
                 + "\"dev_cla\":\"timestamp\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"value_template\":\"{{ now() - timedelta( seconds = value_json.lastmotion ) }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/lastcontact/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " LastContact\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_lastcontact\"," +
                 + "\"dev_cla\":\"timestamp\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"value_template\":\"{{ now() - timedelta( seconds = value_json.lastcontact ) }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/buttoncount/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " ButtonCount\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_buttoncount\"," +
                 + "\"icon\":\"mdi:counter\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"value_template\":\"{{ value_json.buttoncount }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/incount/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " InCount\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_entrancecount\"," +
                 + "\"icon\":\"mdi:counter\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"value_template\":\"{{ value_json.incount }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/outcount/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " OutCount\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_outcount\"," +
                 + "\"icon\":\"mdi:counter\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"value_template\":\"{{ value_json.outcount }}\"}").c_str(), true);
}

void publishHomeAssistantDiscoveryMotionConfig(std::string deviceName, std::string deviceMac) {
  std::transform(deviceMac.begin(), deviceMac.end(), deviceMac.begin(), ::toupper);
  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/battery/config").c_str(), ("{\"~\":\"" + (motionTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Battery\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + motionModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_battery\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"dev_cla\":\"battery\"," +
                 + "\"unit_of_meas\": \"%\", " +
                 + "\"value_template\":\"{{ value_json.batt }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/linkquality/config").c_str(), ("{\"~\":\"" + (motionTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Linkquality\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + motionModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_linkquality\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"icon\":\"mdi:signal\"," +
                 + "\"unit_of_meas\": \"rssi\", " +
                 + "\"value_template\":\"{{ value_json.rssi }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/binary_sensor/" + deviceName + "/motion/config").c_str(), ("{\"~\":\"" + (motionTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Motion\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + motionModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_motion\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"dev_cla\":\"motion\"," +
                 + "\"pl_on\":\"MOTION\"," +
                 + "\"pl_off\":\"NO MOTION\"," +
                 + "\"value_template\":\"{{ value_json.motion }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/binary_sensor/" + deviceName + "/illuminance/config").c_str(), ("{\"~\":\"" + (motionTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " Illuminance\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + motionModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_illuminance\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"dev_cla\":\"light\"," +
                 + "\"pl_on\":\"BRIGHT\"," +
                 + "\"pl_off\":\"DARK\"," +
                 + "\"value_template\":\"{{ value_json.light }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/binary_sensor/" + deviceName + "/led/config").c_str(), ("{\"~\":\"" + (motionTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " LED\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + motionModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_led\"," +
                 + "\"icon\":\"mdi:led-on\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"pl_on\":\"ON\"," +
                 + "\"pl_off\":\"OFF\"," +
                 + "\"value_template\":\"{{ value_json.led }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/lastmotion/config").c_str(), ("{\"~\":\"" + (motionTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " LastMotion\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + motionModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_lastmotion\"," +
                 + "\"dev_cla\":\"timestamp\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"value_template\":\"{{ now() - timedelta( seconds = value_json.lastmotion ) }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/sensedistance/config").c_str(), ("{\"~\":\"" + (motionTopic + deviceName) + "\"," +
                 + "\"name\":\"" + deviceName + " SenseDistance\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + motionModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_sensedistance\"," +
                 + "\"icon\":\"mdi:cog\"," +
                 + "\"stat_t\":\"~/attributes\"," +
                 + "\"value_template\":\"{{ value_json.sensedistance }}\"}").c_str(), true);

}


class ClientCallbacks : public NimBLEClientCallbacks {

    void onConnect(NimBLEClient* pClient) {
      if (printSerialOutputForDebugging) {
        Serial.println("Connected");
      }
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
      if (printSerialOutputForDebugging) {
        Serial.println("Client Passkey Request");
      }
      return 123456;
    };

    bool onConfirmPIN(uint32_t pass_key) {
      if (printSerialOutputForDebugging) {
        Serial.print("The passkey YES/NO number: ");
        Serial.println(pass_key);
      }
      return true;
    };

    void onAuthenticationComplete(ble_gap_conn_desc* desc) {
      if (!desc->sec_state.encrypted) {
        if (printSerialOutputForDebugging) {
          Serial.println("Encrypt connection failed - disconnecting");
        }
        NimBLEDevice::getClientByID(desc->conn_handle)->disconnect();
        return;
      }
    };
};

int le16_to_cpu_signed(const uint8_t data[2]) {
  unsigned value = data[0] | ((unsigned)data[1] << 8);
  if (value & 0x8000)
    return -(int)(~value) - 1;
  else
    return value;
}

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
    if (printSerialOutputForDebugging) {
      Serial.println("CUSTOM notify service not found.");
    }
    return false;
  }
  if (printSerialOutputForDebugging) {
    Serial.println("unsubscribed to notify");
  }
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
    if (printSerialOutputForDebugging) {
      Serial.println("CUSTOM notify service not found.");
    }
    return false;
  }
  if (printSerialOutputForDebugging) {
    Serial.println("subscribed to notify");
  }
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
          if (printSerialOutputForDebugging) {
            Serial.print("Wrote new value to: ");
            Serial.println(pChr->getUUID().toString().c_str());
          }
        }
        else {
          return false;
        }
      }
      else {
        byte bArray[] = {0x57, 0x02}; // write to get settings of device
        if (pChr->writeValue(bArray, 2)) {
          if (printSerialOutputForDebugging) {
            Serial.print("Wrote new value to: ");
            Serial.println(pChr->getUUID().toString().c_str());
          }
        }
        else {
          return false;
        }
      }
    }
    else {
      if (printSerialOutputForDebugging) {
        Serial.println("CUSTOM write service not found.");
      }
      return false;
    }
    if (printSerialOutputForDebugging) {
      Serial.println("Success! subscribed and got settings");
    }
    return true;
  }
}

/** Define a class to handle the callbacks when advertisments are received */
class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
      //waitForDeviceCreation = true;

      if (printSerialOutputForDebugging) {
        Serial.print("Advertised Device found: ");
        Serial.println(advertisedDevice->toString().c_str());
      }
      if (ledOnScan) {
        digitalWrite(LED_PIN, ledONValue);
      }
      publishLastwillOnline();
      std::string advStr = advertisedDevice->getAddress().toString().c_str();
      std::map<std::string, std::string>::iterator itS = allSwitchbotsOpp.find(advStr);
      bool gotAllStatus = false;

      if (itS != allSwitchbotsOpp.end())
      {
        std::string deviceName = itS->second.c_str();
        if ((advertisedDevice->isAdvertisingService(NimBLEUUID("cba20d00-224d-11e6-9fb8-0002a5d5c51b"))) || isContactDevice(deviceName) || isMotionDevice(deviceName))
        {
          std::map<std::string, NimBLEAdvertisedDevice*>::iterator itY;
          if ((shouldMQTTUpdateForDevice(advStr) || isContactDevice(deviceName) || isMotionDevice(deviceName)) && initialScanComplete) {
            itY = allSwitchbotsScanned.find(advStr);
            if (itY != allSwitchbotsScanned.end())
            {
              allSwitchbotsScanned.erase(advStr);
            }
          }
          itY = allSwitchbotsScanned.find(advStr);
          if ((itY == allSwitchbotsScanned.end()) && client.isConnected())
          {
            if (home_assistant_mqtt_discovery) {
              std::map<std::string, bool>::iterator itM = discoveredDevices.find(advStr);
              if (itM == discoveredDevices.end()) {
                if (printSerialOutputForDebugging) {
                  Serial.printf("Publishing MQTT Discovery for %s (%s)\n", deviceName.c_str(), advStr.c_str());
                }
                if (isBotDevice(deviceName)) {
                  publishHomeAssistantDiscoveryBotConfig(deviceName, advStr, home_assistant_use_opt_mode);
                }
                else if (isMeterDevice(deviceName)) {
                  publishHomeAssistantDiscoveryMeterConfig(deviceName, advStr);
                }
                else if (isContactDevice(deviceName)) {
                  publishHomeAssistantDiscoveryContactConfig(deviceName, advStr);
                }
                else if (isMotionDevice(deviceName)) {
                  publishHomeAssistantDiscoveryMotionConfig(deviceName, advStr);
                }
                else if (isCurtainDevice(deviceName)) {
                  publishHomeAssistantDiscoveryCurtainConfig(deviceName, advStr);
                }
                discoveredDevices[advStr] = true;
                delay(100);
              }
            }

            if (printSerialOutputForDebugging) {
              Serial.println("Adding Our Service ... ");
              Serial.println(itS->second.c_str());
            }
            std::string aValueString = advertisedDevice->getServiceData(0);
            gotAllStatus = callForInfoAdvDev(advertisedDevice->getAddress().toString(), advertisedDevice->getRSSI() , aValueString);
            if (gotAllStatus) {
              allSwitchbotsScanned[advStr] = advertisedDevice;
              allSwitchbotsDev[advStr] = advertisedDevice;
              std::map<std::string, long>::iterator itR = rescanTimes.find(advStr);
              if (itR != rescanTimes.end())
              {
                rescanTimes.erase(advStr);
              }
              /* if (isContactDevice(deviceName) || isMotionDevice(deviceName) || isMeterDevice(deviceName)) {
                 rescanTimes[advStr] = millis();
                }*/
            }
            if (printSerialOutputForDebugging) {
              Serial.println("Assigned advDevService");
            }
          }
        }
      }
      else {
        NimBLEDevice::addIgnored(advStr);
      }
      //waitForDeviceCreation = false;

      bool stopScan = false;
      if (client.isConnected()) {
        client.loop();
        if ((allContactSensors.size() + allMotionSensors.size()) != 0) {
          if (allSwitchbotsDev.size() == (allBots.size() + allCurtains.size() + allMeters.size() + allContactSensors.size() + allMotionSensors.size())) {
            if (!initialScanComplete) {
              initialScanComplete = true;
              stopScan = true;
            }
          }
          if (!(commandQueue.isEmpty()) && initialScanComplete && !overrideScan) {
            stopScan = true;
            forceRescan = true;
            lastScanTimes = {};
            allSwitchbotsScanned = {};
          }
        }
        else {
          if ((allSwitchbotsDev.size() == (allBots.size() + allCurtains.size() + allMeters.size() + allContactSensors.size() + allMotionSensors.size())) && (allSwitchbotsScanned.size() == (allBots.size() + allCurtains.size() + allMeters.size() + allContactSensors.size() + allMotionSensors.size())))  {
            stopScan = true;
            forceRescan = false;
            allSwitchbotsScanned = {};
          }
          else if (!(commandQueue.isEmpty()) && initialScanComplete && !overrideScan) {
            forceRescan = true;
            lastScanTimes = {};
            stopScan = true;
            allSwitchbotsScanned = {};
          }
        }
      }
      else {
        forceRescan = true;
        lastScanTimes = {};
        stopScan = true;
        allSwitchbotsScanned = {};
      }

      if (stopScan) {
        if (printSerialOutputForDebugging) {
          Serial.println("Stopping Scan found devices ... ");
        }
        NimBLEDevice::getScan()->stop();
      }
    };
    
    bool callForInfoAdvDev(std::string deviceMac, long anRSSI,  std::string aValueString) {
      if (printSerialOutputForDebugging) {
        Serial.println("callForInfoAdvDev");
      }
      if ((strcmp(deviceMac.c_str(), "") == 0)) {
        return false;
      }
      if ((strcmp(aValueString.c_str(), "") == 0)) {
        return false;
      }
      bool shouldPublish = false;
      std::string aDevice;
      std::string aState = "";
      std::string deviceStateTopic;
      std::string deviceAttrTopic;
      int aLength = aValueString.length();
      std::map<std::string, std::string>::iterator itS = allSwitchbotsOpp.find(deviceMac);
      if (itS != allSwitchbotsOpp.end())
      {
        aDevice = itS->second.c_str();
      }
      else {
        return false;
      }
      std::string deviceName;
      itS = deviceTypes.find(deviceMac);
      if (itS != deviceTypes.end())
      {
        deviceName = itS->second.c_str();
      }

      StaticJsonDocument<200> doc;
      //char aBuffer[200];
      doc["rssi"] = anRSSI;

      if (deviceName == botName) {
        if (aLength < 3) {
          return false;
        }
        shouldPublish = true;
        uint8_t byte1 = (uint8_t) aValueString[1];
        uint8_t byte2 = (uint8_t) aValueString[2];
        bool isSwitch = (byte1 & 0b10000000);
        deviceStateTopic = botTopic + aDevice + "/state";
        deviceAttrTopic = botTopic + aDevice + "/attributes";

        std::string aMode = isSwitch ? "Switch" : "Press"; // Whether the light switch Add-on is used or not
        std::string deviceAssumedStateTopic = botTopic + aDevice + "/assumedstate";

        if (isSwitch) {
          std::map<std::string, bool>::iterator itP = botsInPressMode.find(deviceMac);
          if (itP != botsInPressMode.end())
          {
            botsInPressMode.erase(deviceMac);
          }
          aState = (byte1 & 0b01000000) ? "OFF" : "ON"; // Mine is opposite, not sure why
          client.publish(deviceAssumedStateTopic.c_str(), aState.c_str(), true);
        }
        else {
          botsInPressMode[deviceMac] = true;
          std::map<std::string, bool>::iterator itE = botsSimulateONOFFinPRESSmode.find(aDevice);

          if (itE != botsSimulateONOFFinPRESSmode.end())
          {
            aMode = "PressOnOff";
            std::map<std::string, bool>::iterator itF = botsSimulatedStates.find(aDevice);
            bool boolState = false;
            if (itF != botsSimulatedStates.end())
            {
              boolState = itF->second;
            }
            else {
              boolState = itE->second;
            }
            if (boolState) {
              aState = "ON";
            } else {
              aState = "OFF";
            }
            client.publish(deviceAssumedStateTopic.c_str(), aState.c_str(), true);
          }
          else {
            aState = "OFF";
          }
        }
        int battLevel = byte2 & 0b01111111; // %
        doc["mode"] = aMode;
        doc["state"] = aState;
        doc["batt"] = battLevel;

      }
      else if (deviceName == meterName) {
        if (aLength < 6) {
          return false;
        }
        shouldPublish = true;

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

      }
      else if (deviceName == motionName) {
        if (aLength < 6) {
          return false;
        }

        deviceStateTopic = motionTopic + aDevice + "/state";
        deviceAttrTopic = motionTopic + aDevice + "/attributes";
        std::string deviceMotionTopic = motionTopic + aDevice + "/motion";
        std::string deviceLightTopic = motionTopic + aDevice + "/illuminance";

        uint8_t byte1 = (uint8_t) aValueString[1];
        uint8_t byte2 = (uint8_t) aValueString[2];
        uint8_t byte3 = (uint8_t) aValueString[3];
        uint8_t byte4 = (uint8_t) aValueString[4];
        uint8_t byte5 = (uint8_t) aValueString[5];

        int battLevel = (byte2 & 0b01111111);
        doc["batt"] = battLevel;

        byte data[] = {byte4, byte3};
        long lastMotionLowSeconds = le16_to_cpu_signed(data);
        long lastMotionHighSeconds = (byte5 & 0b10000000);
        long lastMotion = lastMotionHighSeconds + lastMotionLowSeconds;
        doc["lastmotion"] = lastMotion;

        std::map<std::string, long>::iterator itU = lastMotions.find(aDevice);
        itU = lastMotions.find(aDevice);
        if (itU == lastMotions.end())
        {
          lastMotions[aDevice] = millis();
        }
        bool aMotion = (byte1 & 0b01000000);

        bool missedAMotion = (millis() - lastMotions[aDevice] > (lastMotion * 1000));
        if (missedAMotion) {
          shouldPublish = true;
        }

        if (sendBackupMotionContact) {
          if ( missedAMotion && (lastMotion < missedDataResend) ) {
            aMotion = true;
          }
        }
        if (aMotion) {
          lastMotions[aDevice] = millis();
        }

        std::string motion = aMotion ? "MOTION" : "NO MOTION";
        doc["motion"] = motion;
        aState = motion;
        std::string ledState = (byte5 & 0b00100000) ? "ON" : "OFF";
        doc["led"] = ledState;

        bool sensingDistanceA = (byte5 & 0b00001000);
        bool sensingDistanceB = (byte5 & 0b00000100);
        std::string sensingDistance;
        if (!sensingDistanceA && !sensingDistanceB) {
          sensingDistance = "LONG";
        }
        else if (!sensingDistanceA && sensingDistanceB) {
          sensingDistance = "MIDDLE";
        }
        else if (sensingDistanceA && !sensingDistanceB) {
          sensingDistance = "SHORT";
        }
        else if (sensingDistanceA && sensingDistanceB) {
          sensingDistance = "RESERVE";
        }

        doc["sensedistance"] = sensingDistance;

        bool lightA = (byte5 & 0b00000010);
        bool lightB = (byte5 & 0b00000001);
        std::string light;
        if (!lightA && !lightB) {
          light = "RESERVE";
        }
        else if (!lightA && lightB) {
          light = "DARK";
        }
        else if (lightA && !lightB) {
          light = "BRIGHT";
        }
        else if (lightA && lightB) {
          light = "RESERVE";
        }

        doc["light"] = light;

        std::map<std::string, std::string>::iterator itH = motionStates.find(deviceMac);
        if (itH != motionStates.end())
        {
          std::string motionState = itH->second.c_str();
          if (strcmp(motionState.c_str(), motion.c_str()) != 0) {
            shouldPublish = true;
          }
        }
        motionStates[deviceMac] = motion;

        std::map<std::string, std::string>::iterator itL = ledStates.find(deviceMac);
        if (itL != ledStates.end())
        {
          std::string aLED = itL->second.c_str();
          if (strcmp(aLED.c_str(), ledState.c_str()) != 0) {
            shouldPublish = true;
          }
        }
        ledStates[deviceMac] = ledState;

        std::map<std::string, std::string>::iterator itI = illuminanceStates.find(deviceMac);
        if (itI != illuminanceStates.end())
        {
          std::string illuminanceState = itI->second.c_str();
          if (strcmp(illuminanceState.c_str(), light.c_str()) != 0) {
            shouldPublish = true;
          }
        }
        illuminanceStates[deviceMac] = light;

        if (!initialScanComplete) {
          shouldPublish = true;
        }

        if (!shouldPublish) {
          if (shouldMQTTUpdateForDevice(deviceMac)) {
            shouldPublish = true;
          }
        }

        if (shouldPublish) {
          client.publish(deviceMotionTopic.c_str(), motion.c_str(), true);
          client.publish(deviceLightTopic.c_str(), light.c_str(), true);
        }
      }

      else if (deviceName == contactName) {
        if (aLength < 9) {
          return false;
        }

        std::string deviceButtonTopic = contactTopic + aDevice + "/button";
        std::string deviceContactTopic = contactTopic + aDevice + "/contact";
        std::string deviceMotionTopic = contactTopic + aDevice + "/motion";
        std::string deviceInTopic = contactTopic + aDevice + "/in";
        std::string deviceOutTopic = contactTopic + aDevice + "/out";
        std::string deviceLightTopic = contactTopic + aDevice + "/illuminance";
        deviceStateTopic = contactTopic + aDevice + "/state";
        deviceAttrTopic = contactTopic + aDevice + "/attributes";

        uint8_t byte1 = (uint8_t) aValueString[1];
        uint8_t byte2 = (uint8_t) aValueString[2];
        uint8_t byte3 = (uint8_t) aValueString[3];
        uint8_t byte4 = (uint8_t) aValueString[4];
        uint8_t byte5 = (uint8_t) aValueString[5];
        uint8_t byte6 = (uint8_t) aValueString[6];
        uint8_t byte7 = (uint8_t) aValueString[7];
        uint8_t byte8 = (uint8_t) aValueString[8];
        int battLevel = (byte2 & 0b01111111);
        doc["batt"] = battLevel;

        byte data[] = {byte5, byte4};
        long lastMotionLowSeconds = le16_to_cpu_signed(data);
        long lastMotionHighSeconds = (byte3 & 0b10000000);
        long lastMotion = lastMotionHighSeconds + lastMotionLowSeconds;
        doc["lastmotion"] = lastMotion;

        byte data2[] = {byte7, byte6};
        long lastContactLowSeconds = le16_to_cpu_signed(data2);
        long lastContactHighSeconds = (byte3 & 0b01000000);
        long lastContact = lastContactHighSeconds + lastContactLowSeconds;
        doc["lastcontact"] = lastContact;

        std::map<std::string, long>::iterator itU = lastContacts.find(aDevice);
        if (itU == lastContacts.end())
        {
          lastContacts[aDevice] = millis();
        }

        itU = lastMotions.find(aDevice);
        if (itU == lastMotions.end())
        {
          lastMotions[aDevice] = millis();
        }
        bool aMotion = (byte1 & 0b01000000);

        bool missedAMotion = (millis() - lastMotions[aDevice] > (lastMotion * 1000));
        if (missedAMotion) {
          shouldPublish = true;
        }

        if (sendBackupMotionContact) {
          if ( missedAMotion && (lastMotion < missedDataResend) ) {
            aMotion = true;
          }
        }

        std::string motion = aMotion ? "MOTION" : "NO MOTION";
        doc["motion"] = motion;

        if (aMotion) {
          lastMotions[aDevice] = millis();
        }

        bool contactA = (byte3 & 0b00000100);
        bool contactB = (byte3 & 0b00000010);

        std::string contact;
        if (!contactA && !contactB) {
          contact = "CLOSED";
        }
        else if (!contactA && contactB) {
          contact = "OPEN";
          lastContacts[aDevice] = millis();
        }
        else if (contactA && !contactB) {
          contact = "TIMEOUT";
        }
        else if (contactA && contactB) {
          contact = "RESERVE";
        }

        bool missedAContact = (millis() - lastContacts[aDevice] > (lastContact * 1000));
        if (missedAContact) {
          shouldPublish = true;
        }

        if (sendBackupMotionContact) {
          if ( missedAContact && (lastContact < missedDataResend) ) {
            contact = "OPEN";
            lastContacts[aDevice] = millis();
          }
        }

        doc["contact"] = contact;
        aState = contact;
        std::string light = (byte3 & 0b00000001) ? "BRIGHT" : "DARK";
        doc["light"] = light;

        int entranceCountA = (byte8 & 0b10000000) ? 2 : 0;
        int entranceCountB = (byte8 & 0b01000000) ? 1 : 0;
        int entranceCount = entranceCountA + entranceCountB;
        doc["incount"] = entranceCount;

        int outCountA = (byte8 & 0b00100000) ? 2 : 0;
        int outCountB = (byte8 & 0b00010000) ? 1 : 0;
        int outCount = outCountA + outCountB;
        doc["outcount"] = outCount;

        int buttonCountA = (byte8 & 0b00001000) ? 8 : 0;
        int buttonCountB = (byte8 & 0b00000100) ? 4 : 0;
        int buttonCountC = (byte8 & 0b00000010) ? 2 : 0;
        int buttonCountD = (byte8 & 0b00000001) ? 1 : 0;
        int buttonCount = buttonCountA + buttonCountB + buttonCountC + buttonCountD;
        doc["buttoncount"] = buttonCount;

        std::map<std::string, std::string>::iterator itH = motionStates.find(deviceMac);
        if (itH != motionStates.end())
        {
          std::string motionState = itH->second.c_str();
          if (strcmp(motionState.c_str(), motion.c_str()) != 0) {
            shouldPublish = true;
          }

        }
        motionStates[deviceMac] = motion;

        std::map<std::string, std::string>::iterator itC = contactStates.find(deviceMac);
        if (itC != contactStates.end())
        {
          std::string contactState = itC->second.c_str();
          if (strcmp(contactState.c_str(), contact.c_str()) != 0) {
            shouldPublish = true;
          }

        }
        contactStates[deviceMac] = contact;

        std::map<std::string, std::string>::iterator itI = illuminanceStates.find(deviceMac);
        if (itI != illuminanceStates.end())
        {
          std::string illuminanceState = itI->second.c_str();
          if (strcmp(illuminanceState.c_str(), light.c_str()) != 0) {
            shouldPublish = true;
          }

        }
        illuminanceStates[deviceMac] = light;

        std::map<std::string, int>::iterator itE = entranceCounts.find(deviceMac);
        if (itE != entranceCounts.end())
        {
          int eCount = itE->second;
          if (eCount != entranceCount) {
            shouldPublish = true;
            client.publish(deviceInTopic.c_str(), "ENTERING", false);
          }
        }
        entranceCounts[deviceMac] = entranceCount;

        std::map<std::string, int>::iterator itO = outCounts.find(deviceMac);
        if (itO != outCounts.end())
        {
          int oCount = itO->second;
          if (oCount != outCount) {
            shouldPublish = true;
            client.publish(deviceOutTopic.c_str(), "EXITING", false);
          }
        }
        outCounts[deviceMac] = outCount;

        std::map<std::string, int>::iterator itBu = buttonCounts.find(deviceMac);
        if (itBu != buttonCounts.end())
        {
          int bCount = itBu->second;
          if (bCount != buttonCount) {
            shouldPublish = true;
            client.publish(deviceButtonTopic.c_str(), "PUSHED", false);
          }
        }
        buttonCounts[deviceMac] = buttonCount;

        if (!initialScanComplete) {
          shouldPublish = true;
        }

        if (!shouldPublish) {
          if (shouldMQTTUpdateForDevice(deviceMac)) {
            shouldPublish = true;
          }
        }

        if (shouldPublish) {
          client.publish(deviceMotionTopic.c_str(), motion.c_str(), true);
          client.publish(deviceContactTopic.c_str(), contact.c_str(), true);
          client.publish(deviceLightTopic.c_str(), light.c_str(), true);
          client.publish(deviceInTopic.c_str(), "IDLE", false);
          client.publish(deviceOutTopic.c_str(), "IDLE", false);
          client.publish(deviceButtonTopic.c_str(), "IDLE", false);
        }

      }

      else if (deviceName == curtainName) {
        if (aLength < 5) {
          return false;
        }

        shouldPublish = true;
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

        StaticJsonDocument<50> docPos;
        docPos["pos"] = currentPosition;
        serializeJson(docPos, aBuffer);
        client.publish(devicePosTopic.c_str(), aBuffer, true);
      }
      else {
        return false;
      }

      if (shouldPublish) {
        lastScanTimes[deviceMac] = millis();
        serializeJson(doc, aBuffer);
        client.publish(deviceAttrTopic.c_str(), aBuffer, true);
        delay(50);
        client.publish(deviceStateTopic.c_str(), aState.c_str(), true);
      }

      return true;
    };
};

void initialScanEndedCB(NimBLEScanResults results) {
  //pScan->setFilterPolicy(BLE_HCI_SCAN_FILT_USE_WL);
  if (printSerialOutputForDebugging) {
    Serial.println("initialScanEndedCB");
  }
  if (ledOnBootScan) {
    digitalWrite(LED_PIN, ledOFFValue);
  }
  initialScanComplete = true;
  isRescanning = false;
  allSwitchbotsScanned = {};
  if (printSerialOutputForDebugging) {
    Serial.println("Scan Ended");
  }
  client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"idle\"}");
}

void scanEndedCB(NimBLEScanResults results) {
  if (ledOnScan || ledOnCommand) {
    digitalWrite(LED_PIN, ledOFFValue);
  }
  allSwitchbotsScanned = {};
  if (printSerialOutputForDebugging) {
    Serial.println("Scan Ended");
  }
  client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"idle\"}");
}

void rescanEndedCB(NimBLEScanResults results) {
  if (ledOnScan || ledOnCommand) {
    digitalWrite(LED_PIN, ledOFFValue);
  }
  isRescanning = false;
  lastRescan = millis();
  allSwitchbotsScanned = {};
  if (printSerialOutputForDebugging) {
    Serial.println("ReScan Ended");
  }
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
  forceRescan = false;
  pinMode (LED_PIN, OUTPUT);
  // Connect to WiFi network
  WiFi.begin(ssid, password);
  if (printSerialOutputForDebugging) {
    Serial.println("");
  }

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    if (printSerialOutputForDebugging) {
      Serial.print(".");
    }
  }
  if (printSerialOutputForDebugging) {
    Serial.println("");
    Serial.print("Connected to ");
    Serial.println(ssid);
    Serial.print("IP address: ");
    Serial.println(WiFi.localIP());
  }

  /*use mdns for host name resolution*/
  if (!MDNS.begin(host)) { //http://esp32.local
    if (printSerialOutputForDebugging) {
      Serial.println("Error setting up MDNS responder!");
    }
    while (1) {
      delay(1000);
    }
  }
  if (printSerialOutputForDebugging) {
    Serial.println("mDNS responder started");
  }
  /*return index page which is stored in serverIndex */
  server.on("/", HTTP_GET, []() {
    server.sendHeader("Connection", "close");
    if (useLoginScreen) {
      if (!server.authenticate(otaUserId.c_str(), otaPass.c_str())) {
        return server.requestAuthentication();
      }
    }
    server.send(200, "text/html", serverIndex);
  });
  /*handling uploading firmware file */
  server.on("/update", HTTP_POST, []() {
    server.sendHeader("Connection", "close");
    if (useLoginScreen) {
      if (!server.authenticate(otaUserId.c_str(), otaPass.c_str())) {
        return server.requestAuthentication();
      }
    }
    server.send(200, "text/plain", (Update.hasError()) ? "FAIL" : "OK");
    ESP.restart();
  }, []() {
    HTTPUpload& upload = server.upload();
    if (upload.status == UPLOAD_FILE_START) {
      if (printSerialOutputForDebugging) {
        Serial.printf("Update: %s\n", upload.filename.c_str());
      }
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
        if (printSerialOutputForDebugging) {
          Serial.printf("Update Success: %u\nRebooting...\n", upload.totalSize);
        }
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
  static std::map<std::string, std::string> allContactSensorsTemp = {};
  static std::map<std::string, std::string> allMotionSensorsTemp = {};
  static std::map<std::string, std::string> allPasswordsTemp = {};
  static std::map<std::string, bool> botsSimulateONOFFinPRESSmodeTemp = {};
  static std::map<std::string, int> botsSimulatedOFFHoldTimesTemp = {};
  static std::map<std::string, int> botsSimulatedONHoldTimesTemp = {};
  NimBLEDevice::init("");

  std::map<std::string, std::string>::iterator it = allBots.begin();
  std::string anAddr;
  std::string aName;
  while (it != allBots.end())
  {
    aName = it->first;
    anAddr = it->second;
    std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
    std::replace( aName.begin(), aName.end(), ' ', '_');
    allSwitchbotsOpp[anAddr.c_str()] = aName;
    allSwitchbots[aName] = anAddr.c_str();
    deviceTypes[anAddr.c_str()] = botName;
    allBotsTemp[aName] = anAddr.c_str();
    NimBLEDevice::whiteListAdd(NimBLEAddress(anAddr));
    it++;
  }
  allBots = allBotsTemp;

  it = allCurtains.begin();
  while (it != allCurtains.end())
  {
    aName = it->first;
    anAddr = it->second;
    std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
    std::replace( aName.begin(), aName.end(), ' ', '_');
    allSwitchbotsOpp[anAddr.c_str()] = aName;
    allSwitchbots[aName] = anAddr.c_str();
    deviceTypes[anAddr.c_str()] = curtainName;
    allCurtainsTemp[aName] = anAddr.c_str();
    NimBLEDevice::whiteListAdd(NimBLEAddress(anAddr));
    it++;
  }
  allCurtains = allCurtainsTemp;

  it = allMeters.begin();
  while (it != allMeters.end())
  {
    aName = it->first;
    anAddr = it->second;
    std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
    std::replace( aName.begin(), aName.end(), ' ', '_');
    allSwitchbotsOpp[anAddr.c_str()] = aName;
    allSwitchbots[aName] = anAddr.c_str();
    deviceTypes[anAddr.c_str()] = meterName;
    allMetersTemp[aName] = anAddr.c_str();
    NimBLEDevice::whiteListAdd(NimBLEAddress(anAddr));
    it++;
  }
  allMeters = allMetersTemp;

  it = allContactSensors.begin();
  while (it != allContactSensors.end())
  {
    aName = it->first;
    anAddr = it->second;
    std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
    std::replace( aName.begin(), aName.end(), ' ', '_');
    allSwitchbotsOpp[anAddr.c_str()] = aName;
    allSwitchbots[aName] = anAddr.c_str();
    deviceTypes[anAddr.c_str()] = contactName;
    allContactSensorsTemp[aName] = anAddr.c_str();
    NimBLEDevice::whiteListAdd(NimBLEAddress(anAddr));
    it++;
  }
  allContactSensors = allContactSensorsTemp;

  it = allMotionSensors.begin();
  while (it != allMotionSensors.end())
  {
    aName = it->first;
    anAddr = it->second;
    std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
    std::replace( aName.begin(), aName.end(), ' ', '_');
    allSwitchbotsOpp[anAddr.c_str()] = aName;
    allSwitchbots[aName] = anAddr.c_str();
    deviceTypes[anAddr.c_str()] = motionName;
    allMotionSensorsTemp[aName] = anAddr.c_str();
    NimBLEDevice::whiteListAdd(NimBLEAddress(anAddr));
    it++;
  }
  allMotionSensors = allMotionSensorsTemp;

  it = allPasswords.begin();
  std::string aPass;
  while (it != allPasswords.end())
  {
    aName = it->first;
    aPass = it->second;
    std::replace( aName.begin(), aName.end(), ' ', '_');
    allPasswordsTemp[aName] = aPass.c_str();
    it++;
  }
  allPasswords = allPasswordsTemp;

  std::map<std::string, bool>::iterator itT = botsSimulateONOFFinPRESSmode.begin();
  bool aBool;
  while (itT != botsSimulateONOFFinPRESSmode.end())
  {
    aName = itT->first;
    aBool = itT->second;
    std::replace( aName.begin(), aName.end(), ' ', '_');
    botsSimulateONOFFinPRESSmodeTemp[aName] = aBool;
    itT++;
  }
  botsSimulateONOFFinPRESSmode = botsSimulateONOFFinPRESSmodeTemp;

  std::map<std::string, int>::iterator itY = botsSimulatedOFFHoldTimes.begin();
  int aInt;
  while (itY != botsSimulatedOFFHoldTimes.end())
  {
    aName = itY->first;
    aInt = itY->second;
    std::replace( aName.begin(), aName.end(), ' ', '_');
    botsSimulatedOFFHoldTimesTemp[aName] = aInt;
    itY++;
  }
  botsSimulatedOFFHoldTimes = botsSimulatedOFFHoldTimesTemp;

  itY = botsSimulatedONHoldTimes.begin();
  while (itY != botsSimulatedONHoldTimes.end())
  {
    aName = itY->first;
    aInt = itY->second;
    std::replace( aName.begin(), aName.end(), ' ', '_');
    botsSimulatedONHoldTimesTemp[aName] = aInt;
    itY++;
  }
  botsSimulatedONHoldTimes = botsSimulatedONHoldTimesTemp;

  Serial.begin(115200);
  Serial.println("Switchbot ESP32 starting...");
  if (!printSerialOutputForDebugging) {
    Serial.println("Set printSerialOutputForDebugging = true to see more Serial output");
  }
  if (printSerialOutputForDebugging) {
    Serial.println("Starting NimBLE Client");
  }

  NimBLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM |*/ BLE_SM_PAIR_AUTHREQ_SC);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  //NimBLEDevice::setScanFilterMode(2);
  pScan = NimBLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  pScan->setInterval(45);
  pScan->setWindow(15);
  pScan->setDuplicateFilter(false);
  pScan->setActiveScan(true);
  //pScan->setMaxResults(20);
  //pScan->setFilterPolicy(BLE_HCI_SCAN_FILT_USE_WL);

}

void rescan(int seconds) {
  lastRescan = millis();
  while (pScan->isScanning()) {
    delay(50);
  }
  allSwitchbotsScanned = {};
  //pScan->clearResults();
  lastRescan = millis();
  isRescanning = true;
  delay(50);
  client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"scanning\"}");
  delay(50);
  if (ledOnScan) {
    digitalWrite(LED_PIN, ledONValue);
  }
  pScan->start(seconds, rescanEndedCB, true);
}

void scanForever() {
  lastRescan = millis();
  while (pScan->isScanning()) {
    delay(50);
  }
  //allSwitchbotsScanned = {};
  lastRescan = millis();
  isRescanning = true;
  delay(50);
  client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"scanning\"}");
  delay(50);
  if (ledOnScan) {
    digitalWrite(LED_PIN, ledONValue);
  }
  pScan->start(0, rescanEndedCB, true);
}

void rescanFind(std::string aMac) {
  if (isRescanning) {
    return;
  }
  while (pScan->isScanning()) {
    delay(50);
  }

  allSwitchbotsScanned = {};
  std::map<std::string, NimBLEAdvertisedDevice*>::iterator it = allSwitchbotsDev.begin();
  std::string anAddr;

  while (it != allSwitchbotsDev.end())
  {
    anAddr = it->first;
    if (anAddr != aMac) {
      allSwitchbotsScanned[anAddr] = it->second;
    }
    it++;
  }

  //allSwitchbotsDev.erase(aMac);
  //pScan->erase(NimBLEAddress(aMac));
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

    if (printSerialOutputForDebugging) {
      Serial.println("In all get bot settings...");
    }
    gotSettings = true;
    std::map<std::string, std::string>::iterator itT = allBots.begin();
    std::string aDevice;
    while (itT != allBots.end())
    {
      aDevice = itT->first;
      processing = true;
      client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"getsettings\"}");
      controlMQTT(aDevice, "REQUESTSETTINGS", true);
      client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"idle\"}");
      processing = false;
      itT++;
    }
    if (ledOnBootScan) {
      digitalWrite(LED_PIN, ledOFFValue);
    }
    if (printSerialOutputForDebugging) {
      Serial.println("Added all get bot settings...");
    }
  }
}

void loop () {
  vTaskDelay(10 / portTICK_PERIOD_MS);
  server.handleClient();
  client.loop();
  publishLastwillOnline();

  if ((!initialScanComplete) && client.isConnected() && (!waitForResponse) && (!processing) && (!(pScan->isScanning())) && (!isRescanning)) {
    client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"scanning\"}");
    isRescanning = true;
    pScan->start(initialScan, initialScanEndedCB, true);
  }

  if (initialScanComplete && client.isConnected()) {
    if (isRescanning) {
      lastRescan = millis();
    }
    if ((!waitForResponse) && (!processing) && (!(pScan->isScanning())) && (!isRescanning)) {
      if (getSettingsOnBoot && !gotSettings ) {
        getAllBotSettings();
      }
    }

    if ((allContactSensors.size() + allMotionSensors.size()) != 0) {
      if ((!waitForResponse) && (!processing) && (!(pScan->isScanning())) && (!isRescanning)) {
        bool queueProcessed = false;
        queueProcessed = processQueue();
      }
      if (commandQueue.isEmpty() && (!waitForResponse) && (!processing) && (!(pScan->isScanning())) && (!isRescanning)) {
        if (!getSettingsOnBoot || (getSettingsOnBoot && gotSettings) ) {
          startForeverScan();
        }
      }
    }
    else {
      if ((!waitForResponse) && (!processing) && (!(pScan->isScanning())) && (!isRescanning)) {
        bool queueProcessed = false;
        queueProcessed = processQueue();
        if (commandQueue.isEmpty() && queueProcessed && !waitForResponse && !processing && !(pScan->isScanning()) && !isRescanning) {
          if (scanAfterControl || (!allMeters.empty())) {
            recurringScan();
          }
        }
      }
      if (commandQueue.isEmpty() && (!waitForResponse) && (!processing) && (!(pScan->isScanning())) && (!isRescanning)) {
        if (autoRescan || forceRescan) {
          recurringRescan();
        }
      }
    }
  }
}

void recurringRescan() {
  if (isRescanning) {
    lastRescan = millis();
    return;
  }

  if (((millis() - lastRescan) >= (rescanTime * 1000)) || forceRescan) {
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

void startForeverScan() {
  if (isRescanning) {
    lastRescan = millis();
    return;
  }
  if (!processing && !(pScan->isScanning()) && !isRescanning) {
    scanForever();
  }
}

bool shouldMQTTUpdateForDevice(std::string anAddr) {
  if (!client.isConnected()) {
    return false;
  }
  std::map<std::string, std::string>::iterator itB = allSwitchbotsOpp.find(anAddr);
  long lastScanTime = 0;
  std::map<std::string, long>::iterator itX = lastScanTimes.find(anAddr);
  if (itX != lastScanTimes.end())
  {
    lastScanTime = itX->second;
  }
  else
  {
    return true;
  }

  if ((millis() - lastScanTime) >= (rescanTime * 1000)) {
    return true;
  }

  std::map<std::string, std::string>::iterator itM = allMeters.find(itB->second);
  if (itM != allMeters.end())
  {
    if ((lastScanTime == 0 ) || ((millis() - lastScanTime) >= (defaultMeterScanSecs * 1000))) {
      return true;
    }
  }
  itM = allContactSensors.find(itB->second);
  if (itM != allContactSensors.end())
  {
    if ((lastScanTime == 0 ) || ((millis() - lastScanTime) >= (defaultContactScanSecs * 1000))) {
      return true;
    }
  }
  itM = allMotionSensors.find(itB->second);
  if (itM != allMotionSensors.end())
  {
    if ((lastScanTime == 0 ) || ((millis() - lastScanTime) >= (defaultMotionScanSecs * 1000))) {
      return true;
    }
  }

  std::map<std::string, int>::iterator itS = botScanTime.find(itB->second);
  long lastTime;
  std::map<std::string, long>::iterator it = rescanTimes.find(anAddr);
  if (it != rescanTimes.end())
  {
    lastTime = it->second;
  }
  else {
    return false;
  }

  long scanTime = defaultScanAfterControlSecs; //default if not in list

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
      int holdTimePlus = (itH->second) + defaultScanAfterControlSecs;
      if (holdTimePlus > scanTime) {
        scanTime =  holdTimePlus;
      }
    }
  }

  if ((millis() - lastTime) >= (scanTime * 1000)) {
    return true;
  }
  return false;
}

void recurringScan() {
  if ((millis() - lastScanCheck) >= 200) {
    recurringMeterScan();
    if (!rescanTimes.empty()) {
      std::map<std::string, long>::iterator it = rescanTimes.begin();
      std::string anAddr;
      while (it != rescanTimes.end())
      {
        anAddr = it->first;
        bool shouldMQTTUpdate = false;
        shouldMQTTUpdate = shouldMQTTUpdateForDevice(anAddr);
        if (shouldMQTTUpdate) {
          if (!processing && !(pScan->isScanning()) && !isRescanning) {
            rescanFind(anAddr);
            delay(100);
            std::map<std::string, long>::iterator itS = rescanTimes.find(anAddr);
            if (itS != rescanTimes.end())
            {
              rescanTimes.erase(anAddr);
            }
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

void recurringMeterScan() {
  if (!allMeters.empty()) {
    std::map<std::string, std::string>::iterator it = allMeters.begin();
    std::string anAddr = it->second;
    while (it != allMeters.end())
    {
      bool shouldMQTTUpdate = false;
      shouldMQTTUpdate = shouldMQTTUpdateForDevice(anAddr);
      if (shouldMQTTUpdate) {
        rescanTimes[anAddr] = millis();
      }
      it++;
    }
  }
}


bool processRequest(std::string macAdd, std::string aName, const char * command, std::string deviceTopic, bool disconnectAfter) {
  bool isSuccess = false;
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
      overrideScan = true;
      rescanFind(macAdd);
      //pScan->start(10 * count, scanEndedCB, true);
      delay(100);
      while (pScan->isScanning()) {
        delay(10);
      }
      overrideScan = false;
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
    //char aBuffer[100];
    doc["id"] = aName.c_str();
    doc["status"] = "errorLocatingDevice";
    serializeJson(doc, aBuffer);
    client.publish((deviceTopic + "/status").c_str(), aBuffer);
  }
  else {
    isSuccess = sendToDevice(advDevice, aName, command, deviceTopic, disconnectAfter);
  }
  return isSuccess;
}

bool waitToProcess(QueueCommand aCommand) {
  bool wait = false;
  long waitTimeLeft = 0;

  std::map<std::string, bool>::iterator itP = botsToWaitFor.find(aCommand.device);
  if (itP != botsToWaitFor.end())
  {
    if (!aCommand.priority) {
      return true;
    }
  }

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
    if (printSerialOutputForDebugging) {
      Serial.print("Control for device: ");
      Serial.print(aCommand.device.c_str());
      Serial.print(" will wait ");
      Serial.print(waitTimeLeft);
      Serial.println(" millisecondSeconds");
    }
  }
  return wait;
}

bool processQueue() {
  processing = true;
  struct QueueCommand aCommand;
  if (!commandQueue.isEmpty()) {
    bool disconnectAfter = true;
    if (ledOnCommand) {
      digitalWrite(LED_PIN, ledONValue);
    }
    if (!waitForResponse) {
      bool requeue = false;
      bool skip = false;
      aCommand = commandQueue.getHead();
      bool getSettingsAfter = false;
      std::string requestDevice = aCommand.device.c_str();
      std::string deviceStateTopic = botTopic + aCommand.device + "/state";
      if ((aCommand.topic == ESPMQTTTopic + "/rescan") && isRescanning) {
        commandQueue.dequeue();
      }
      else {
        if ( pScan->isScanning() || isRescanning ) {
          return false;
        }
        processing = true;
        if (printSerialOutputForDebugging) {
          Serial.print("Received something on ");
          Serial.println(aCommand.topic.c_str());
          Serial.println(aCommand.device.c_str());
        }
        if (aCommand.topic == ESPMQTTTopic + "/control") {
          if (aCommand.disconnectAfter == false) {
            disconnectAfter = false;
          };
          processing = true;
          bool boolState = false;

          if (waitToProcess(aCommand)) {
            std::map<std::string, bool>::iterator itP = botsToWaitFor.find(aCommand.device);
            if (itP == botsToWaitFor.end())
            {
              botsToWaitFor[aCommand.device] = true;
              aCommand.priority = true;
            }
            commandQueue.enqueue(aCommand);
          }
          else {
            bool skipProcess = false;
            if ((strcmp(aCommand.payload.c_str(), "OFF") == 0) || (strcmp(aCommand.payload.c_str(), "ON") == 0)) {
              if (isBotDevice(aCommand.device.c_str()))
              {
                std::map<std::string, std::string>::iterator itN = allBots.find(aCommand.device);
                std::string anAddr = itN->second;
                std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
                std::map<std::string, bool>::iterator itP = botsInPressMode.find(anAddr);
                std::map<std::string, bool>::iterator itE = botsSimulateONOFFinPRESSmode.find(aCommand.device);
                if (itP != botsInPressMode.end()) {
                  if (itE != botsSimulateONOFFinPRESSmode.end()) {
                    std::map<std::string, bool>::iterator itF = botsSimulatedStates.find(aCommand.device);
                    if (itF != botsSimulatedStates.end())
                    {
                      boolState = itF->second;
                      if (boolState && (strcmp(aCommand.payload.c_str(), "ON") == 0)) {
                        skipProcess = true;
                        client.publish(deviceStateTopic.c_str(), "ON", true);
                        std::map<std::string, bool>::iterator itP = botsToWaitFor.find(aCommand.device);
                        if (itP != botsToWaitFor.end())
                        {
                          botsToWaitFor.erase(aCommand.device);
                        }
                      }
                      else if (!boolState && (strcmp(aCommand.payload.c_str(), "OFF") == 0)) {
                        skipProcess = true;
                        client.publish(deviceStateTopic.c_str(), "OFF", true);
                        std::map<std::string, bool>::iterator itP = botsToWaitFor.find(aCommand.device);
                        if (itP != botsToWaitFor.end())
                        {
                          botsToWaitFor.erase(aCommand.device);
                        }
                      }
                    }
                  }
                  else {
                    if (strcmp(aCommand.payload.c_str(), "OFF") == 0) {
                      skipProcess = true;
                      std::map<std::string, bool>::iterator itP = botsToWaitFor.find(aCommand.device);
                      if (itP != botsToWaitFor.end())
                      {
                        botsToWaitFor.erase(aCommand.device);
                      }
                    }
                  }
                }
              }
            }
            if (!skipProcess) {
              if (ledOnCommand) {
                digitalWrite(LED_PIN, ledONValue);
              }
              noResponse = true;
              bool shouldContinue = true;
              long timeSent = millis();
              bool isSuccess = false;
              if (isBotDevice(aCommand.device.c_str()))
              {
                bool isNum = is_number(aCommand.payload.c_str());
                if (isNum) {
                  isSuccess = controlMQTT(aCommand.device, aCommand.payload, false);
                }
                else {
                  isSuccess = controlMQTT(aCommand.device, aCommand.payload, disconnectAfter);
                }
                if (getBotResponse) {
                  while (noResponse && shouldContinue )
                  {
                    waitForResponse = true;
                    //if (printSerialOutputForDebugging) {Serial.println("waiting for response...");}
                    if ((millis() - timeSent) > (waitForResponseSec * 1000)) {
                      shouldContinue = false;
                    }
                  }
                  if (noResponse && assumeNoResponseMeansSuccess && !retryBotActionNoResponse && isSuccess) {
                    std::string deviceAssumedStateTopic = botTopic + aCommand.device + "/state";
                    std::map<std::string, bool>::iterator itE = botsSimulateONOFFinPRESSmode.find(aCommand.device);
                    if (itE != botsSimulateONOFFinPRESSmode.end()) {
                      if ((strcmp(aCommand.payload.c_str(), "ON") == 0) || (strcmp(aCommand.payload.c_str(), "OFF") == 0)) {
                        if (strcmp(aCommand.payload.c_str(), "OFF") == 0) {
                          botsSimulatedStates[aCommand.device] = false;
                          client.publish(deviceStateTopic.c_str(), "OFF", true);
                          client.publish(deviceAssumedStateTopic.c_str(), "OFF", true);
                        }
                        else if (strcmp(aCommand.payload.c_str(), "ON") == 0) {
                          botsSimulatedStates[aCommand.device] = true;
                          client.publish(deviceStateTopic.c_str(), "ON", true);
                          client.publish(deviceAssumedStateTopic.c_str(), "ON", true);
                        }
                        else if (strcmp(aCommand.payload.c_str(), "PRESS") == 0) {
                          botsSimulatedStates[aCommand.device] = !(botsSimulatedStates[aCommand.device]);
                          if (botsSimulatedStates[aCommand.device]) {
                            client.publish(deviceStateTopic.c_str(), "ON", true);
                            client.publish(deviceAssumedStateTopic.c_str(), "ON", true);
                          }
                          else {
                            client.publish(deviceStateTopic.c_str(), "OFF", true);
                            client.publish(deviceAssumedStateTopic.c_str(), "OFF", true);
                          }
                        }
                      }
                    }
                  }
                  std::map<std::string, std::string>::iterator itN = allBots.find(aCommand.device);
                  std::string anAddr = itN->second;
                  std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
                  NimBLEClient* pClient = nullptr;
                  if (NimBLEDevice::getClientListSize()) {
                    pClient = NimBLEDevice::getClientByPeerAddress(anAddr);
                    if (pClient) {
                      if (pClient->isConnected()) {
                        unsubscribeToNotify(pClient);
                        if (disconnectAfter) {
                          pClient->disconnect();
                        }
                      }
                    }
                  }

                  if (isNum && !lastCommandWasBusy && getBotResponse) {
                    getSettingsAfter = true;
                  }
                  if (lastCommandWasBusy && retryBotOnBusy) {
                    requeue = true;
                    botsToWaitFor[aCommand.device] = true;
                    lastCommandWasBusy = false;
                    getSettingsAfter = false;

                    lastCommandSent[anAddr] = 0;
                  }
                  else if ((retryBotActionNoResponse && noResponse && (aCommand.currentTry <= noResponseRetryAmount)) || (retryBotSetNoResponse && noResponse && (aCommand.currentTry <= noResponseRetryAmount) && ((strcmp(aCommand.payload.c_str(), "REQUESTSETTINGS") == 0) || (strcmp(aCommand.payload.c_str(), "GETSETTINGS") == 0)
                           || (strcmp(aCommand.payload.c_str(), "MODEPRESS") == 0) || (strcmp(aCommand.payload.c_str(), "MODEPRESSINV") == 0) || (strcmp(aCommand.payload.c_str(), "MODESWITCH") == 0) || (strcmp(aCommand.payload.c_str(), "MODESWITCHINV") == 0) || isNum ))) {
                    if (printSerialOutputForDebugging) {
                      Serial.print("current retry...");
                      Serial.println(aCommand.currentTry);
                    }
                    requeue = true;
                    botsToWaitFor[aCommand.device] = true;
                    lastCommandWasBusy = false;
                    getSettingsAfter = false;
                  }
                  else {
                    std::map<std::string, bool>::iterator itP = botsToWaitFor.find(aCommand.device);
                    if (itP != botsToWaitFor.end())
                    {
                      botsToWaitFor.erase(aCommand.device);
                    }
                  }
                }
                waitForResponse = false;
                noResponse = false;
              }
              else if (isCurtainDevice(aCommand.device.c_str()))
              {
                controlMQTT(aCommand.device, aCommand.payload, disconnectAfter);
                std::string anAddr;
                if (getCurtainResponse) {
                  while (noResponse && shouldContinue )
                  {
                    waitForResponse = true;
                    if (printSerialOutputForDebugging) {
                      Serial.println("waiting for response...");
                    }
                    if ((millis() - timeSent) > (waitForResponseSec * 1000)) {
                      shouldContinue = false;
                    }
                  }
                  std::map<std::string, std::string>::iterator itN = allCurtains.find(aCommand.device);
                  anAddr = itN->second;
                  std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
                  NimBLEClient* pClient = nullptr;
                  if (NimBLEDevice::getClientListSize()) {
                    pClient = NimBLEDevice::getClientByPeerAddress(anAddr);
                    if (pClient) {
                      if (pClient->isConnected()) {
                        unsubscribeToNotify(pClient);
                        if (disconnectAfter) {
                          pClient->disconnect();
                        }
                      }
                    }
                  }
                  if (lastCommandWasBusy && retryCurtainOnBusy) {
                    requeue = true;
                    botsToWaitFor[aCommand.device] = true;
                    lastCommandWasBusy = false;
                    lastCommandSent[anAddr] = 0;
                  }
                  else if (retryCurtainNoResponse && noResponse && (aCommand.currentTry <= noResponseRetryAmount)) {
                    if (printSerialOutputForDebugging) {
                      Serial.print("current retry...");
                      Serial.println(aCommand.currentTry);
                    }
                    requeue = true;
                    botsToWaitFor[aCommand.device] = true;
                    lastCommandWasBusy = false;
                  }
                  else {
                    std::map<std::string, bool>::iterator itP = botsToWaitFor.find(aCommand.device);
                    if (itP != botsToWaitFor.end())
                    {
                      botsToWaitFor.erase(aCommand.device);
                    }
                  }
                }
              }
              waitForResponse = false;
              noResponse = false;
            }

            std::map<std::string, bool>::iterator itE = botsSimulateONOFFinPRESSmode.find(aCommand.device);
            if (itE != botsSimulateONOFFinPRESSmode.end())
            {
              std::map<std::string, bool>::iterator itF = botsSimulatedStates.find(aCommand.device);
              if (itF != botsSimulatedStates.end())
              {
                bool boolState = itF->second;
                if (boolState && (strcmp(aCommand.payload.c_str(), "OFF") == 0))  {
                  client.publish(deviceStateTopic.c_str(), "ON", true);
                }
                else if (!boolState && (strcmp(aCommand.payload.c_str(), "ON") == 0)) {
                  client.publish(deviceStateTopic.c_str(), "OFF", true);
                }
              }
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
          aCommand.currentTry = aCommand.currentTry + 1;
          aCommand.priority = true;
          commandQueue.enqueue(aCommand);
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
            client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"getsettings\"}");
            controlMQTT(requestDevice, "REQUESTSETTINGS", disconnectAfter);
            while (noResponse && shouldContinue )
            {
              waitForResponse = true;
              //if (printSerialOutputForDebugging) {Serial.println("waiting for response...");}
              if ((millis() - timeSent) > (waitForResponseSec * 1000)) {
                shouldContinue = false;
              }
            }
            waitForResponse = false;
          }
        }
        commandQueue.dequeue();
      }
    }
    client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"idle\"}");
  }
  if (ledOnCommand) {
    digitalWrite(LED_PIN, ledOFFValue);
  }
  processing = false;
  return true;
}

bool sendToDevice(NimBLEAdvertisedDevice * advDevice, std::string aName, const char * command, std::string deviceTopic, bool disconnectAfter) {
  bool isSuccess = false;
  NimBLEAdvertisedDevice* advDeviceToUse = advDevice;
  std::string addr = advDeviceToUse->getAddress().toString();
  //std::transform(addr.begin(), addr.end(), addr.begin(), ::toupper);
  addr = addr.c_str();
  std::string deviceStateTopic = deviceTopic + "/state";
  deviceTopic = deviceTopic + "/status";

  if ((advDeviceToUse != nullptr) && (advDeviceToUse != NULL))
  {
    //char aBuffer[100];
    StaticJsonDocument<100> doc;
    //    doc["id"] = aName.c_str();
    if (strcmp(command, "requestInfo") == 0 || strcmp(command, "REQUESTINFO") == 0 || strcmp(command, "GETINFO") == 0) {
      isSuccess = requestInfo(advDeviceToUse);
      if (!isSuccess) {
        doc["status"] = "errorRequestInfo";
        doc["command"] = command;
        serializeJson(doc, aBuffer);
        client.publish(deviceTopic.c_str(),  aBuffer);
      }
      return isSuccess;
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
        doc["command"] = command;
        serializeJson(doc, aBuffer);
        client.publish(deviceTopic.c_str(),  aBuffer);
      }
      else {
        if (count > tryConnecting) {
          shouldContinue = false;
          doc["status"] = "errorConnect";
          doc["command"] = command;
          serializeJson(doc, aBuffer);
          client.publish(deviceTopic.c_str(),  aBuffer);
        }
      }
    }
    count = 0;
    if (isConnected) {
      shouldContinue = true;
      while (shouldContinue) {
        if (count > 1) {
          delay(50);
        }
        isSuccess = sendCommand(advDeviceToUse, command, count, disconnectAfter);
        count++;
        if (isSuccess) {
          delay(100);
          shouldContinue = false;
          if (!lastCommandSentPublished) {
            StaticJsonDocument<100> doc;
            //char aBuffer[100];
            doc["status"] = "commandSent";
            doc["command"] = command;
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
                //char aBuffer[100];
                docPos["pos"] = aVal;
                serializeJson(docPos, aBuffer);
                client.publish(devicePosTopic.c_str(), aBuffer);
              }
            }
            else {
              std::map<std::string, bool>::iterator itP = botsInPressMode.find(addr);
              std::map<std::string, bool>::iterator itE = botsSimulateONOFFinPRESSmode.find(aDevice);
              if (itP != botsInPressMode.end() && itE == botsSimulateONOFFinPRESSmode.end())
              {
                client.publish(deviceStateTopic.c_str(), "OFF", true);
              }
              else {
                client.publish(deviceStateTopic.c_str(), command, true);
              }
            }
            if (scanAfterControl && scanAfterNum) {
              rescanTimes[addr] = millis();
            }
          }
        }
        else {
          if (count > trySending) {
            shouldContinue = false;
            doc["status"] = "errorCommand";
            doc["command"] = command;
            serializeJson(doc, aBuffer);
            client.publish(deviceTopic.c_str(),  aBuffer);
          }
        }
      }
    }
  }
  if (printSerialOutputForDebugging) {
    Serial.println("Done sendCommand...");
  }
  return isSuccess;
}

bool is_number(const std::string & s)
{
  std::string::const_iterator it = s.begin();
  while (it != s.end() && std::isdigit(*it)) ++it;
  return !s.empty() && it == s.end();
}

bool controlMQTT(std::string device, std::string payload, bool disconnectAfter) {
  client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"controlling\"}");
  bool isSuccess = false;
  processing = true;
  if (printSerialOutputForDebugging) {
    Serial.println("Processing Control MQTT...");
  }

  std::string deviceAddr = "";
  std::string deviceTopic;
  std::string anAddr;

  if (printSerialOutputForDebugging) {
    Serial.print("Device: ");
    Serial.println(device.c_str());
    Serial.print("Device value: ");
    Serial.println(payload.c_str());
  }

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
  itS = allContactSensors.find(device.c_str());
  if (itS != allContactSensors.end())
  {
    anAddr = itS->second;
    std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
    deviceAddr = anAddr.c_str();
    deviceTopic = contactTopic;
  }
  itS = allMotionSensors.find(device.c_str());
  if (itS != allMotionSensors.end())
  {
    anAddr = itS->second;
    std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
    deviceAddr = anAddr.c_str();
    deviceTopic = motionTopic;
  }

  bool diffDevice = false;
  if (lastDeviceControlled != deviceAddr) {
    diffDevice = true;
  }
  lastDeviceControlled = deviceAddr;
  if (lastDeviceControlled != "") {
    if (diffDevice) {
      NimBLEClient* pClient = nullptr;
      if (NimBLEDevice::getClientListSize()) {
        pClient = NimBLEDevice::getClientByPeerAddress(lastDeviceControlled);
        if (pClient) {
          if (pClient->isConnected()) {
            unsubscribeToNotify(pClient);
            pClient->disconnect();
          }
        }
      }
    }

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
      isSuccess = processRequest(deviceAddr, device, payload.c_str(), deviceTopic, disconnectAfter);
    }
    else {
      if ((strcmp(payload.c_str(), "PRESS") == 0) || (strcmp(payload.c_str(), "ON") == 0) || (strcmp(payload.c_str(), "OFF") == 0) || (strcmp(payload.c_str(), "OPEN") == 0) || (strcmp(payload.c_str(), "CLOSE") == 0) || (strcmp(payload.c_str(), "PAUSE") == 0)
          || (strcmp(payload.c_str(), "REQUESTSETTINGS") == 0) || (strcmp(payload.c_str(), "REQUESTINFO") == 0) || (strcmp(payload.c_str(), "GETSETTINGS") == 0) || (strcmp(payload.c_str(), "GETINFO") == 0)
          || (strcmp(payload.c_str(), "MODEPRESS") == 0) || (strcmp(payload.c_str(), "MODEPRESSINV") == 0) || (strcmp(payload.c_str(), "MODESWITCH") == 0) || (strcmp(payload.c_str(), "MODESWITCHINV") == 0)) {
        isSuccess = processRequest(deviceAddr, device, payload.c_str(), deviceTopic, disconnectAfter);
      }
      else {
        //char aBuffer[100];
        StaticJsonDocument<100> docOut;
        docOut["status"] = "errorJSONValue";
        serializeJson(docOut, aBuffer);
        if (printSerialOutputForDebugging) {
          Serial.println("Parsing failed = value not a valid command");
        }
        client.publish(ESPMQTTTopic.c_str(), aBuffer);
      }
    }
  }
  else {
    //char aBuffer[100];
    StaticJsonDocument<100> docOut;
    docOut["status"] = "errorJSONDevice";
    serializeJson(docOut, aBuffer);
    if (printSerialOutputForDebugging) {
      Serial.println("Parsing failed = device not from list");
    }
    client.publish(ESPMQTTTopic.c_str(), aBuffer);
  }

  delay(100);
  return isSuccess;
}

void performHoldPressSequence(std::string aDevice, std::string aCommand, int aHold ) {
  String holdString = String(aHold);
  struct QueueCommand queueCommandHold;
  queueCommandHold.payload = holdString.c_str();
  queueCommandHold.topic = ESPMQTTTopic + "/control";
  queueCommandHold.device = aDevice;
  queueCommandHold.currentTry = 1;
  queueCommandHold.priority = false;
  queueCommandHold.disconnectAfter = false;
  commandQueue.enqueue(queueCommandHold);
  delay(50);
  struct QueueCommand queueCommandPress;
  std::string aPress = aCommand;
  queueCommandPress.payload = aPress.c_str();
  queueCommandPress.topic = ESPMQTTTopic + "/control";
  queueCommandPress.device = aDevice;
  queueCommandPress.currentTry = 1;
  queueCommandPress.priority = false;
  queueCommandPress.disconnectAfter = true;
  commandQueue.enqueue(queueCommandPress);
}

void performHoldPress(std::string aDevice, int aHold) {
  performHoldPressSequence(aDevice, "PRESS", aHold );
}

void performHoldOn(std::string aDevice, int aHold) {
  performHoldPressSequence(aDevice, "ON", aHold );
}

void performHoldOff(std::string aDevice, int aHold) {
  performHoldPressSequence(aDevice, "OFF", aHold );
}

void rescanMQTT(std::string payload) {
  isRescanning = true;
  processing = true;
  if (printSerialOutputForDebugging) {
    Serial.println("Processing Rescan MQTT...");
  }
  StaticJsonDocument<100> docIn;
  deserializeJson(docIn, payload);

  if (docIn == nullptr) { //Check for errors in parsing
    if (printSerialOutputForDebugging) {
      Serial.println("Parsing failed");
    }
    //char aBuffer[100];
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
        //char aBuffer[100];
        StaticJsonDocument<100> docOut;
        docOut["status"] = "errorJSONValue";
        serializeJson(docOut, aBuffer);
        if (printSerialOutputForDebugging) {
          Serial.println("Parsing failed = device not from list");
        }
        client.publish(ESPMQTTTopic.c_str(), aBuffer);
      }
    }
  }
  processing = false;
}

void requestInfoMQTT(std::string payload) {
  processing = true;
  if (printSerialOutputForDebugging) {
    Serial.println("Processing Request Info MQTT...");
  }
  StaticJsonDocument<100> docIn;
  deserializeJson(docIn, payload);

  if (docIn == nullptr) { //Check for errors in parsing
    if (printSerialOutputForDebugging) {
      Serial.println("Parsing failed");
    }
    //char aBuffer[100];
    StaticJsonDocument<100> docOut;
    docOut["status"] = "errorParsingJSON";
    serializeJson(docOut, aBuffer);
    client.publish(ESPMQTTTopic.c_str(), aBuffer);
  }
  else {
    const char * aName = docIn["id"]; //Get sensor type value
    if (printSerialOutputForDebugging) {
      Serial.print("Device: ");
      Serial.println(aName);
    }

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
      itS = allContactSensors.find(aName);
      if (itS != allContactSensors.end())
      {
        anAddr = itS->second;
        std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
        deviceAddr = anAddr.c_str();
        deviceTopic = contactTopic;
      }
      itS = allMotionSensors.find(aName);
      if (itS != allMotionSensors.end())
      {
        anAddr = itS->second;
        std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
        deviceAddr = anAddr.c_str();
        deviceTopic = motionTopic;
      }
    }
    if (deviceAddr != "") {
      deviceTopic = deviceTopic + aName;
      processRequest(deviceAddr, aName, "requestInfo", deviceTopic, true);
    }
    else {
      //char aBuffer[100];
      StaticJsonDocument<100> docOut;
      docOut["status"] = "errorJSONId";
      serializeJson(docOut, aBuffer);
      if (printSerialOutputForDebugging) {
        Serial.println("Parsing failed = device not from list");
      }
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

    it = allBots.begin();
    while (it != allBots.end())
    {
      std::string deviceStr ;
      aDevice = it->first.c_str();
      client.subscribe((botTopic + aDevice + "/assumedstate").c_str(), [aDevice] (const String & payload)  {
        if (printSerialOutputForDebugging) {
          Serial.println("state MQTT Received (from retained)...updating ON/OFF simulate states");
        }

        if (isBotDevice(aDevice)) {
          std::map<std::string, std::string>::iterator itP = allBots.find(aDevice);
          if (itP != allBots.end())
          {
            std::string aMac = itP->second.c_str();
            std::map<std::string, bool>::iterator itZ = botsInPressMode.find(aMac);
            std::map<std::string, bool>::iterator itE = botsSimulateONOFFinPRESSmode.find(aDevice);
            std::map<std::string, bool>::iterator itI = botsSimulatedStates.find(aDevice);

            if (itE != botsSimulateONOFFinPRESSmode.end())
            {
              Serial.println("settings the value");
              if ((strcmp(payload.c_str(), "OFF") == 0)) {
                if (itI == botsSimulatedStates.end()) {
                  botsSimulatedStates[aDevice] = false;
                }

              } else if ((strcmp(payload.c_str(), "ON") == 0)) {
                if (itI == botsSimulatedStates.end()) {
                  botsSimulatedStates[aDevice] = true;
                }
              }
            }
          }
        }
        client.unsubscribe((botTopic + aDevice + "/assumedstate").c_str());
      });
      it++;
    }
  }
  client.publish(lastWill, "online", true);

  long startTime = millis();
  while (((millis() - startTime) > waitForMQTTRetainMessages * 1000) && (botsSimulatedStates.size() != botsSimulateONOFFinPRESSmode.size()))
  {
    delay(10);
  }

  it = allCurtains.begin();
  while (it != allCurtains.end())
  {
    std::string deviceStr ;
    aDevice = it->first.c_str();
    client.subscribe((curtainTopic + aDevice + "/set").c_str(), [aDevice] (const String & payload)  {
      if (printSerialOutputForDebugging) {
        Serial.println("Control MQTT Received...");
      }
      if (pScan->isScanning() || isRescanning) {
        if (pScan->isScanning()) {
          pScan->stop();
        }
        allSwitchbotsScanned = {};
        forceRescan = true;
        lastScanTimes = {};
      }
      if (!commandQueue.isFull()) {
        if (immediateCurtainStateUpdate && isCurtainDevice(aDevice)) {
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
              //char aBuffer[100];
              docPos["pos"] = aVal;
              serializeJson(docPos, aBuffer);
              client.publish(devicePosTopic.c_str(), aBuffer);
            }
            else if ((strcmp(payload.c_str(), "OPEN") == 0))  {
              client.publish(deviceStateTopic.c_str(), "OPEN", true);
            } else if ((strcmp(payload.c_str(), "CLOSE") == 0))  {
              client.publish(deviceStateTopic.c_str(), "CLOSE", true);
            } else if ((strcmp(payload.c_str(), "PAUSE") == 0)) {
              client.publish(deviceStateTopic.c_str(), "PAUSE", true);
            }
          }
        }
        struct QueueCommand queueCommand;
        queueCommand.payload = payload.c_str();
        queueCommand.topic = ESPMQTTTopic + "/control";
        queueCommand.device = aDevice;
        queueCommand.disconnectAfter = true;
        queueCommand.priority = false;
        queueCommand.currentTry = 1;
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
      if (printSerialOutputForDebugging) {
        Serial.println("Control MQTT Received...");
      }

      std::map<std::string, bool>::iterator itE = botsSimulateONOFFinPRESSmode.find(aDevice);
      std::string deviceStateTopic = botTopic + aDevice + "/state";
      if (itE != botsSimulateONOFFinPRESSmode.end() && ((strcmp(payload.c_str(), "STATEOFF") == 0) || (strcmp(payload.c_str(), "STATEON") == 0))) {
        if (strcmp(payload.c_str(), "STATEOFF") == 0) {
          botsSimulatedStates[aDevice] = false;
          client.publish(deviceStateTopic.c_str(), "OFF", true);
        }
        else if (strcmp(payload.c_str(), "STATEON") == 0) {
          botsSimulatedStates[aDevice] = true;
          client.publish(deviceStateTopic.c_str(), "ON", true);
        }
      }
      else {
        if (pScan->isScanning() || isRescanning) {
          if (pScan->isScanning()) {
            pScan->stop();
          }
          allSwitchbotsScanned = {};
          forceRescan = true;
          lastScanTimes = {};
        }
        if (!commandQueue.isFull()) {
          if (isBotDevice(aDevice)) {
            std::map<std::string, std::string>::iterator itP = allBots.find(aDevice);
            if (itP != allBots.end())
            {
              std::string aMac = itP->second.c_str();
              std::map<std::string, bool>::iterator itZ = botsInPressMode.find(aMac);
              std::map<std::string, bool>::iterator itE = botsSimulateONOFFinPRESSmode.find(aDevice);
              if (immediateBotStateUpdate) {
                if (itZ != botsInPressMode.end() && itE == botsSimulateONOFFinPRESSmode.end())
                {
                  client.publish(deviceStateTopic.c_str(), "OFF", true);
                }
                else {
                  if ((strcmp(payload.c_str(), "OFF") == 0)) {
                    client.publish(deviceStateTopic.c_str(), "OFF", true);
                  } else if ((strcmp(payload.c_str(), "ON") == 0)) {
                    client.publish(deviceStateTopic.c_str(), "ON", true);
                  }
                }
              }

              int aHold = -1;
              std::string commandString = "";
              if (itE != botsSimulateONOFFinPRESSmode.end())
              {
                if (strcmp(payload.c_str(), "OFF") == 0) {
                  commandString = "OFF";
                  std::map<std::string, int>::iterator itI = botsSimulatedOFFHoldTimes.find(aDevice);
                  if (itI != botsSimulatedOFFHoldTimes.end())
                  {
                    aHold = itI->second;
                  }
                }

                else if (strcmp(payload.c_str(), "ON") == 0) {
                  commandString = "ON";
                  std::map<std::string, int>::iterator itI = botsSimulatedONHoldTimes.find(aDevice);
                  if (itI != botsSimulatedONHoldTimes.end())
                  {
                    aHold = itI->second;
                  }
                }
              }
              if (aHold >= 0) {
                performHoldPressSequence(aDevice, commandString, aHold);
              }
              else {
                struct QueueCommand queueCommand;
                queueCommand.payload = payload.c_str();
                queueCommand.topic = ESPMQTTTopic + "/control";
                queueCommand.device = aDevice;
                queueCommand.disconnectAfter = true;
                queueCommand.priority = false;
                queueCommand.currentTry = 1;
                commandQueue.enqueue(queueCommand);
              }
            }
          }
        }
        else {
          client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"errorQueueFull\"}");
        }
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
      if (printSerialOutputForDebugging) {
        Serial.println("Control MQTT Received...");
      }
      bool skip = false;
      if (isRescanning) {
        if (pScan->isScanning() || isRescanning) {
          if (pScan->isScanning()) {
            pScan->stop();
          }
          allSwitchbotsScanned = {};
          forceRescan = true;
          lastScanTimes = {};
        }
        if ((strcmp(payload.c_str(), "REQUESTINFO") == 0) || (strcmp(payload.c_str(), "GETINFO") == 0)) {
          skip = true;
        }
      }
      if (!skip) {
        if (!commandQueue.isFull()) {
          struct QueueCommand queueCommand;
          queueCommand.payload = payload.c_str();
          queueCommand.topic = ESPMQTTTopic + "/control";
          queueCommand.device = aDevice;
          queueCommand.disconnectAfter = true;
          queueCommand.priority = false;
          queueCommand.currentTry = 1;
          commandQueue.enqueue(queueCommand);
        }
        else {
          client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"errorQueueFull\"}");
        }
      }
    });

    it++;
  }

  it = allContactSensors.begin();
  while (it != allContactSensors.end())
  {
    std::string deviceStr ;
    aDevice = it->first.c_str();
    client.subscribe((contactTopic + aDevice + "/set").c_str(), [aDevice] (const String & payload)  {
      if (printSerialOutputForDebugging) {
        Serial.println("Control MQTT Received...");
      }
      bool skip = false;
      if (isRescanning) {
        if (pScan->isScanning() || isRescanning) {
          if (pScan->isScanning()) {
            pScan->stop();
          }
          allSwitchbotsScanned = {};
          forceRescan = true;
          lastScanTimes = {};
        }
        if ((strcmp(payload.c_str(), "REQUESTINFO") == 0) || (strcmp(payload.c_str(), "GETINFO") == 0)) {
          skip = true;
        }
      }
      if (!skip) {
        if (!commandQueue.isFull()) {
          struct QueueCommand queueCommand;
          queueCommand.payload = payload.c_str();
          queueCommand.topic = ESPMQTTTopic + "/control";
          queueCommand.device = aDevice;
          queueCommand.disconnectAfter = true;
          queueCommand.priority = false;
          queueCommand.currentTry = 1;
          commandQueue.enqueue(queueCommand);
        }
        else {
          client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"errorQueueFull\"}");
        }
      }
    });

    it++;
  }

  it = allMotionSensors.begin();
  while (it != allMotionSensors.end())
  {
    std::string deviceStr ;
    aDevice = it->first.c_str();
    client.subscribe((motionTopic + aDevice + "/set").c_str(), [aDevice] (const String & payload)  {
      if (printSerialOutputForDebugging) {
        Serial.println("Control MQTT Received...");
      }
      bool skip = false;
      if (isRescanning) {
        if (pScan->isScanning() || isRescanning) {
          if (pScan->isScanning()) {
            pScan->stop();
          }
          allSwitchbotsScanned = {};
          forceRescan = true;
          lastScanTimes = {};
        }
        if ((strcmp(payload.c_str(), "REQUESTINFO") == 0) || (strcmp(payload.c_str(), "GETINFO") == 0)) {
          skip = true;
        }
      }
      if (!skip) {
        if (!commandQueue.isFull()) {
          struct QueueCommand queueCommand;
          queueCommand.payload = payload.c_str();
          queueCommand.topic = ESPMQTTTopic + "/control";
          queueCommand.device = aDevice;
          queueCommand.disconnectAfter = true;
          queueCommand.priority = false;
          queueCommand.currentTry = 1;
          commandQueue.enqueue(queueCommand);
        }
        else {
          client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"errorQueueFull\"}");
        }
      }
    });

    it++;
  }

  client.subscribe(requestInfoTopic, [] (const String & payload)  {
    if (printSerialOutputForDebugging) {
      Serial.println("Request Info MQTT Received...");
    }
    bool skip = false;
    if (isRescanning) {
      if (pScan->isScanning() || isRescanning) {
        if (pScan->isScanning()) {
          pScan->stop();
        }
        allSwitchbotsScanned = {};
        forceRescan = true;
        lastScanTimes = {};
      }
      skip = true;
    }
    if (!skip) {
      if (!commandQueue.isFull()) {
        struct QueueCommand queueCommand;
        queueCommand.payload = payload.c_str();
        queueCommand.topic = ESPMQTTTopic + "/requestInfo";
        queueCommand.disconnectAfter = true;
        queueCommand.priority = false;
        queueCommand.currentTry = 1;
        commandQueue.enqueue(queueCommand);
      }
      else {
        client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"errorQueueFull\"}");
      }
    }
  });

  client.subscribe(requestSettingsTopic, [] (const String & payload)  {
    if (printSerialOutputForDebugging) {
      Serial.println("Request Settings MQTT Received...");
    }
    if (!commandQueue.isFull()) {
      StaticJsonDocument<100> docIn;
      deserializeJson(docIn, payload.c_str());
      const char * aDevice = docIn["id"];
      struct QueueCommand queueCommand;
      queueCommand.payload = "REQUESTSETTINGS";
      queueCommand.topic = ESPMQTTTopic + "/control";
      queueCommand.device = aDevice;
      queueCommand.disconnectAfter = true;
      queueCommand.priority = false;
      queueCommand.currentTry = 1;
      commandQueue.enqueue(queueCommand);
    }
    else {
      client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"errorQueueFull\"}");
    }
  });

  client.subscribe(setModeTopic, [] (const String & payload)  {
    if (printSerialOutputForDebugging) {
      Serial.println("setMode  MQTT Received...");
    }
    if (!commandQueue.isFull()) {
      StaticJsonDocument<100> docIn;
      deserializeJson(docIn, payload.c_str());
      const char * aDevice = docIn["id"];
      const char * aMode = docIn["mode"];
      struct QueueCommand queueCommand;
      queueCommand.payload = aMode;
      queueCommand.topic = ESPMQTTTopic + "/control";
      queueCommand.device = aDevice;
      queueCommand.disconnectAfter = true;
      queueCommand.priority = false;
      queueCommand.currentTry = 1;
      commandQueue.enqueue(queueCommand);
    }
    else {
      client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"errorQueueFull\"}");
    }
  });

  client.subscribe(setHoldTopic, [] (const String & payload)  {
    if (printSerialOutputForDebugging) {
      Serial.println("setHold MQTT Received...");
    }
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
      queueCommand.disconnectAfter = true;
      queueCommand.priority = false;
      queueCommand.currentTry = 1;
      commandQueue.enqueue(queueCommand);
    }
    else {
      client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"errorQueueFull\"}");
    }
  });

  client.subscribe(holdPressTopic, [] (const String & payload)  {
    if (printSerialOutputForDebugging) {
      Serial.println("holdPress MQTT Received...");
    }
    if (!commandQueue.isFull()) {
      StaticJsonDocument<100> docIn;
      deserializeJson(docIn, payload.c_str());
      const char * aDevice = docIn["id"];
      int aHold = docIn["hold"];
      performHoldPress(aDevice, aHold);
    }
    else {
      client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"errorQueueFull\"}");
    }
  });

  client.subscribe(rescanTopic, [] (const String & payload)  {
    if (printSerialOutputForDebugging) {
      Serial.println("Rescan MQTT Received...");
    }

    bool skip = false;
    if (isRescanning) {
      if (pScan->isScanning() || isRescanning) {
        if (pScan->isScanning()) {
          pScan->stop();
        }
        allSwitchbotsScanned = {};
        forceRescan = true;
        lastScanTimes = {};
      }
      skip = true;
    }
    if (!skip) {

      if (!commandQueue.isFull()) {
        struct QueueCommand queueCommand;
        queueCommand.payload = payload.c_str();
        queueCommand.topic = ESPMQTTTopic + "/rescan";
        queueCommand.disconnectAfter = true;
        queueCommand.priority = false;
        queueCommand.currentTry = 1;
        commandQueue.enqueue(queueCommand);
      }
      else {
        client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"errorQueueFull\"}");
      }
    }
  });

  publishHomeAssistantDiscoveryESPConfig();
  discoveredDevices = {};

}

bool connectToServer(NimBLEAdvertisedDevice * advDeviceToUse) {
  if (printSerialOutputForDebugging) {
    Serial.println("Try to connect. Try a reconnect first...");
  }
  NimBLEClient* pClient = nullptr;
  if (NimBLEDevice::getClientListSize()) {

    pClient = NimBLEDevice::getClientByPeerAddress(advDeviceToUse->getAddress());
    if (pClient) {
      if (!pClient->connect(advDeviceToUse, false)) {
        if (printSerialOutputForDebugging) {
          Serial.println("Reconnect failed");
        }
      }
      else {
        if (printSerialOutputForDebugging) {
          Serial.println("Reconnected client");
        }
      }
    }
    else {
      pClient = NimBLEDevice::getDisconnectedClient();
    }
  }
  if (!pClient) {
    if (NimBLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS) {
      if (printSerialOutputForDebugging) {
        Serial.println("Max clients reached - no more connections available");
      }
      return false;
    }
    pClient = NimBLEDevice::createClient();
    if (printSerialOutputForDebugging) {
      Serial.println("New client created");
    }
    pClient->setClientCallbacks(&clientCB, false);
    pClient->setConnectionParams(12, 12, 0, 51);
    pClient->setConnectTimeout(10);

  }
  if (!pClient->isConnected()) {
    if (!pClient->connect(advDeviceToUse)) {
      NimBLEDevice::deleteClient(pClient);
      if (printSerialOutputForDebugging) {
        Serial.println("Failed to connect, deleted client");
      }
      return false;
    }
  }
  if (printSerialOutputForDebugging) {
    Serial.print("Connected to: ");
    Serial.println(pClient->getPeerAddress().toString().c_str());
    Serial.print("RSSI: ");
    Serial.println(pClient->getRssi());
  }
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

bool isBotDevice(std::string aDevice) {
  std::map<std::string, std::string>::iterator itS = allBots.find(aDevice);
  if (itS != allBots.end())
  {
    return true;
  }
  return false;
}

bool isCurtainDevice(std::string aDevice) {
  std::map<std::string, std::string>::iterator itS = allCurtains.find(aDevice);
  if (itS != allCurtains.end())
  {
    return true;
  }
  return false;
}

bool isMeterDevice(std::string aDevice) {
  std::map<std::string, std::string>::iterator itS = allMeters.find(aDevice);
  if (itS != allMeters.end())
  {
    return true;
  }
  return false;
}

bool isContactDevice(std::string aDevice) {
  std::map<std::string, std::string>::iterator itS = allContactSensors.find(aDevice);
  if (itS != allContactSensors.end())
  {
    return true;
  }
  return false;
}

bool isMotionDevice(std::string aDevice) {
  std::map<std::string, std::string>::iterator itS = allMotionSensors.find(aDevice);
  if (itS != allMotionSensors.end())
  {
    return true;
  }
  return false;
}

bool sendCommand(NimBLEAdvertisedDevice * advDeviceToUse, const char * type, int attempts, bool disconnectAfter) {
  if (advDeviceToUse == nullptr) {
    return false;
  }
  if (printSerialOutputForDebugging) {
    Serial.println("Sending command...");
  }
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
      if (printSerialOutputForDebugging) {
        Serial.println("Failed to connect for sending command");
      }
      return false;
    }
    count++;
    if (printSerialOutputForDebugging) {
      Serial.println("Attempt to send command. Not connecting. Try connecting...");
    }
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
        lastCommandSentStrings[anAddr] = type;
        if (isNum) {
          int aVal;
          sscanf(type, "%d", &aVal);
          if (isBotDevice(aDevice)) {
            skipWaitAfter = true;
            if (aPass == "") {
              if (printSerialOutputForDebugging) {
                Serial.println("Num is for a bot device - no pass");
              }
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
            if (printSerialOutputForDebugging) {
              Serial.print("Wrote new value to: ");
              Serial.println(pChr->getUUID().toString().c_str());
            }
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
      if (printSerialOutputForDebugging) {
        Serial.println("CUSTOM write service not found.");
      }
      returnValue = false;
    }
  }
  if (!returnValue) {
    if (attempts >= 10) {
      if (printSerialOutputForDebugging) {
        Serial.println("Sending failed. Disconnecting client");
      }
      pClient->disconnect();
    } return false;
  }
  if (disconnectAfter) {
    pClient->disconnect();
  }
  if (printSerialOutputForDebugging) {
    Serial.println("Success! Command sent/received to/from SwitchBot");
  }
  if (!skipWaitAfter) {
    lastCommandSent[anAddr] = millis();
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
      if (printSerialOutputForDebugging) {
        Serial.print(pChr->getUUID().toString().c_str());
        Serial.print(" Value: ");
        Serial.println(pChr->readValue().c_str());
      } // should return WoHand
      deviceTypes[advDeviceToUse->getAddress().toString().c_str()] = pChr->readValue().c_str();
      return true;
    }
  }
  return false;
}

bool requestInfo(NimBLEAdvertisedDevice * advDeviceToUse) {
  if (advDeviceToUse == nullptr) {
    return false;
  }
  if (printSerialOutputForDebugging) {
    Serial.println("Requesting info...");
  }
  rescanFind(advDeviceToUse->getAddress().toString().c_str());
  return true;
}

void notifyCB(NimBLERemoteCharacteristic * pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  if (printSerialOutputForDebugging) {
    Serial.println("notifyCB");
  }
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

  std::string aCommand = "";
  std::map<std::string, std::string>::iterator itH = lastCommandSentStrings.find(deviceMac);
  if (itH != lastCommandSentStrings.end())
  {
    aCommand = itH->second.c_str();
  }

  //char aBuffer[200];
  std::string deviceName;
  itS = deviceTypes.find(deviceMac.c_str());
  if (itS != deviceTypes.end())
  {
    deviceName = itS->second.c_str();
  }

  if (printSerialOutputForDebugging) {
    Serial.printf("deviceName: %s\n", deviceName.c_str());
  }

  if (deviceName == botName) {
    deviceStatusTopic = botTopic + aDevice + "/status";
    deviceSettingsTopic = botTopic + aDevice + "/settings";
    deviceAttrTopic = botTopic + aDevice + "/attributes";
    std::string deviceAssumedStateTopic = botTopic + aDevice + "/assumedstate";

    if (!lastCommandSentPublished) {
      StaticJsonDocument<60> statDoc;
      statDoc["status"] = "commandSent";
      statDoc["command"] = aCommand;
      serializeJson(statDoc, aBuffer);
      client.publish(deviceStatusTopic.c_str(), aBuffer);
      lastCommandSentPublished = true;
    }

    if (length == 1) {
      StaticJsonDocument<100> statDoc;
      uint8_t byte1 = pData[0];
      if (printSerialOutputForDebugging) {
        Serial.print("The response value from bot set mode or holdSecs: ");
        Serial.println(byte1);
      }
      if (byte1 == 3) {
        statDoc["status"] = "busy";
        lastCommandWasBusy = true;
      }
      //SUCCESS == 1 when setting hold secs or mode
      else if (byte1 == 1) {
        statDoc["status"] = "success";
        lastCommandWasBusy = false;
      }
      else if (byte1 == 6) {
        statDoc["status"] = "lowbattery";
        lastCommandWasBusy = false;
      }
      else {
        statDoc["status"] = "failed";
        lastCommandWasBusy = false;
      }
      statDoc["value"] = byte1;
      statDoc["command"] = aCommand;
      serializeJson(statDoc, aBuffer);
      client.publish(deviceStatusTopic.c_str(), aBuffer);
    }
    if (length == 3) {
      StaticJsonDocument<60> statDoc;
      uint8_t byte1 = pData[0];
      if (printSerialOutputForDebugging) {
        Serial.print("The response value from bot action: ");
        Serial.println(byte1);
      }
      if (byte1 == 3) {
        statDoc["status"] = "busy";
        lastCommandWasBusy = true;
      }
      //SUCCESS == 1 (on/off) or == 5 (press) for bot
      else if (byte1 == 1 || byte1 == 5) {
        statDoc["status"] = "success";
        lastCommandWasBusy = false;
        if (strcmp(aCommand.c_str(), "OFF") == 0) {
          client.publish(deviceAssumedStateTopic.c_str(), "OFF", true);
        }
        else if (strcmp(aCommand.c_str(), "ON") == 0) {
          client.publish(deviceAssumedStateTopic.c_str(), "ON", true);
        }
        std::map<std::string, bool>::iterator itE = botsSimulateONOFFinPRESSmode.find(aDevice.c_str());
        if (itE != botsSimulateONOFFinPRESSmode.end())
        {
          if (strcmp(aCommand.c_str(), "OFF") == 0) {
            botsSimulatedStates[aDevice] = false;
          }
          else if (strcmp(aCommand.c_str(), "ON") == 0) {
            botsSimulatedStates[aDevice] = true;
          }
          else if (strcmp(aCommand.c_str(), "PRESS") == 0) {
            botsSimulatedStates[aDevice] = !(botsSimulatedStates[aDevice]);
            client.publish(deviceAssumedStateTopic.c_str(), botsSimulatedStates[aDevice] ? "ON" : "OFF", true);
          }
        }
      }
      else if (byte1 == 6) {
        statDoc["status"] = "lowbattery";
        lastCommandWasBusy = false;
      }
      else {
        statDoc["status"] = "failed";
        lastCommandWasBusy = false;
      }
      statDoc["value"] = byte1;
      statDoc["command"] = aCommand;
      serializeJson(statDoc, aBuffer);
      client.publish(deviceStatusTopic.c_str(), aBuffer);
    }
    else if (length == 13) {
      StaticJsonDocument<50> statDoc;
      statDoc["status"] = "success";
      statDoc["command"] = aCommand;
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
              botsInPressMode[deviceMac] = true;
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

      botHoldSecs[deviceMac] = holdSecs;
    }
  }

  else if (deviceName == curtainName) {
    deviceStatusTopic = curtainTopic + aDevice + "/status";
    deviceSettingsTopic = curtainTopic + aDevice + "/settings";
    deviceAttrTopic = curtainTopic + aDevice + "/attributes";

    if (!lastCommandSentPublished) {
      StaticJsonDocument<50> statDoc;
      statDoc["status"] = "commandSent";
      statDoc["command"] = aCommand;
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

      if (printSerialOutputForDebugging) {
        Serial.print("The response value from curtain: ");
        Serial.println(byte1);
      }
      if (byte1 == 3) {
        statDoc["status"] = "busy";
        lastCommandWasBusy = true;
      }
      //SUCCESS == 1 or == 5 for curtain ????? just assuming based on bot
      else if (byte1 == 1 || byte1 == 5) {
        statDoc["status"] = "success";
        lastCommandWasBusy = false;
      }
      else if (byte1 == 6) {
        statDoc["status"] = "lowbattery";
        lastCommandWasBusy = false;
      }
      else {
        statDoc["status"] = "failed";
        lastCommandWasBusy = false;
      }
      statDoc["value"] = byte1;
      statDoc["command"] = aCommand;
      serializeJson(statDoc, aBuffer);
      client.publish(deviceStatusTopic.c_str(), aBuffer);
    }
  }

  NimBLEClient* pClient = nullptr;
  if (NimBLEDevice::getClientListSize()) {
    pClient = NimBLEDevice::getClientByPeerAddress(deviceMac);
    if (pClient) {
      if (pClient->isConnected()) {
        unsubscribeToNotify(pClient);
        //pClient->disconnect();
      }
    }
  }
}
