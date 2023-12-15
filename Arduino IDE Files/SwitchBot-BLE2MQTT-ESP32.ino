/** SwitchBot-MQTT-BLE-ESP32:

  https://github.com/devWaves/SwitchBot-MQTT-BLE-ESP32

  **does not use/require switchbot hub

  Code can be installed using Arduino IDE OR using Visual Studio Code PlatformIO
    -For Arduino IDE - Use only the SwitchBot-BLE2MQTT-ESP32.ino file
  -For Visual Studio Code PlatformIO - Use the src/SwitchBot-BLE2MQTT-ESP32.cpp and platformio.ini files
  Allows for "unlimited" switchbots devices to be controlled via MQTT sent to ESP32. ESP32 will send BLE commands to switchbots and return MQTT responses to the broker
     ** I do not know where performance will be affected by number of devices **
     ** This is an unofficial SwitchBot integration. User takes full responsibility with the use of this code **

  v7.1.5

    Updated: on Dec 13 2023
        Author: devWaves

        Contributions from:
                HardcoreWR
                vin-w
		iz8mbw
  		newsguytor

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

    - Curtain position will move as curtain moves when controlled from MQTT

    - Mesh 2 or more ESP32s together if you have motion or contact or meter. Mesh does not apply for bots or curtains

    - Active Scan (uses more battery of BLE devices) vs Passive Scan (uses less battery of BLE devices). Motion/Contact Sensors support Passive Scanning. Bot/Curtain/Meter require active scanning

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
        - "REQUESTSETTINGS" or "GETSETTINGS"                            (for bot only) Does the same thing as calling <ESPMQTTTopic>/requestSettings
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
  ESP32 will Subscribe to MQTT topic for device settings information
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

/* If using one ESP32 */
/* Enter all the switchbot device MAC addresses in the lists below */

/* If using multiple ESP32s - ESP32 can be meshed together for better motion/contact/meter performance */
/* Bot and Curtains and Plugs: (CANNOT BE MESHED) Enter the MAC addresses of the switchbot devices on the ESP32 closest to the switchbot device */
/* Motion and Contact and Meter: (CAN BE MESHED) Enter the MAC addresses of the switchbot devices into all or most of the ESP32s */

/* Wifi Settings */
static const char* host = "esp32";                                  //  Unique name for ESP32. The name detected by your router and MQTT. If you are using more then 1 ESPs to control different switchbots be sure to use unique hostnames. Host is the MQTT Client name and is used in MQTT topics
static const char* ssid = "SSID";                                   //  WIFI SSID
static const char* password = "Password";                           //  WIFI Password

/* MQTT Settings */
/* MQTT Client name is set to WIFI host from Wifi Settings*/
static const char* mqtt_host = "192.168.0.1";                       //  MQTT Broker server ip
static const char* mqtt_user = "switchbot";                         //  MQTT Broker username. If empty or NULL, no authentication will be used
static const char* mqtt_pass = "switchbot";                         //  MQTT Broker password
static const int mqtt_port = 1883;                                  //  MQTT Port
static const std::string mqtt_main_topic = "switchbot";             //  MQTT main topic

/* Mesh Settings */
/* Ignore if only one ESP32 is used */
static const bool enableMesh = false;                               // Ignore if only one ESP32 is used. Set to false
static const char* meshHost = "";                                   // Ignore if only one ESP32 is used. Ignore if you don't have either meter/contact/motion. Enter the host value of the primary ESP32 if you are using multiple esp32s and you want to mesh them together for better contact/motion
static const bool meshMeters = true;                                // Mesh meters together if meshHost is set. The meter values will use the meshHost MQTT topics
static const bool meshContactSensors = true;                        // Mesh contact sensors together if meshHost is set. The contact values will use the meshHost MQTT topics.
static const bool meshMotionSensors = true;                         // Mesh motion sensors together if meshHost is set. The motion values will use the meshHost MQTT topics
static const bool onlyAllowRootESPToPublishContact = true;          // All meshed messages for contact and motions sensors will pass through the root mesh host ESP32. Only the root host will send contact and motion messages
static const bool onlyAllowRootESPToPublishMotion = false;          // All meshed messages for motion will pass through the root mesh host ESP32. Only the root host will send motion messages
static const bool onlyAllowRootESPToPublishLight = false;           // All meshed messages for illuminance will pass through the root mesh host ESP32. Only the root host will send illuminance messages
static const bool countContactToAvoidDuplicates = true;             // count the number of open/close/timeout over all esp32s so that none are duplicated
static const bool countMotionToAvoidDuplicates = false;             // count the number of motion/no motion over all esp32s so that none are duplicated
static const bool countLightToAvoidDuplicates = false;              // count the number of dark/bright over all esp32s so that none are duplicated
static const int timeToIgnoreDuplicates = 30;                       // if a duplicate is determined, ignore it within X seconds


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

/* Switchbot Plug Mini Settings */
static std::map<std::string, std::string> allPlugs = {
  /*{ "plugone", "xX:xX:xX:xX:xX:xX" },
    { "plugtwo", "yY:yY:yY:yY:yY:yY" }*/
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
#ifndef LED_BUILTIN
	#define LED_BUILTIN 2                            // If your board doesn't have a defined LED_BUILTIN, replace 2 with the LED pin value
#endif
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
static const int tryConnecting = 60;                         // How many times to try connecting to bot
static const int trySending = 30;                            // How many times to try sending command to bot
static const int initialScan = 120;                          // How many seconds to scan for bots on ESP reboot and autoRescan. Once all devices are found scan stops, so you can set this to a big number
static const int infoScanTime = 60;                          // How many seconds to scan for single device status updates
static const int rescanTime = 10800;                         // Automatically perform a full active scan for device info of all devices every X seconds (default 3 hours). XXXXActiveScanSecs will also active scan on schedule
static const int queueSize = 50;                             // Max number of control/requestInfo/rescan MQTT commands stored in the queue. If you send more then queueSize, they will be ignored
static const int defaultBotWaitTime = 2;                     // wait at least X seconds between control command send to bots. ESP32 will detect if bot is in press mode with a hold time and will add hold time to this value per device
static const int defaultCurtainWaitTime = 0;                 // wait at least X seconds between control command send to curtains
static const int waitForResponseSec = 20;                    // How many seconds to wait for a bot/curtain response
static const int noResponseRetryAmount = 5;                  // How many times to retry if no response received
static const int defaultBotScanAfterControlSecs = 10;        // Default How many seconds to wait for state/status update call after set/control command. *override with botScanTime list
static const int defaultCurtainScanAfterControlSecs = 30;    // Default How many seconds to wait for state/status update call after set/control command. *override with botScanTime list. Also used by scanWhileCurtainIsMoving
static const int defaultBotMQTTUpdateSecs = 600;             // Used only when alwaysMQTTUpdate = false. Default MQTT Update for bot every X seconds. Note: a change in state will be always be published either way during active scanning
static const int defaultCurtainMQTTUpdateSecs = 600;         // Used only when alwaysMQTTUpdate = false. Default MQTT Update for curtain every X seconds.  Note: a change in state will be always be published either way during active scanning
static const int defaultMeterMQTTUpdateSecs = 600;           // Used only when alwaysMQTTUpdate = false. Default MQTT Update for meter temp sensors every X seconds. Note: a change in state will be always be published either way at all times (active or passive)
static const int defaultMotionMQTTUpdateSecs = 600;          // Used only when alwaysMQTTUpdate = false. Default MQTT Update for motion sensors every X seconds. Note: a change in state will be always be published either way at all times (active or passive)
static const int defaultContactMQTTUpdateSecs = 600;         // Used only when alwaysMQTTUpdate = false. Default MQTT Update for contact temp sensors every X seconds. Note: a change in state will be always be published either way at all times (active or passive)
static const int defaultPlugMQTTUpdateSecs = 600;            // Used only when alwaysMQTTUpdate = false. Default MQTT Update for motion sensors every X seconds. Note: a change in state will be always be published either way at all times (active or passive)
static const int defaultBotActiveScanSecs = 1800;            // Default Active Scan for bot every X seconds if not active scanned since X seconds. *override with botScanTime list.
static const int defaultCurtainActiveScanSecs = 1800;        // Default Active Scan for curtain every X seconds if not active scanned since X seconds. *override with botScanTime list.
static const int defaultMeterActiveScanSecs = 3600;          // Default Active Scan for meter temp sensors every X seconds if not active scanned since X seconds. *override with botScanTime list.
static const int defaultMotionActiveScanSecs = 3600;         // Default Active Scan for motion sensors every X seconds if not active scanned since X seconds. *override with botScanTime list.
static const int defaultContactActiveScanSecs = 3600;        // Default Active Scan for contact temp sensors every X seconds if not active scanned since X seconds. *override with botScanTime list.
static const int defaultPlugActiveScanSecs = 3600;           // Default Active Scan for motion sensors every X seconds if not active scanned since X seconds. *override with botScanTime list.
static const int waitForMQTTRetainMessages = 10;             // On boot ESP32 will look for retained MQTT state messages for X secs, otherwise default state is used. This is for bots in simulate ON/OFF, and when ESP32 mesh is used
static const int missedDataResend = 120;                     // Experimental. If a motion or contact is somehow missed while controlling bots, send the MQTT messages within X secs of it occuring as a backup. requires sendBackupMotionContact = true. Note: Not used if multiple ESP32s are meshed
static const int missedContactDelay = 30;                    // Experimental. If a contact is somehow missed while controlling bots, compare lastcontact from esp32 with contact sensor lastcontact. If different is greater than X, send a contact message. Note: Not used if multiple ESP32s are meshed
static const int missedMotionDelay = 30;                     // Experimental. If a motion is somehow missed while controlling bots, compare lastmotion from esp32 with motion/contact sensor lastmotion. If different is greater than X, send a motion message. Note: Not used if multiple ESP32s are meshed

static const bool sendBackupMotionContact = false;           // Experimental. Compares last contact/motion time value from switchbot contact/motion devices against what the esp32 received. If ESP32 missed one while controlling bots, it will send a motion/contact message after. Note: Not used if multiple ESP32s are meshed
static const bool autoRescan = true;                         // perform automatic rescan (uses rescanTime and initialScan).
static const bool activeScanOnSchedule = true;               // perform an active scan on decice types based on the scheduled seconds values for XXXXActiveScanSecs
static const bool scanAfterControl = true;                   // perform requestInfo after successful control command (uses botScanTime).
static const bool waitBetweenControl = true;                 // wait between commands sent to bot/curtain (avoids sending while bot is busy)
static const bool getSettingsOnBoot = true;                  // Currently only works for bot (curtain documentation not available but can probably be reverse engineered easily). Get bot extra settings values like firmware, holdSecs, inverted, number of timers. ***If holdSecs is available it is used by waitBetweenControl
static const bool retryBotOnBusy = true;                     // if bot responds with busy, the last control command will retry until success
static const bool retryCurtainOnBusy = true;                 // if curtain responds with busy, the last control command will retry until success
static const bool retryPlugOnBusy = true;                    // if plug responds with busy, the last control command will retry until success
static const bool retryBotActionNoResponse = false;          // Retry if bot doesn't send a response. Bot default is false because no response can still mean the bot triggered.
static const bool retryPlugActionNoResponse = true;          // Retry if plug doesn't send a response. Default is true. It shouldn't matter if plug receives the same command twice (or multiple times)
static const bool retryBotSetNoResponse = true;              // Retry if bot doesn't send a response when requesting settings (hold, firwmare etc) or settings hold/mode
static const bool retryCurtainNoResponse = true;             // Retry if curtain doesn't send a response. Default is true. It shouldn't matter if curtain receives the same command twice (or multiple times)
static const bool immediateBotStateUpdate = true;            // ESP32 will send ON/OFF state update as soon as MQTT is received. You can set this = false if not using Home Assistant Discovery.
static const bool immediatePlugStateUpdate = true;           // ESP32 will send ON/OFF state update as soon as MQTT is received. You can set this = false if not using Home Assistant Discovery.
static const bool immediateCurtainStateUpdate = true;        // ESP32 will send OPEN/CLOSE and Position state update as soon as MQTT is received. You can set this = false if not using Home Assistant Discovery.
static const bool assumeNoResponseMeansSuccess = true;       // Only for bots in simulated ON/OFF: If the ESP32 does not receive a response after sending command (after noResponseRetryAmount reached and retryBotActionNoResponse = true) assume it worked and change state
static const bool alwaysMQTTUpdate = false;                  // If the ESP32 is scanning, always publish MQTT data instead of using set times. ***Note: This creates a lot of MQTT traffic
static const bool onlyActiveScan = false;                    // Active scanning requires more battery from the BLE switchbot devices. If false, passive scanning is used when possible for contact/motion
static const bool onlyPassiveScan = false;                   // If this ESP32 is a mesh ESP32 or you only have motion/contact/meter sensors. Passive scanning uses less battery from BLE switchbot devices. Passive scanning provides less data then active scanning, but uses less battery
static const bool alwaysActiveScan = false;                  // No battery optimizations. If you are using the switchbot hub or app to control devices also and you want immediate state updates for bot and curtains in MQTT set to true
static const bool scanWhileCurtainIsMoving = true;           // The ESP32 will scan for defaultCurtainScanAfterControlSecs seconds after control to keep the position slider in sync with the actual position

static const bool printSerialOutputForDebugging = false;     // Only set to true when you want to debug an issue from Arduino IDE. Lots of Serial output from scanning can crash the ESP32
static bool manualDebugStartESP32WithMQTT = false;           // Only set to true when you want to debug an issue. ESP32 will boot in an OFF state when set to true. To start the ESP32 processing send any MQTT message to the topic ESPMQTTTopic/manualstart. This will make it easier to catch the last serial output of the ESP32 before crashing

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

static const String versionNum = "v7.1.5";

/*
   Server Index Page
*/
static const char* hostForScan = (meshHost == NULL || strlen(meshHost) < 1) ? host : meshHost;
static const char* hostForControl = host;

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
  "<center><font size=3>Primary Hostname/MQTT Client Name: " + std::string(hostForControl).c_str() + "</font></center>"
  "<center><font size=3>Mesh Hostname/MQTT Client Name: " + std::string(hostForScan).c_str() + "</font></center>"
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

static const uint16_t mqtt_packet_size = 1300;
static const bool home_assistant_discovery_set_up = false;
static const std::string manufacturer = "WonderLabs SwitchBot";
static const std::string curtainModel = "Curtain";
static const std::string curtainName = "WoCurtain";
static const std::string plugName = "WoPlug";
static const std::string plugModel = "Plug";
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
static bool isActiveScan = true;
static bool isMeshNode = false;
void scanEndedCB(NimBLEScanResults results);
void rescanEndedCB(NimBLEScanResults results);
void initialScanEndedCB(NimBLEScanResults results);
bool isBotDevice(std::string aDevice);
bool isPlugDevice(std::string aDevice);
bool isMeterDevice(std::string & aDevice);
bool isCurtainDevice(std::string aDevice);
bool isMotionDevice(std::string & aDevice);
bool isContactDevice(std::string & aDevice);
bool processQueue();
void startForeverScan();
void recurringMeterScan();
uint32_t getPassCRC(std::string & aDevice);
bool is_number(const std::string & s);
bool controlMQTT(std::string & device, std::string payload, bool disconnectAfter);
bool sendCommand(NimBLEAdvertisedDevice * advDeviceToUse, const char * type, int attempts, bool disconnectAfter);
bool sendToDevice(NimBLEAdvertisedDevice * advDevice, std::string & aName, const char * command, std::string & deviceTopic, bool disconnectAfter);
bool requestInfo(NimBLEAdvertisedDevice * advDeviceToUse);
bool connectToServer(NimBLEAdvertisedDevice * advDeviceToUse);
void rescanMQTT(std::string & payload);
void requestInfoMQTT(std::string & payload);
void recurringScan();
void recurringRescan();
void notifyCB(NimBLERemoteCharacteristic * pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify);
std::string getPass(std::string aDevice);
bool shouldMQTTUpdateForDevice(std::string & anAddr);
bool shouldMQTTUpdateOrActiveScanForDevice(std::string & anAddr);
bool shouldActiveScanForDevice(std::string & anAddr);
void processAdvData(std::string & deviceMac, long anRSSI,  std::string & aValueString, bool useActiveScan);
static std::map<std::string, NimBLEAdvertisedDevice*> allSwitchbotsDev = {};
static std::map<std::string, NimBLEAdvertisedDevice*> allSwitchbotsScanned = {};
static std::map<std::string, unsigned long> rescanTimes = {};
static std::map<std::string, unsigned long> lastUpdateTimes = {};
static std::map<std::string, unsigned long> lastActiveScanTimes = {};
static std::map<std::string, bool> botsSimulatedStates = {};
static std::map<std::string, std::string> motionStates = {};
static std::map<std::string, std::string> contactStates = {};
static std::map<std::string, std::string> plugStates = {};
static std::map<std::string, std::string> botStates = {};
static std::map<std::string, std::string> curtainStates = {};
static std::map<std::string, int> meterHumidStates = {};
static std::map<std::string, float> meterTempCStates = {};
static std::map<std::string, float> meterTempFStates = {};
static std::map<std::string, int> curtainPositionStates = {};
static std::map<std::string, int> curtainLightStates = {};
static std::map<std::string, long> plugPowerStates = {};
static std::map<std::string, bool> plugOverloadStates = {};
static std::map<std::string, std::string> contactMeshStates = {};
static std::map<std::string, std::string> lightMeshStates = {};
static std::map<std::string, std::string> motionMeshStates = {};
static std::map<std::string, unsigned long> lastMotions = {};
static std::map<std::string, unsigned long> lastContacts = {};
static std::map<std::string, unsigned long> lastButton = {};
static std::map<std::string, unsigned long> lastIn = {};
static std::map<std::string, unsigned long> lastOut = {};
static std::map<std::string, std::string> illuminanceStates = {};
static std::map<std::string, std::string> ledStates = {};
static std::map<std::string, int> outCounts = {};
static std::map<std::string, int> entranceCounts = {};
static std::map<std::string, int> buttonCounts = {};
static std::map<std::string, int> meshOpenCounts = {};
static std::map<std::string, int> meshClosedCounts = {};
static std::map<std::string, int> meshDarkCounts = {};
static std::map<std::string, int> meshBrightCounts = {};
static std::map<std::string, int> meshTimeoutCounts = {};
static std::map<std::string, int> meshMotionCounts = {};
static std::map<std::string, int> meshNoMotionCounts = {};

static std::map<std::string, int> updateMeshOpenCount = {};
static std::map<std::string, int> updateMeshClosedCount = {};
static std::map<std::string, int> updateMeshTimeoutCount = {};
static std::map<std::string, int> updateMeshDarkCount = {};
static std::map<std::string, int> updateMeshBrightCount = {};
static std::map<std::string, int> updateMeshMotionCount = {};
static std::map<std::string, int> updateMeshNoMotionCount = {};

static std::map<std::string, unsigned long> updateClosedCount = {};
static std::map<std::string, unsigned long> updateOpenCount = {};
static std::map<std::string, unsigned long> updateTimeoutCount = {};

static std::map<std::string, unsigned long> updateMotionCount = {};
static std::map<std::string, unsigned long> updateNoMotionCount = {};
static std::map<std::string, unsigned long> updateDarkCount = {};
static std::map<std::string, unsigned long> updateBrightCount = {};

static std::map<std::string, int> openCounts = {};
static std::map<std::string, int> closedCounts = {};
static std::map<std::string, int> darkCounts = {};
static std::map<std::string, int> brightCounts = {};
static std::map<std::string, int> timeoutCounts = {};
static std::map<std::string, int> motionCounts = {};
static std::map<std::string, int> noMotionCounts = {};
static std::map<std::string, int> batteryValues = {};
static std::map<std::string, std::string> lastCommandSentStrings = {};
static std::map<std::string, std::string> allSwitchbots;
static std::map<std::string, std::string> allSwitchbotsOpp;
static std::map<std::string, bool> discoveredDevices = {};
static std::map<std::string, bool> botsInPressMode = {};
static std::map<std::string, bool> botsToWaitFor = {};
static std::map<std::string, int> botHoldSecs = {};
static std::map<std::string, const char *> botFirmwares = {};
static std::map<std::string, int> botNumTimers = {};
static std::map<std::string, bool> botInverteds = {};
static std::map<std::string, unsigned long> lastCommandSent = {};
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
static bool overrideScan = false;
//static char aBuffer[120];
static const std::string ESPMQTTTopic = mqtt_main_topic + "/" + std::string(hostForControl);
static const std::string ESPMQTTTopicMesh = mqtt_main_topic + "/" + std::string(hostForScan);
static const std::string esp32Topic = ESPMQTTTopic + "/esp32";
static const std::string rssiStdStr = esp32Topic + "/rssi";
static const std::string lastWillStr = ESPMQTTTopic + "/lastwill";
static const std::string lastWillScanStr = ESPMQTTTopicMesh + "/lastwill";
static const char* lastWill = lastWillStr.c_str();
static const char* lastWillScan = lastWillScanStr.c_str();
static const std::string botTopic = ESPMQTTTopic + "/bot/";
static const std::string plugTopic = ESPMQTTTopic + "/plug/";
static const std::string curtainTopic = ESPMQTTTopic + "/curtain/";
static std::string meterTopic = ESPMQTTTopic + "/meter/";
static std::string contactTopic = ESPMQTTTopic + "/contact/";
static const std::string contactMainTopic = ESPMQTTTopic + "/contact/";
static const std::string motionMainTopic = ESPMQTTTopic + "/motion/";
static const std::string meterMainTopic = ESPMQTTTopic + "/meter/";
static std::string motionTopic = ESPMQTTTopic + "/motion/";
static const std::string rescanStdStr = ESPMQTTTopic + "/rescan";
static const std::string requestInfoStdStr = ESPMQTTTopic + "/requestInfo";
static const std::string requestSettingsStdStr = ESPMQTTTopic + "/requestSettings";
static const std::string setModeStdStr = ESPMQTTTopic + "/setMode";
static const std::string setHoldStdStr = ESPMQTTTopic + "/setHold";
static const std::string holdPressStdStr = ESPMQTTTopic + "/holdPress";
//static StaticJsonDocument<120> aJsonDoc;

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

struct QueuePublish {
  std::string topic;
  std::string payload;
  char * aBuffer;
  bool retain;
};

struct QueueAdvData {
  std::string macAddr;
  long rssi;
  std::string aValueString;
  std::string deviceType;
  bool useActiveScan;
};


static const int publishQueueSize = 300;
static const int advDataQueueSize = 300;

ArduinoQueue<QueueCommand> commandQueue(queueSize);

ArduinoQueue<QueuePublish> publishQueue(publishQueueSize);

ArduinoQueue<QueueAdvData> advDataQueue(advDataQueueSize);

void addToAdvDevData(std::string aMac, long anRSSI, std::string aString, bool shouldBeActive) {
  bool queueIsFull = advDataQueue.isFull();
  if (!queueIsFull) {
    struct QueueAdvData anAdvData;
    anAdvData.macAddr = aMac;
    anAdvData.rssi = anRSSI;
    anAdvData.aValueString = aString;
    anAdvData.useActiveScan = shouldBeActive;
    advDataQueue.enqueue(anAdvData);
  }
}

void addToPublish(std::string aTopic, std::string aPayload, bool retain) {
  bool queueIsFull = publishQueue.isFull();
  if (!queueIsFull) {
    struct QueuePublish aPublish;
    aPublish.payload = aPayload;
    aPublish.topic = aTopic;
    aPublish.retain = retain;
    publishQueue.enqueue(aPublish);
  }
}

void addToPublish(std::string aTopic, char * aPayload, bool retain) {
  bool queueIsFull = publishQueue.isFull();
  if (!queueIsFull) {
    struct QueuePublish aPublish;
    aPublish.payload = aPayload;
    aPublish.topic = aTopic;
    aPublish.retain = retain;
    publishQueue.enqueue(aPublish);
  }
}

void addToPublish(std::string aTopic, int aPayload, bool retain) {
  bool queueIsFull = publishQueue.isFull();
  if (!queueIsFull) {
    struct QueuePublish aPublish;
    aPublish.payload = (String(aPayload)).c_str();
    aPublish.topic = aTopic;
    aPublish.retain = retain;
    publishQueue.enqueue(aPublish);
  }
}

void addToPublish(std::string aTopic, long aPayload, bool retain) {
  bool queueIsFull = publishQueue.isFull();
  if (!queueIsFull) {
    struct QueuePublish aPublish;
    aPublish.payload = (String(aPayload)).c_str();
    aPublish.topic = aTopic;
    aPublish.retain = retain;
    publishQueue.enqueue(aPublish);
  }
}

void addToPublish(std::string aTopic, std::string aPayload) {
  bool queueIsFull = publishQueue.isFull();
  if (!queueIsFull) {
    struct QueuePublish aPublish;
    aPublish.payload = aPayload;
    aPublish.topic = aTopic;
    aPublish.retain = false;
    publishQueue.enqueue(aPublish);
  }
}

void addToPublish(std::string aTopic, char * aPayload) {
  bool queueIsFull = publishQueue.isFull();
  if (!queueIsFull) {
    struct QueuePublish aPublish;
    aPublish.payload = aPayload;
    aPublish.topic = aTopic;
    aPublish.retain = false;
    publishQueue.enqueue(aPublish);
  }
}

void printAString (const char * aString) {
  if (printSerialOutputForDebugging && !manualDebugStartESP32WithMQTT) {
    Serial.println(aString);
  }
}

void printAString (std::string & aString) {
  if (printSerialOutputForDebugging && !manualDebugStartESP32WithMQTT) {
    Serial.println(aString.c_str());
  }
}

void printAString (String & aString) {
  if (printSerialOutputForDebugging && !manualDebugStartESP32WithMQTT) {
    Serial.println(aString);
  }
}

void printAString (int aInt) {
  if (printSerialOutputForDebugging && !manualDebugStartESP32WithMQTT) {
    Serial.println(aInt);
  }
}

int le16_to_cpu_signed(const uint8_t data[2]) {
  unsigned value = data[0] | ((unsigned)data[1] << 8);
  if (value & 0x8000)
    return -(int)(~value) - 1;
  else
    return value;
}

void publishContactContact(std::string & aDevice, std::string & aValue) {
  addToPublish((contactTopic + aDevice + "/contact"), aValue, true);
}
void publishContactBinContact(std::string & aDevice, std::string & aValue) {
  addToPublish((contactTopic + aDevice + "/bin"), aValue, true);

}
void publishContactState(std::string & aDevice, std::string & aValue) {
  addToPublish((contactTopic + aDevice + "/state"), aValue, true);
}

void publishContactLastMotion(std::string & aDevice, long aValue) {
  addToPublish((contactTopic + aDevice + "/lastmotion"), aValue, true);
}

void publishContactLastContact(std::string & aDevice, long aValue) {
  addToPublish((contactTopic + aDevice + "/lastcontact"), aValue, true);
}

void publishMotionLastMotion(std::string & aDevice, long aValue) {
  addToPublish((motionTopic + aDevice + "/lastmotion"), aValue, true);
}

void processMotionMotion(std::string & aDevice, std::string & deviceMac, std::string & aValueString, bool isActive, bool aPublish) {

  bool aMotion = false;
  long lastMotionHighSeconds = 0;
  bool shouldPublish = aPublish;
  long lastMotion = -1;
  uint8_t byte1 = 0;
  uint8_t byte3 = 0;
  uint8_t byte4 = 0;
  uint8_t byte5 = 0;
  bool fakeLastMotion = false;

  if (isActive) {
    uint8_t byte1 = (uint8_t) aValueString[1];
    aMotion = (byte1 & 0b01000000);
    uint8_t byte3 = (uint8_t) aValueString[3];
    uint8_t byte4 = (uint8_t) aValueString[4];
    uint8_t byte5 = (uint8_t) aValueString[5];
    lastMotionHighSeconds = (byte5 & 0b10000000);

    byte data[] = {byte4, byte3};
    long lastMotionLowSeconds = le16_to_cpu_signed(data);
    lastMotion = lastMotionHighSeconds + lastMotionLowSeconds;
    std::map<std::string, unsigned long>::iterator itU = lastMotions.find(aDevice);
    itU = lastMotions.find(aDevice);
    if (itU == lastMotions.end())
    {
      lastMotions[aDevice] = millis();
    }

    if (!enableMesh && sendBackupMotionContact) {
      long theLastKnownMotion = millis() - lastMotions[aDevice];
      if (theLastKnownMotion < 0) {
        theLastKnownMotion = 0;
      }

      bool missedAMotion = (theLastKnownMotion > ((lastMotion * 1000) + (missedMotionDelay * 1000)));

      if (missedAMotion) {
        shouldPublish = true;
      }

      if ( missedAMotion && (lastMotion < missedDataResend) ) {
        aMotion = true;
      }
    }
  }
  else {
    byte1 = (uint8_t) aValueString[7];
    byte3 = (uint8_t) aValueString[9];
    byte5 = (uint8_t) aValueString[11];
    aMotion = (byte3 & 0b01000000);
    lastMotionHighSeconds = (byte5 & 0b10000000);

    if (aMotion) {
      fakeLastMotion = true;
      lastMotion = 0;
    }

  }

  if (aMotion) {
    lastMotions[aDevice] = millis();
  }

  std::string motion = aMotion ? "MOTION" : "NO MOTION";

  int motionCount = 0;
  int noMotionCount = 0;
  int meshMotionCount = 0;
  int meshNoMotionCount = 0;

  if (meshMotionSensors && enableMesh && countMotionToAvoidDuplicates) {
    std::map<std::string, int>::iterator itQQ = motionCounts.find(deviceMac);
    if (itQQ != motionCounts.end())
    {
      motionCount = itQQ->second;
    }

    itQQ = noMotionCounts.find(deviceMac);
    if (itQQ != noMotionCounts.end())
    {
      noMotionCount = itQQ->second;
    }

    itQQ = meshMotionCounts.find(deviceMac);
    if (itQQ != meshMotionCounts.end())
    {
      meshMotionCount = itQQ->second;
    }

    itQQ = meshNoMotionCounts.find(deviceMac);
    if (itQQ != meshNoMotionCounts.end())
    {
      meshNoMotionCount = itQQ->second;
    }

    if ((meshMotionCount == 0) || (meshNoMotionCount == 0)) {
      if (meshMotionCount == 0) {
        std::string deviceMotionMeshTopic = motionTopic + aDevice + "/motioncount";
        meshMotionCount = 1;
        motionCounts[deviceMac] = meshMotionCount;
        meshMotionCounts[deviceMac] = meshMotionCount;
        addToPublish(deviceMotionMeshTopic.c_str(), meshMotionCount, true);
      }
      if (meshNoMotionCount == 0) {
        std::string deviceNoMotionMeshTopic = motionTopic + aDevice + "/nomotioncount";
        meshNoMotionCount = 1;
        noMotionCounts[deviceMac] = meshNoMotionCount;
        meshNoMotionCounts[deviceMac] = meshNoMotionCount;
        addToPublish(deviceNoMotionMeshTopic.c_str(), meshNoMotionCount, true);
      }
    }
  }
  bool publishLastMotion = true;
  std::map<std::string, std::string>::iterator itH = motionStates.find(deviceMac.c_str());
  if (itH != motionStates.end())
  {
    std::string motionState = itH->second.c_str();
    if (strcmp(motionState.c_str(), motion.c_str()) != 0) {
      shouldPublish = true;
      if (meshMotionSensors && enableMesh && countMotionToAvoidDuplicates) {
        if (aMotion) {

          if ((meshMotionCount == motionCount) && (meshMotionCount != 0) && (meshNoMotionCount == noMotionCount)) {
            meshMotionCount = meshMotionCount + 1;
            if (meshMotionCount > 50) {
              meshMotionCount = 1;
            }
            meshMotionCounts[deviceMac.c_str()] = meshMotionCount;
            motionMeshStates[deviceMac.c_str()] = "MOTION";
            std::map<std::string, unsigned long>::iterator itW = updateMotionCount.find(deviceMac.c_str());
            if (itW != updateMotionCount.end())
            {
              updateMotionCount.erase(deviceMac.c_str());
            }
          }
          else {
            updateMotionCount[deviceMac.c_str()] = millis();
            updateMeshMotionCount[deviceMac.c_str()] = meshMotionCount;
            shouldPublish = false;
          }

          motionCount = motionCount + 1;
          if (motionCount > 50) {
            motionCount = 1;
          }
          motionCounts[deviceMac.c_str()] = motionCount;
        }
        else {
          if ((meshNoMotionCount == noMotionCount) && (meshNoMotionCount != 0) && (meshMotionCount == motionCount)) {
            meshNoMotionCount = meshNoMotionCount + 1;
            if (meshNoMotionCount > 50) {
              meshNoMotionCount = 1;
            }
            meshNoMotionCounts[deviceMac.c_str()] = meshNoMotionCount;
            motionMeshStates[deviceMac.c_str()] = "NO MOTION";
            std::map<std::string, unsigned long>::iterator itW = updateNoMotionCount.find(deviceMac.c_str());
            if (itW != updateNoMotionCount.end())
            {
              updateNoMotionCount.erase(deviceMac.c_str());
            }
          }
          else {
            updateNoMotionCount[deviceMac.c_str()] = millis();
            updateMeshNoMotionCount[deviceMac.c_str()] = meshNoMotionCount;
            shouldPublish = false;
          }

          noMotionCount = noMotionCount + 1;
          if (noMotionCount > 50) {
            noMotionCount = 1;
          }
          noMotionCounts[deviceMac.c_str()] = noMotionCount;
        }
      }
    }
    else {

      if (fakeLastMotion && (lastMotion == 0)) {
        publishLastMotion = false;
      }

      std::map<std::string, unsigned long>::iterator itW = updateMotionCount.find(deviceMac.c_str());
      if (itW != updateMotionCount.end())
      {
        shouldPublish = false;
      }
      itW = updateNoMotionCount.find(deviceMac.c_str());
      if (itW != updateNoMotionCount.end())
      {
        shouldPublish = false;
      }
    }
  }

  motionStates[deviceMac] = motion;

  if (((meshNoMotionCount != noMotionCount) || (meshMotionCount != motionCount)) && enableMesh && meshMotionSensors && countMotionToAvoidDuplicates) {
    shouldPublish = false;
  }

  if (shouldPublish) {
    if (meshMotionSensors && enableMesh && countMotionToAvoidDuplicates) {
      if (aMotion && (meshMotionCount != 0) && (meshMotionCount == motionCount)) {
        std::string deviceMotionMeshTopic = motionTopic + aDevice + "/motioncount";
        addToPublish(deviceMotionMeshTopic.c_str(), meshMotionCount, true);
      }
      else if (!aMotion && (meshNoMotionCount != 0) && (meshNoMotionCount == noMotionCount)) {
        std::string deviceNoMotionMeshTopic = motionTopic + aDevice + "/nomotioncount";
        addToPublish(deviceNoMotionMeshTopic.c_str(), meshNoMotionCount, true);
      }
    }

    if (!isMeshNode || !onlyAllowRootESPToPublishMotion) {
      std::string deviceMotionTopic = motionTopic + aDevice + "/motion";
      addToPublish(deviceMotionTopic.c_str(), motion.c_str(), true);
      std::string deviceStateTopic = motionTopic + aDevice + "/state";
      addToPublish(deviceStateTopic.c_str(), motion.c_str(), true);
      if (lastMotion >= 0 && publishLastMotion) {
        publishMotionLastMotion(aDevice, lastMotion);
      }
    }
  }
}

void processMotionContact(std::string & aDevice, std::string & deviceMac, std::string & aValueString, bool isActive, bool aPublish) {

  bool aMotion = false;
  long lastMotionHighSeconds = 0;
  bool shouldPublish = aPublish;
  long lastMotion = -1;
  uint8_t byte1 = 0;
  uint8_t byte3 = 0;
  uint8_t byte4 = 0;
  uint8_t byte5 = 0;
  bool fakeLastMotion = false;
  if (isActive) {
    byte1 = (uint8_t) aValueString[1];
    byte3 = (uint8_t) aValueString[3];
    byte4 = (uint8_t) aValueString[4];
    byte5 = (uint8_t) aValueString[5];
    aMotion = (byte1 & 0b01000000);
    lastMotionHighSeconds = (byte3 & 0b10000000);
    byte data[] = {byte5, byte4};
    long lastMotionLowSeconds = le16_to_cpu_signed(data);

    lastMotion = lastMotionHighSeconds + lastMotionLowSeconds;
    std::map<std::string, unsigned long>::iterator itU = lastMotions.find(aDevice);
    itU = lastMotions.find(aDevice);
    if (itU == lastMotions.end())
    {
      lastMotions[aDevice] = millis();
    }
    if (!enableMesh && sendBackupMotionContact) {
      long theLastKnownMotion = millis() - lastMotions[aDevice];
      if (theLastKnownMotion < 0) {
        theLastKnownMotion = 0;
      }

      bool missedAMotion = (theLastKnownMotion > ((lastMotion * 1000) + (missedMotionDelay * 1000)));
      if (missedAMotion) {
        shouldPublish = true;
      }

      if ( missedAMotion && (lastMotion < missedDataResend) ) {
        aMotion = true;
      }
    }
  }
  else {
    byte3 = (uint8_t) aValueString[9];
    aMotion = (byte3 & 0b10000000);
    lastMotionHighSeconds = (byte3 & 0b00000010);
    if (aMotion) {
      fakeLastMotion = true;
      lastMotion = 0;
    }
  }

  if (aMotion) {
    lastMotions[aDevice] = millis();
  }

  std::string motion = aMotion ? "MOTION" : "NO MOTION";

  int motionCount = 0;
  int noMotionCount = 0;
  int meshMotionCount = 0;
  int meshNoMotionCount = 0;

  if (meshContactSensors && enableMesh && countMotionToAvoidDuplicates) {
    std::map<std::string, int>::iterator itQQ = motionCounts.find(deviceMac);
    if (itQQ != motionCounts.end())
    {
      motionCount = itQQ->second;
    }

    itQQ = noMotionCounts.find(deviceMac);
    if (itQQ != noMotionCounts.end())
    {
      noMotionCount = itQQ->second;
    }

    itQQ = meshMotionCounts.find(deviceMac);
    if (itQQ != meshMotionCounts.end())
    {
      meshMotionCount = itQQ->second;
    }

    itQQ = meshNoMotionCounts.find(deviceMac);
    if (itQQ != meshNoMotionCounts.end())
    {
      meshNoMotionCount = itQQ->second;
    }

    if ((meshMotionCount == 0) || (meshNoMotionCount == 0)) {
      if (meshMotionCount == 0) {
        std::string deviceMotionMeshTopic = contactTopic + aDevice + "/motioncount";
        meshMotionCount = 1;
        motionCounts[deviceMac] = meshMotionCount;
        meshMotionCounts[deviceMac] = meshMotionCount;
        addToPublish(deviceMotionMeshTopic.c_str(), meshMotionCount, true);
      }
      if (meshNoMotionCount == 0) {
        std::string deviceNoMotionMeshTopic = contactTopic + aDevice + "/nomotioncount";
        meshNoMotionCount = 1;
        noMotionCounts[deviceMac] = meshNoMotionCount;
        meshNoMotionCounts[deviceMac] = meshNoMotionCount;
        addToPublish(deviceNoMotionMeshTopic.c_str(), meshNoMotionCount, true);
      }
    }
  }
  bool publishLastMotion = true;
  std::map<std::string, std::string>::iterator itH = motionStates.find(deviceMac.c_str());
  if (itH != motionStates.end())
  {
    std::string motionState = itH->second.c_str();
    if (strcmp(motionState.c_str(), motion.c_str()) != 0) {
      shouldPublish = true;
      if (meshContactSensors && enableMesh && countMotionToAvoidDuplicates) {
        if (aMotion) {

          if ((meshMotionCount == motionCount) && (meshMotionCount != 0) && (meshNoMotionCount == noMotionCount)) {
            meshMotionCount = meshMotionCount + 1;
            if (meshMotionCount > 50) {
              meshMotionCount = 1;
            }
            meshMotionCounts[deviceMac.c_str()] = meshMotionCount;
            motionMeshStates[deviceMac.c_str()] = "MOTION";
            std::map<std::string, unsigned long>::iterator itW = updateMotionCount.find(deviceMac.c_str());
            if (itW != updateMotionCount.end())
            {
              updateMotionCount.erase(deviceMac.c_str());
            }
          }
          else {
            updateMotionCount[deviceMac.c_str()] = millis();
            updateMeshMotionCount[deviceMac.c_str()] = meshMotionCount;
            shouldPublish = false;
          }

          motionCount = motionCount + 1;
          if (motionCount > 50) {
            motionCount = 1;
          }
          motionCounts[deviceMac.c_str()] = motionCount;
        }
        else {
          if ((meshNoMotionCount == noMotionCount) && (meshNoMotionCount != 0) && (meshMotionCount == motionCount)) {
            meshNoMotionCount = meshNoMotionCount + 1;
            if (meshNoMotionCount > 50) {
              meshNoMotionCount = 1;
            }
            meshNoMotionCounts[deviceMac.c_str()] = meshNoMotionCount;
            motionMeshStates[deviceMac.c_str()] = "NO MOTION";
            std::map<std::string, unsigned long>::iterator itW = updateNoMotionCount.find(deviceMac.c_str());
            if (itW != updateNoMotionCount.end())
            {
              updateNoMotionCount.erase(deviceMac.c_str());
            }
          }
          else {
            updateNoMotionCount[deviceMac.c_str()] = millis();
            updateMeshNoMotionCount[deviceMac.c_str()] = meshNoMotionCount;
            shouldPublish = false;
          }

          noMotionCount = noMotionCount + 1;
          if (noMotionCount > 50) {
            noMotionCount = 1;
          }
          noMotionCounts[deviceMac.c_str()] = noMotionCount;
        }
      }
    }
    else {

      if (fakeLastMotion && (lastMotion == 0)) {
        publishLastMotion = false;
      }

      std::map<std::string, unsigned long>::iterator itW = updateMotionCount.find(deviceMac.c_str());
      if (itW != updateMotionCount.end())
      {
        shouldPublish = false;
      }
      itW = updateNoMotionCount.find(deviceMac.c_str());
      if (itW != updateNoMotionCount.end())
      {
        shouldPublish = false;
      }
    }
  }

  motionStates[deviceMac] = motion;

  if (((meshNoMotionCount != noMotionCount) || (meshMotionCount != motionCount)) && enableMesh && meshContactSensors && countMotionToAvoidDuplicates) {
    shouldPublish = false;
  }

  if (shouldPublish) {
    if (meshContactSensors && enableMesh && countMotionToAvoidDuplicates) {
      if (aMotion && (meshMotionCount != 0) && (meshMotionCount == motionCount)) {
        std::string deviceMotionMeshTopic = contactTopic + aDevice + "/motioncount";
        addToPublish(deviceMotionMeshTopic.c_str(), meshMotionCount, true);
      }
      else if (!aMotion && (meshNoMotionCount != 0) && (meshNoMotionCount == noMotionCount)) {
        std::string deviceNoMotionMeshTopic = contactTopic + aDevice + "/nomotioncount";
        addToPublish(deviceNoMotionMeshTopic.c_str(), meshNoMotionCount, true);
      }
    }

    if (!isMeshNode || !onlyAllowRootESPToPublishMotion) {
      std::string deviceMotionTopic = contactTopic + aDevice + "/motion";
      addToPublish(deviceMotionTopic.c_str(), motion.c_str(), true);
      if (lastMotion >= 0 && publishLastMotion) {
        publishContactLastMotion(aDevice, lastMotion);
      }
    }
  }
}


void processLightMotion(std::string & aDevice, std::string & deviceMac, std::string & aValueString, bool isActive, bool aPublish) {

  bool shouldPublish = aPublish;
  bool lightA = false;
  bool lightB = false;
  if (isActive) {
    uint8_t byte5 = (uint8_t) aValueString[5];
    lightA = (byte5 & 0b00000010);
    lightB = (byte5 & 0b00000001);
  }
  else {
    uint8_t byte3 = (uint8_t) aValueString[9];
    lightA = (byte3 & 0b00100000);
    lightB = (byte3 & 0b00010000);
  }

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

  int darkCount = 0;
  int brightCount = 0;
  int meshDarkCount = 0;
  int meshBrightCount = 0;

  if (meshMotionSensors && enableMesh && countLightToAvoidDuplicates) {
    std::map<std::string, int>::iterator itQQ = darkCounts.find(deviceMac);
    if (itQQ != darkCounts.end())
    {
      darkCount = itQQ->second;
    }

    itQQ = brightCounts.find(deviceMac);
    if (itQQ != brightCounts.end())
    {
      brightCount = itQQ->second;
    }

    itQQ = meshDarkCounts.find(deviceMac);
    if (itQQ != meshDarkCounts.end())
    {
      meshDarkCount = itQQ->second;
    }

    itQQ = meshBrightCounts.find(deviceMac);
    if (itQQ != meshBrightCounts.end())
    {
      meshBrightCount = itQQ->second;
    }

    if ((meshDarkCount == 0) || (meshBrightCount == 0)) {
      if (meshDarkCount == 0) {
        std::string deviceDarkMeshTopic = motionTopic + aDevice + "/darkcount";
        meshDarkCount = 1;
        darkCounts[deviceMac] = meshDarkCount;
        meshDarkCounts[deviceMac] = meshDarkCount;
        addToPublish(deviceDarkMeshTopic.c_str(), meshDarkCount, true);
      }
      if (meshBrightCount == 0) {
        std::string deviceBrightMeshTopic = motionTopic + aDevice + "/brightcount";
        meshBrightCount = 1;
        brightCounts[deviceMac] = meshBrightCount;
        meshBrightCounts[deviceMac] = meshBrightCount;
        addToPublish(deviceBrightMeshTopic.c_str(), meshBrightCount, true);
      }
    }
  }

  std::map<std::string, std::string>::iterator itH = illuminanceStates.find(deviceMac.c_str());
  if (itH != illuminanceStates.end())
  {
    std::string illuminanceState = itH->second.c_str();
    if (strcmp(illuminanceState.c_str(), light.c_str()) != 0) {
      shouldPublish = true;
      if (meshMotionSensors && enableMesh && countLightToAvoidDuplicates) {
        if (strcmp(light.c_str(), "DARK") == 0) {

          if ((meshDarkCount == darkCount) && (meshDarkCount != 0) && (meshBrightCount == brightCount)) {
            meshDarkCount = meshDarkCount + 1;
            if (meshDarkCount > 50) {
              meshDarkCount = 1;
            }
            meshDarkCounts[deviceMac.c_str()] = meshDarkCount;
            lightMeshStates[deviceMac.c_str()] = "DARK";
            std::map<std::string, unsigned long>::iterator itW = updateDarkCount.find(deviceMac.c_str());
            if (itW != updateDarkCount.end())
            {
              updateDarkCount.erase(deviceMac.c_str());
            }
          }
          else {
            updateDarkCount[deviceMac.c_str()] = millis();
            updateMeshDarkCount[deviceMac.c_str()] = meshDarkCount;
            shouldPublish = false;
          }

          darkCount = darkCount + 1;
          if (darkCount > 50) {
            darkCount = 1;
          }
          darkCounts[deviceMac.c_str()] = darkCount;
        }
        else if (strcmp(light.c_str(), "BRIGHT") == 0) {
          if ((meshBrightCount == brightCount) && (meshBrightCount != 0) && (meshDarkCount == darkCount)) {
            meshBrightCount = meshBrightCount + 1;
            if (meshBrightCount > 50) {
              meshBrightCount = 1;
            }
            meshBrightCounts[deviceMac.c_str()] = meshBrightCount;
            lightMeshStates[deviceMac.c_str()] = "BRIGHT";
            std::map<std::string, unsigned long>::iterator itW = updateBrightCount.find(deviceMac.c_str());
            if (itW != updateBrightCount.end())
            {
              updateBrightCount.erase(deviceMac.c_str());
            }
          }
          else {
            updateBrightCount[deviceMac.c_str()] = millis();
            updateMeshBrightCount[deviceMac.c_str()] = meshBrightCount;
            shouldPublish = false;
          }

          brightCount = brightCount + 1;
          if (brightCount > 50) {
            brightCount = 1;
          }
          brightCounts[deviceMac.c_str()] = brightCount;
        }
      }
    }
    else {
      std::map<std::string, unsigned long>::iterator itW = updateDarkCount.find(deviceMac.c_str());
      if (itW != updateDarkCount.end())
      {
        shouldPublish = false;
      }
      itW = updateBrightCount.find(deviceMac.c_str());
      if (itW != updateBrightCount.end())
      {
        shouldPublish = false;
      }
    }
  }

  illuminanceStates[deviceMac] = light;

  if (((meshBrightCount != brightCount) || (meshDarkCount != darkCount)) && enableMesh && meshMotionSensors && countLightToAvoidDuplicates) {
    shouldPublish = false;
  }

  if (shouldPublish) {
    if (meshMotionSensors && enableMesh && countLightToAvoidDuplicates) {
      if ((strcmp(light.c_str(), "DARK") == 0) && (meshDarkCount != 0) && (meshDarkCount == darkCount)) {
        std::string deviceDarkMeshTopic = motionTopic + aDevice + "/darkcount";
        addToPublish(deviceDarkMeshTopic.c_str(), meshDarkCount, true);
      }
      else if ((strcmp(light.c_str(), "BRIGHT") == 0) && (meshBrightCount != 0) && (meshBrightCount == brightCount)) {
        std::string deviceBrightMeshTopic = motionTopic + aDevice + "/brightcount";
        addToPublish(deviceBrightMeshTopic.c_str(), meshBrightCount, true);
      }
    }

    if (!isMeshNode || !onlyAllowRootESPToPublishLight) {
      std::string deviceLightTopic = motionTopic + aDevice + "/illuminance";
      addToPublish(deviceLightTopic.c_str(), light.c_str(), true);
    }
  }
}


void processLightContact(std::string & aDevice, std::string & deviceMac, std::string & aValueString, bool isActive, bool aPublish) {
  bool shouldPublish = aPublish;
  std::string light = "";
  int battLevel = 0;
  uint8_t byte3 = 0;
  if (isActive) {
    byte3 = (uint8_t) aValueString[3];
    light = (byte3 & 0b00000001) ? "BRIGHT" : "DARK";
  }
  else
  {
    byte3 = (uint8_t) aValueString[9];
    light = (byte3 & 0b01000000) ? "BRIGHT" : "DARK";

  }

  int darkCount = 0;
  int brightCount = 0;
  int meshDarkCount = 0;
  int meshBrightCount = 0;

  if (meshContactSensors && enableMesh && countLightToAvoidDuplicates) {
    std::map<std::string, int>::iterator itQQ = darkCounts.find(deviceMac);
    if (itQQ != darkCounts.end())
    {
      darkCount = itQQ->second;
    }

    itQQ = brightCounts.find(deviceMac);
    if (itQQ != brightCounts.end())
    {
      brightCount = itQQ->second;
    }

    itQQ = meshDarkCounts.find(deviceMac);
    if (itQQ != meshDarkCounts.end())
    {
      meshDarkCount = itQQ->second;
    }

    itQQ = meshBrightCounts.find(deviceMac);
    if (itQQ != meshBrightCounts.end())
    {
      meshBrightCount = itQQ->second;
    }

    if ((meshDarkCount == 0) || (meshBrightCount == 0)) {
      if (meshDarkCount == 0) {
        std::string deviceDarkMeshTopic = contactTopic + aDevice + "/darkcount";
        meshDarkCount = 1;
        darkCounts[deviceMac] = meshDarkCount;
        meshDarkCounts[deviceMac] = meshDarkCount;
        addToPublish(deviceDarkMeshTopic.c_str(), meshDarkCount, true);
      }
      if (meshBrightCount == 0) {
        std::string deviceBrightMeshTopic = contactTopic + aDevice + "/brightcount";
        meshBrightCount = 1;
        brightCounts[deviceMac] = meshBrightCount;
        meshBrightCounts[deviceMac] = meshBrightCount;
        addToPublish(deviceBrightMeshTopic.c_str(), meshBrightCount, true);
      }
    }
  }

  std::map<std::string, std::string>::iterator itH = illuminanceStates.find(deviceMac.c_str());
  if (itH != illuminanceStates.end())
  {
    std::string illuminanceState = itH->second.c_str();
    if (strcmp(illuminanceState.c_str(), light.c_str()) != 0) {
      shouldPublish = true;
      if (meshContactSensors && enableMesh && countLightToAvoidDuplicates) {
        if (strcmp(light.c_str(), "DARK") == 0) {

          if ((meshDarkCount == darkCount) && (meshDarkCount != 0) && (meshBrightCount == brightCount)) {
            meshDarkCount = meshDarkCount + 1;
            if (meshDarkCount > 50) {
              meshDarkCount = 1;
            }
            meshDarkCounts[deviceMac.c_str()] = meshDarkCount;
            lightMeshStates[deviceMac.c_str()] = "DARK";
            std::map<std::string, unsigned long>::iterator itW = updateDarkCount.find(deviceMac.c_str());
            if (itW != updateDarkCount.end())
            {
              updateDarkCount.erase(deviceMac.c_str());
            }
          }
          else {
            updateDarkCount[deviceMac.c_str()] = millis();
            updateMeshDarkCount[deviceMac.c_str()] = meshDarkCount;
            shouldPublish = false;
          }

          darkCount = darkCount + 1;
          if (darkCount > 50) {
            darkCount = 1;
          }
          darkCounts[deviceMac.c_str()] = darkCount;
        }
        else if (strcmp(light.c_str(), "BRIGHT") == 0) {
          if ((meshBrightCount == brightCount) && (meshBrightCount != 0) && (meshDarkCount == darkCount)) {
            meshBrightCount = meshBrightCount + 1;
            if (meshBrightCount > 50) {
              meshBrightCount = 1;
            }
            meshBrightCounts[deviceMac.c_str()] = meshBrightCount;
            lightMeshStates[deviceMac.c_str()] = "BRIGHT";
            std::map<std::string, unsigned long>::iterator itW = updateBrightCount.find(deviceMac.c_str());
            if (itW != updateBrightCount.end())
            {
              updateBrightCount.erase(deviceMac.c_str());
            }
          }
          else {
            updateBrightCount[deviceMac.c_str()] = millis();
            updateMeshBrightCount[deviceMac.c_str()] = meshBrightCount;
            shouldPublish = false;
          }

          brightCount = brightCount + 1;
          if (brightCount > 50) {
            brightCount = 1;
          }
          brightCounts[deviceMac.c_str()] = brightCount;
        }
      }
    }
    else {
      std::map<std::string, unsigned long>::iterator itW = updateDarkCount.find(deviceMac.c_str());
      if (itW != updateDarkCount.end())
      {
        shouldPublish = false;
      }
      itW = updateBrightCount.find(deviceMac.c_str());
      if (itW != updateBrightCount.end())
      {
        shouldPublish = false;
      }
    }
  }

  illuminanceStates[deviceMac] = light;

  if (((meshBrightCount != brightCount) || (meshDarkCount != darkCount)) && enableMesh && meshContactSensors && countLightToAvoidDuplicates) {
    shouldPublish = false;
  }

  if (shouldPublish) {
    if (meshContactSensors && enableMesh && countLightToAvoidDuplicates) {
      if ((strcmp(light.c_str(), "DARK") == 0) && (meshDarkCount != 0) && (meshDarkCount == darkCount)) {
        std::string deviceDarkMeshTopic = contactTopic + aDevice + "/darkcount";
        addToPublish(deviceDarkMeshTopic.c_str(), meshDarkCount, true);
      }
      else if ((strcmp(light.c_str(), "BRIGHT") == 0) && (meshBrightCount != 0) && (meshBrightCount == brightCount)) {
        std::string deviceBrightMeshTopic = contactTopic + aDevice + "/brightcount";
        addToPublish(deviceBrightMeshTopic.c_str(), meshBrightCount, true);
      }
    }

    if (!isMeshNode || !onlyAllowRootESPToPublishLight) {
      std::string deviceLightTopic = contactTopic + aDevice + "/illuminance";
      addToPublish(deviceLightTopic.c_str(), light.c_str(), true);
    }
  }
}

void processSenseDistance(std::string & aDevice, std::string & deviceMac, std::string & aValueString, bool isActive, bool aPublish) {

  if (!isActive) {
    return;
  }
  bool shouldPublish = aPublish;
  uint8_t byte5 = (uint8_t) aValueString[5];
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
  //aJsonDoc["sensedist"] = sensingDistance;
  if (shouldPublish) {
    std::string deviceSenseTopic = motionTopic + aDevice + "/sensedist";
    addToPublish(deviceSenseTopic.c_str(), sensingDistance.c_str(), true);
  }
}


void processLED(std::string & aDevice, std::string & deviceMac, std::string & aValueString, bool isActive, bool aPublish) {
  if (!isActive) {
    return;
  }
  bool shouldPublish = aPublish;
  uint8_t byte5 = (uint8_t) aValueString[5];
  std::string ledState = (byte5 & 0b00100000) ? "ON" : "OFF";
  //aJsonDoc["led"] = ledState;

  std::map<std::string, std::string>::iterator itL = ledStates.find(deviceMac);
  if (itL != ledStates.end())
  {
    std::string aLED = itL->second.c_str();
    if (strcmp(aLED.c_str(), ledState.c_str()) != 0) {
      shouldPublish = true;
    }
  }
  ledStates[deviceMac] = ledState;

  if (shouldPublish) {
    std::string deviceLEDTopic = motionTopic + aDevice + "/led";
    addToPublish(deviceLEDTopic.c_str(), ledState.c_str(), true);
  }
}

void processContact(std::string & aDevice, std::string & deviceMac, std::string & aValueString, bool isActive, bool aPublish) {
  bool contactA = false;
  bool contactB = false;
  long lastContactHighSeconds = 0;
  bool shouldPublish = aPublish;
  uint8_t byte3 = 0;
  uint8_t byte6 = 0;
  uint8_t byte7 = 0;
  std::string light = "";
  int battLevel = 0;

  if ( isActive) {
    byte3 = (uint8_t) aValueString[3];
    byte6 = (uint8_t) aValueString[6];
    byte7 = (uint8_t) aValueString[7];
    contactA = (byte3 & 0b00000100);
    contactB = (byte3 & 0b00000010);
    lastContactHighSeconds = (byte3 & 0b01000000);
  }
  else
  {
    byte3 = (uint8_t) aValueString[9];
    byte6 = (uint8_t) aValueString[12];
    byte7 = (uint8_t) aValueString[13];
    contactA = (byte3 & 0b00100000);
    contactB = (byte3 & 0b00010000);
    lastContactHighSeconds = (byte3 & 0b00000001);
  }

  byte data2[] = {byte7, byte6};
  long lastContactLowSeconds = le16_to_cpu_signed(data2);

  long lastContact = lastContactHighSeconds + lastContactLowSeconds;

  std::map<std::string, unsigned long>::iterator itU = lastContacts.find(aDevice.c_str());
  if (itU == lastContacts.end())
  {
    lastContacts[aDevice.c_str()] = millis();
  }

  std::string contact;
  std::string binContact;
  if (!contactA && !contactB) {
    contact = "CLOSED";
  }
  else if (!contactA && contactB) {
    contact = "OPEN";
    lastContacts[aDevice] = millis();
  }
  else if (contactA && !contactB) {
    contact = "TIMEOUT";
    lastContacts[aDevice] = millis();
  }
  else if (contactA && contactB) {
    contact = "RESERVE";
  }

  if (!enableMesh && sendBackupMotionContact) {
    long theLastKnownContact = millis() - lastContacts[aDevice];
    if (theLastKnownContact < 0) {
      theLastKnownContact = 0;
    }

    bool missedAContact = (theLastKnownContact > ((lastContact * 1000) + (missedContactDelay * 1000)));
    if (missedAContact) {
      shouldPublish = true;
    }

    if ( missedAContact && (lastContact < missedDataResend) ) {
      contact = "OPEN";
      lastContacts[aDevice.c_str()] = millis();
    }
  }

  if (strcmp(contact.c_str(), "CLOSED") == 0) {
    binContact = "CLOSED";
  }
  else {
    binContact = "OPEN";
  }

  int openCount = 0;
  int closedCount = 0;
  int timeoutCount = 0;
  int meshOpenCount = 0;
  int meshClosedCount = 0;
  int meshTimeoutCount = 0;

  if (meshContactSensors && enableMesh && countContactToAvoidDuplicates) {
    std::map<std::string, int>::iterator itQQ = openCounts.find(deviceMac.c_str());
    if (itQQ != openCounts.end())
    {
      openCount = itQQ->second;
    }

    itQQ = closedCounts.find(deviceMac.c_str());
    if (itQQ != closedCounts.end())
    {
      closedCount = itQQ->second;
    }

    itQQ = timeoutCounts.find(deviceMac.c_str());
    if (itQQ != timeoutCounts.end())
    {
      timeoutCount = itQQ->second;
    }

    itQQ = meshOpenCounts.find(deviceMac.c_str());
    if (itQQ != meshOpenCounts.end())
    {
      meshOpenCount = itQQ->second;
    }

    itQQ = meshClosedCounts.find(deviceMac.c_str());
    if (itQQ != meshClosedCounts.end())
    {
      meshClosedCount = itQQ->second;
    }

    itQQ = meshTimeoutCounts.find(deviceMac.c_str());
    if (itQQ != meshTimeoutCounts.end())
    {
      meshTimeoutCount = itQQ->second;
    }

    if ((meshOpenCount == 0) || (meshClosedCount == 0) || (meshTimeoutCount == 0)) {
      if (meshOpenCount == 0) {
        std::string deviceOpenMeshTopic = contactTopic + aDevice + "/opencount";
        meshOpenCount = 1;
        openCounts[deviceMac.c_str()] = meshOpenCount;
        meshOpenCounts[deviceMac.c_str()] = meshOpenCount;
        addToPublish(deviceOpenMeshTopic.c_str(), meshOpenCount, true);
      }
      if (meshClosedCount == 0) {
        std::string deviceClosedMeshTopic = contactTopic + aDevice + "/closedcount";
        meshClosedCount = 1;
        closedCounts[deviceMac.c_str()] = meshClosedCount;
        meshClosedCounts[deviceMac.c_str()] = meshClosedCount;
        addToPublish(deviceClosedMeshTopic.c_str(), meshClosedCount, true);
      }
      if (meshTimeoutCount == 0) {
        std::string deviceTimeoutMeshTopic = contactTopic + aDevice + "/timeoutcount";
        meshTimeoutCount = 1;
        timeoutCounts[deviceMac.c_str()] = meshTimeoutCount;
        meshTimeoutCounts[deviceMac.c_str()] = meshTimeoutCount;
        addToPublish(deviceTimeoutMeshTopic.c_str(), meshTimeoutCount, true);
      }
    }
  }

  std::map<std::string, std::string>::iterator itH = contactStates.find(deviceMac.c_str());
  if (itH != contactStates.end())
  {
    std::string contactState = itH->second.c_str();
    if (strcmp(contactState.c_str(), contact.c_str()) != 0) {
      shouldPublish = true;
      if (meshContactSensors && enableMesh && countContactToAvoidDuplicates) {
        if (strcmp(contact.c_str(), "OPEN") == 0) {

          if ((meshOpenCount == openCount) && (meshOpenCount != 0) && (meshTimeoutCount == timeoutCount) && (meshClosedCount == closedCount)) {
            meshOpenCount = meshOpenCount + 1;
            if (meshOpenCount > 50) {
              meshOpenCount = 1;
            }
            meshOpenCounts[deviceMac.c_str()] = meshOpenCount;
            contactMeshStates[deviceMac.c_str()] = "OPEN";
            std::map<std::string, unsigned long>::iterator itW = updateOpenCount.find(deviceMac.c_str());
            if (itW != updateOpenCount.end())
            {
              updateOpenCount.erase(deviceMac.c_str());
            }
          }
          else {
            updateOpenCount[deviceMac.c_str()] = millis();
            updateMeshOpenCount[deviceMac.c_str()] = meshOpenCount;
            shouldPublish = false;
          }

          openCount = openCount + 1;
          if (openCount > 50) {
            openCount = 1;
          }
          openCounts[deviceMac.c_str()] = openCount;
        }
        else if (strcmp(contact.c_str(), "CLOSED") == 0) {
          if ((meshClosedCount == closedCount) && (meshClosedCount != 0) && (meshTimeoutCount == timeoutCount) && (meshOpenCount == openCount)) {
            meshClosedCount = meshClosedCount + 1;
            if (meshClosedCount > 50) {
              meshClosedCount = 1;
            }
            meshClosedCounts[deviceMac.c_str()] = meshClosedCount;
            contactMeshStates[deviceMac.c_str()] = "CLOSED";
            std::map<std::string, unsigned long>::iterator itW = updateClosedCount.find(deviceMac.c_str());
            if (itW != updateClosedCount.end())
            {
              updateClosedCount.erase(deviceMac.c_str());
            }
          }
          else {
            updateClosedCount[deviceMac.c_str()] = millis();
            updateMeshClosedCount[deviceMac.c_str()] = meshClosedCount;
            shouldPublish = false;
          }

          closedCount = closedCount + 1;
          if (closedCount > 50) {
            closedCount = 1;
          }
          closedCounts[deviceMac.c_str()] = closedCount;
        }
        else if (strcmp(contact.c_str(), "TIMEOUT") == 0) {

          if ((meshTimeoutCount == timeoutCount) && (meshTimeoutCount != 0) && (meshOpenCount == openCount) && (meshClosedCount == closedCount)) {
            meshTimeoutCount = meshTimeoutCount + 1;
            if (meshTimeoutCount > 50) {
              meshTimeoutCount = 1;
            }
            meshTimeoutCounts[deviceMac.c_str()] = meshTimeoutCount;
            contactMeshStates[deviceMac.c_str()] = "TIMEOUT";
            std::map<std::string, unsigned long>::iterator itW = updateTimeoutCount.find(deviceMac.c_str());
            if (itW != updateTimeoutCount.end())
            {
              updateTimeoutCount.erase(deviceMac.c_str());
            }
          }
          else {
            updateTimeoutCount[deviceMac.c_str()] = millis();
            updateMeshTimeoutCount[deviceMac.c_str()] = meshTimeoutCount;
            shouldPublish = false;
          }

          timeoutCount = timeoutCount + 1;

          if (timeoutCount > 50) {
            timeoutCount = 1;
          }

          timeoutCounts[deviceMac.c_str()] = timeoutCount;
        }
      }
    }
    else {

      std::map<std::string, unsigned long>::iterator itW = updateOpenCount.find(deviceMac.c_str());
      if (itW != updateOpenCount.end())
      {
        shouldPublish = false;
      }
      itW = updateClosedCount.find(deviceMac.c_str());
      if (itW != updateClosedCount.end())
      {
        shouldPublish = false;
      }
      itW = updateTimeoutCount.find(deviceMac.c_str());
      if (itW != updateTimeoutCount.end())
      {
        shouldPublish = false;
      }
    }
  }

  contactStates[deviceMac.c_str()] = contact;

  if (((meshClosedCount != closedCount) || (meshTimeoutCount != timeoutCount) || (meshOpenCount != openCount)) && enableMesh && meshContactSensors && countContactToAvoidDuplicates) {
    shouldPublish = false;
  }

  if (shouldPublish) {
    if (meshContactSensors && enableMesh && countContactToAvoidDuplicates) {
      if ((strcmp(contact.c_str(), "OPEN") == 0) && (meshOpenCount != 0) && (meshOpenCount == openCount)) {
        std::string deviceOpenMeshTopic = contactTopic + aDevice + "/opencount";
        addToPublish(deviceOpenMeshTopic.c_str(), meshOpenCount, true);
      }
      else if ((strcmp(contact.c_str(), "CLOSED") == 0) && (meshClosedCount != 0) && (meshClosedCount == closedCount)) {
        std::string deviceClosedMeshTopic = contactTopic + aDevice + "/closedcount";
        addToPublish(deviceClosedMeshTopic.c_str(), meshClosedCount, true);
      }
      else if ((strcmp(contact.c_str(), "TIMEOUT") == 0) && (meshTimeoutCount != 0) && (meshTimeoutCount == timeoutCount)) {
        std::string deviceTimeoutMeshTopic = contactTopic + aDevice + "/timeoutcount";
        addToPublish(deviceTimeoutMeshTopic.c_str(), meshTimeoutCount, true);
      }
    }

    if (!isMeshNode || !onlyAllowRootESPToPublishContact) {
      std::string deviceContactTopic = contactTopic + aDevice + "/contact";
      std::string deviceBinContactTopic = contactTopic + aDevice + "/bin";
      std::string deviceStateTopic = contactTopic + aDevice + "/state";
      addToPublish(deviceBinContactTopic.c_str(), binContact.c_str(), true);
      addToPublish(deviceContactTopic.c_str(), contact.c_str(), true);
      addToPublish(deviceStateTopic.c_str(), contact, true);
      if (lastContact >= 0) {
        publishContactLastContact(aDevice, lastContact);
      }
    }
  }
}

void processButton(std::string & aDevice, std::string & deviceMac, std::string & aValueString, bool isActive, bool aPublish) {
  uint8_t byte8 = 0;
  bool shouldPublish = aPublish;
  bool valueChanged = false;
  if (isActive) {
    byte8 = (uint8_t) aValueString[8];
  }
  else {
    byte8 = (uint8_t) aValueString[14];
  }

  int buttonCountA = (byte8 & 0b00001000) ? 8 : 0;
  int buttonCountB = (byte8 & 0b00000100) ? 4 : 0;
  int buttonCountC = (byte8 & 0b00000010) ? 2 : 0;
  int buttonCountD = (byte8 & 0b00000001) ? 1 : 0;
  int buttonCount = buttonCountA + buttonCountB + buttonCountC + buttonCountD;

  std::string deviceButtonTopic = contactTopic + aDevice + "/button";

  std::map<std::string, int>::iterator itE = buttonCounts.find(deviceMac);
  if (itE != buttonCounts.end())
  {
    int bCount = itE->second;
    if (((bCount < buttonCount) ||  ((bCount > 10) && (buttonCount < 5))) && (buttonCount != 0)) {
      shouldPublish = true;
      valueChanged = true;
    }
    else if (bCount != buttonCount) {
      shouldPublish = false;
    }
  }
  else {
    buttonCounts[deviceMac] = buttonCount;
    shouldPublish = false;
  }

  if (shouldPublish) {
    buttonCounts[deviceMac] = buttonCount;
    std::string deviceButtonMeshTopic = contactTopic + aDevice + "/buttoncount";
    addToPublish(deviceButtonMeshTopic.c_str(), buttonCount, false);
    if (!isMeshNode && valueChanged) {
      //addToPublish(deviceButtonTopic.c_str(), "PUSHED", false);
      client.publish(deviceButtonTopic.c_str(), "PUSHED", false);
      lastButton[deviceMac] = millis();
    }
    else if (!isMeshNode) {
      addToPublish(deviceButtonTopic.c_str(), "IDLE", false);
    }
  }
}

void processEntrance(std::string & aDevice, std::string & deviceMac, std::string & aValueString, bool isActive, bool aPublish) {
  uint8_t byte8 = 0;
  bool shouldPublish = aPublish;
  bool valueChanged = false;
  if (isActive) {
    byte8 = (uint8_t) aValueString[8];
  }
  else {
    byte8 = (uint8_t) aValueString[14];
  }
  int entranceCountA = (byte8 & 0b10000000) ? 2 : 0;
  int entranceCountB = (byte8 & 0b01000000) ? 1 : 0;
  int entranceCount = entranceCountA + entranceCountB;
  std::string deviceInTopic = contactTopic + aDevice + "/in";
  std::map<std::string, int>::iterator itE = entranceCounts.find(deviceMac);
  if (itE != entranceCounts.end())
  {
    int eCount = itE->second;
    if (((eCount < entranceCount) ||  ((eCount == 3) && (entranceCount == 1))) && (entranceCount != 0)) {
      shouldPublish = true;
      valueChanged = true;
    }
    else if (eCount != entranceCount) {
      shouldPublish = false;
    }
  }
  else {
    entranceCounts[deviceMac] = entranceCount;
    shouldPublish = false;
  }

  if (shouldPublish) {
    entranceCounts[deviceMac] = entranceCount;

    std::string deviceInMeshTopic = contactTopic + aDevice + "/incount";
    addToPublish(deviceInMeshTopic.c_str(), entranceCount, false);
    if (!isMeshNode && valueChanged) {
      //addToPublish(deviceInTopic.c_str(), "ENTERED", false);
      client.publish(deviceInTopic.c_str(), "ENTERED", false);
      lastIn[deviceMac] = millis();
    }
    else if (!isMeshNode) {
      addToPublish(deviceInTopic.c_str(), "IDLE", false);
    }
  }
}

void processExit(std::string & aDevice, std::string & deviceMac, std::string & aValueString, bool isActive, bool aPublish) {
  uint8_t byte8 = 0;
  bool shouldPublish = aPublish;
  bool valueChanged = false;
  if (isActive) {
    byte8 = (uint8_t) aValueString[8];
  }
  else {
    byte8 = (uint8_t) aValueString[14];
  }
  std::string deviceOutTopic = contactTopic + aDevice + "/out";
  int outCountA = (byte8 & 0b00100000) ? 2 : 0;
  int outCountB = (byte8 & 0b00010000) ? 1 : 0;
  int outCount = outCountA + outCountB;
  std::map<std::string, int>::iterator itE = outCounts.find(deviceMac.c_str());
  if (itE != outCounts.end())
  {
    int oCount = itE->second;
    if (((oCount < outCount) ||  ((oCount == 3) && (outCount == 1))) && (outCount != 0)) {
      shouldPublish = true;
      valueChanged = true;
    }
    else if (oCount != outCount) {
      shouldPublish = false;
    }
  }
  else {
    outCounts[deviceMac.c_str()] = outCount;
    shouldPublish = false;
  }

  if (shouldPublish) {
    outCounts[deviceMac.c_str()] = outCount;

    std::string deviceOutMeshTopic = contactTopic + aDevice + "/outcount";
    addToPublish(deviceOutMeshTopic.c_str(), outCount, false);
    if (!isMeshNode && valueChanged) {
      //addToPublish(deviceOutTopic.c_str(), "EXITED", false);
      client.publish(deviceOutTopic.c_str(), "EXITED", false);
      lastOut[deviceMac] = millis();
    }
    else if (!isMeshNode) {
      addToPublish(deviceOutTopic.c_str(), "IDLE", false);
    }
  }
}


void processBotBattery(std::string & aDevice, std::string & deviceMac, std::string & aValueString, bool isActive, bool aPublish, JsonDocument & aJsonDoc) {
  int battLevel = 0;
  bool shouldPublish = aPublish;
  if (isActive) {
    uint8_t byte2 = (uint8_t) aValueString[2];
    battLevel = (byte2 & 0b01111111);
    batteryValues[aDevice] = battLevel;
    aJsonDoc["batt"] = battLevel;
  }
  else if (onlyPassiveScan) {
    shouldPublish = false;
  }
  else {
    battLevel = batteryValues[aDevice];
    aJsonDoc["batt"] = battLevel;
  }
  if (shouldPublish) {
    //std::string deviceBatteryTopic = ;
    addToPublish(botTopic + aDevice + "/battery", battLevel, true);
  }
}

void processCurtainBattery(std::string & aDevice, std::string & deviceMac, std::string & aValueString, bool isActive, bool aPublish, JsonDocument & aJsonDoc) {
  int battLevel = 0;
  bool shouldPublish = aPublish;
  if (isActive) {
    uint8_t byte2 = (uint8_t) aValueString[2];
    battLevel = (byte2 & 0b01111111);
    batteryValues[aDevice] = battLevel;
    aJsonDoc["batt"] = battLevel;
  }
  else if (onlyPassiveScan) {
    shouldPublish = false;
  }
  else {
    battLevel = batteryValues[aDevice];
    aJsonDoc["batt"] = battLevel;
  }
  if (shouldPublish) {
    //std::string deviceBatteryTopic = ;
    addToPublish(curtainTopic + aDevice + "/battery", battLevel, true);
  }
}

void processMeterBattery(std::string & aDevice, std::string & deviceMac, std::string & aValueString, bool isActive, bool aPublish, JsonDocument & aJsonDoc) {
  int battLevel = 0;
  bool shouldPublish = aPublish;
  if (isActive) {
    uint8_t byte2 = (uint8_t) aValueString[2];
    battLevel = (byte2 & 0b01111111);
    batteryValues[aDevice] = battLevel;
    aJsonDoc["batt"] = battLevel;
  }
  else if (onlyPassiveScan) {
    shouldPublish = false;
  }
  else {
    battLevel = batteryValues[aDevice];
    aJsonDoc["batt"] = battLevel;
  }
  if (shouldPublish) {
    //std::string deviceBatteryTopic = ;
    addToPublish(meterTopic + aDevice + "/battery", battLevel, true);
  }
}

void processContactBattery(std::string & aDevice, std::string & deviceMac, std::string & aValueString, bool isActive, bool aPublish) {
  int battLevel = 0;
  bool shouldPublish = aPublish;
  if (isActive) {
    uint8_t byte2 = (uint8_t) aValueString[2];
    battLevel = (byte2 & 0b01111111);
    batteryValues[aDevice] = battLevel;
    // aJsonDoc["batt"] = battLevel;
  }
  else if (onlyPassiveScan) {
    shouldPublish = false;
  }
  else {
    battLevel = batteryValues[aDevice];
  }
  if (shouldPublish) {
    //std::string deviceBatteryTopic = ;
    addToPublish(contactTopic + aDevice + "/battery", battLevel, true);
  }
}

void processMotionBattery(std::string & aDevice, std::string & deviceMac, std::string & aValueString, bool isActive, bool aPublish) {
  int battLevel = 0;
  bool shouldPublish = aPublish;
  if (isActive) {
    uint8_t byte2 = (uint8_t) aValueString[2];
    battLevel = (byte2 & 0b01111111);
    batteryValues[aDevice] = battLevel;
    //aJsonDoc["batt"] = battLevel;
  }
  else if (onlyPassiveScan) {
    shouldPublish = false;
  }
  else {
    battLevel = batteryValues[aDevice];
  }
  if (shouldPublish) {
    //std::string deviceBatteryTopic = ;
    addToPublish(motionTopic + aDevice + "/battery", battLevel, true);
  }
}

void processBotRSSI(std::string & aDevice, std::string & deviceMac, long anRSSI, bool isActive, bool aPublish, JsonDocument & aJsonDoc) {
  bool shouldPublish = aPublish;
  aJsonDoc["rssi"] = anRSSI;
  if (shouldPublish) {
    //std::string deviceRSSITopic = ;
    addToPublish(botTopic + aDevice + "/rssi", anRSSI, true);
  }
}

void processCurtainRSSI(std::string & aDevice, std::string & deviceMac, long anRSSI, bool isActive, bool aPublish, JsonDocument & aJsonDoc) {
  bool shouldPublish = aPublish;
  aJsonDoc["rssi"] = anRSSI;
  if (shouldPublish) {
    //std::string deviceRSSITopic = curtainTopic + aDevice + "/rssi";
    addToPublish(curtainTopic + aDevice + "/rssi", anRSSI, true);
  }
}

void processPlugRSSI(std::string & aDevice, std::string & deviceMac, long anRSSI, bool isActive, bool aPublish, JsonDocument & aJsonDoc) {
  bool shouldPublish = aPublish;
  aJsonDoc["rssi"] = anRSSI;
  if (shouldPublish) {
    addToPublish(plugTopic + aDevice + "/rssi", anRSSI, true);
  }
}

void processContactRSSI(std::string & aDevice, std::string & deviceMac, long anRSSI, bool isActive, bool aPublish) {
  bool shouldPublish = aPublish;
  //aJsonDoc["rssi"] = anRSSI;

  if (shouldPublish) {
    //std::string deviceRSSITopic = ;
    addToPublish(contactTopic + aDevice + "/rssi", anRSSI, true);
    if (meshContactSensors && enableMesh) {
      //deviceRSSITopic = ;
      addToPublish(contactMainTopic + aDevice + "/rssi", anRSSI, true);
    }
  }
}

void processMotionRSSI(std::string & aDevice, std::string & deviceMac, long anRSSI, bool isActive, bool aPublish) {
  bool shouldPublish = aPublish;
  //aJsonDoc["rssi"] = anRSSI;

  if (shouldPublish) {
    //std::string deviceRSSITopic = ;
    addToPublish(motionTopic + aDevice + "/rssi", anRSSI, true);
    if (meshMotionSensors && enableMesh) {
      //deviceRSSITopic = motionMainTopic + aDevice + "/rssi";
      addToPublish(motionMainTopic + aDevice + "/rssi", anRSSI, true);
    }
  }
}

void processMeterRSSI(std::string & aDevice, std::string & deviceMac, long anRSSI, bool isActive, bool aPublish, JsonDocument & aJsonDoc) {
  bool shouldPublish = aPublish;
  aJsonDoc["rssi"] = anRSSI;

  if (shouldPublish) {
    //std::string deviceRSSITopic = ;
    addToPublish(meterTopic + aDevice + "/rssi", anRSSI, true);
    if (meshMeters && enableMesh) {
      //deviceRSSITopic = ;
      addToPublish(meterMainTopic + aDevice + "/rssi", anRSSI, true);
    }
  }
}

static unsigned long lastOnlinePublished = 0;
static unsigned long lastRescan = 0;
static unsigned long lastScanCheck = 0;
static bool noResponse = false;
static bool waitForResponse = false;
static std::string lastDeviceControlled = "";

bool publishMQTT(QueuePublish aCommand) {
  if (client.isConnected()) {
    client.publish(aCommand.topic.c_str(), aCommand.payload.c_str(), aCommand.retain);
    return true;
  }
  return false;
}

/*bool publishMQTT(std::string topic, std::string payload) {
  return publishMQTT(topic, payload, false);
  }

  bool publishMQTT(std::string topic, char * payload) {
  return publishMQTT(topic, payload, false);
  }*/

void processContactSensorTasks() {
  std::string anAddr;
  std::string aDevice;
  unsigned long aTime = 0;
  std::map<std::string, std::string>::iterator itT = allContactSensors.begin();
  while (itT != allContactSensors.end())
  {
    anAddr = itT->second;
    aDevice = itT->first;
    std::map<std::string, unsigned long>::iterator itW = lastButton.find(anAddr.c_str());
    if (itW != lastButton.end())
    {
      aTime = itW->second;
      if ((millis() - aTime) > 1000) {
        std::string deviceButtonTopic = contactTopic + aDevice + "/button";
        addToPublish(deviceButtonTopic.c_str(), "IDLE", false);
        lastButton.erase(anAddr.c_str());
      }
    }

    itW = lastIn.find(anAddr.c_str());
    if (itW != lastIn.end())
    {
      aTime = itW->second;
      if ((millis() - aTime) > 1000) {
        std::string deviceInTopic = contactTopic + aDevice + "/in";
        addToPublish(deviceInTopic.c_str(), "IDLE", false);
        lastIn.erase(anAddr.c_str());
      }
    }

    itW = lastOut.find(anAddr.c_str());
    if (itW != lastOut.end())
    {
      aTime = itW->second;
      if ((millis() - aTime) > 1000) {
        std::string deviceOutTopic = contactTopic + aDevice + "/out";
        addToPublish(deviceOutTopic.c_str(), "IDLE", false);
        lastOut.erase(anAddr.c_str());
      }
    }

    itW = updateOpenCount.find(anAddr.c_str());
    if (itW != updateOpenCount.end())
    {
      aTime = itW->second;
      if ((millis() - aTime) > (timeToIgnoreDuplicates * 1000)) {
        std::map<std::string, int>::iterator itQQ = updateMeshOpenCount.find(anAddr.c_str());
        if (itQQ != updateMeshOpenCount.end())
        {
          openCounts[anAddr.c_str()] = itQQ->second;
          updateOpenCount.erase(anAddr.c_str());
        }
      }
    }

    itW = updateClosedCount.find(anAddr.c_str());
    if (itW != updateClosedCount.end())
    {
      aTime = itW->second;
      if ((millis() - aTime) > (timeToIgnoreDuplicates * 1000)) {
        std::map<std::string, int>::iterator itQQ = updateMeshClosedCount.find(anAddr.c_str());
        if (itQQ != updateMeshClosedCount.end())
        {
          closedCounts[anAddr.c_str()] = itQQ->second;
          updateClosedCount.erase(anAddr.c_str());
        }
      }
    }

    itW = updateTimeoutCount.find(anAddr.c_str());
    if (itW != updateTimeoutCount.end())
    {
      aTime = itW->second;
      if ((millis() - aTime) > (timeToIgnoreDuplicates * 1000)) {
        std::map<std::string, int>::iterator itQQ = updateMeshTimeoutCount.find(anAddr.c_str());
        if (itQQ != updateMeshTimeoutCount.end())
        {
          timeoutCounts[anAddr.c_str()] = itQQ->second;
          updateTimeoutCount.erase(anAddr.c_str());
        }
      }
    }

    itW = updateMotionCount.find(anAddr.c_str());
    if (itW != updateMotionCount.end())
    {
      aTime = itW->second;
      if ((millis() - aTime) > (timeToIgnoreDuplicates * 1000)) {
        std::map<std::string, int>::iterator itQQ = meshMotionCounts.find(anAddr.c_str());
        if (itQQ != meshMotionCounts.end())
        {
          motionCounts[anAddr.c_str()] = itQQ->second;
          updateMotionCount.erase(anAddr.c_str());
        }
      }
    }

    itW = updateNoMotionCount.find(anAddr.c_str());
    if (itW != updateNoMotionCount.end())
    {
      aTime = itW->second;
      if ((millis() - aTime) > (timeToIgnoreDuplicates * 1000)) {
        std::map<std::string, int>::iterator itQQ = meshNoMotionCounts.find(anAddr.c_str());
        if (itQQ != meshNoMotionCounts.end())
        {
          noMotionCounts[anAddr.c_str()] = itQQ->second;
          updateNoMotionCount.erase(anAddr.c_str());
        }
      }
    }

    itW = updateDarkCount.find(anAddr.c_str());
    if (itW != updateDarkCount.end())
    {
      aTime = itW->second;
      if ((millis() - aTime) > (timeToIgnoreDuplicates * 1000)) {
        std::map<std::string, int>::iterator itQQ = meshDarkCounts.find(anAddr.c_str());
        if (itQQ != meshDarkCounts.end())
        {
          darkCounts[anAddr.c_str()] = itQQ->second;
          updateDarkCount.erase(anAddr.c_str());
        }
      }
    }

    itW = updateBrightCount.find(anAddr.c_str());
    if (itW != updateBrightCount.end())
    {
      aTime = itW->second;
      if ((millis() - aTime) > (timeToIgnoreDuplicates * 1000)) {
        std::map<std::string, int>::iterator itQQ = meshBrightCounts.find(anAddr.c_str());
        if (itQQ != meshBrightCounts.end())
        {
          brightCounts[anAddr.c_str()] = itQQ->second;
          updateBrightCount.erase(anAddr.c_str());
        }
      }
    }
    itT++;
  }
}

void processMotionSensorTasks() {
  std::string anAddr;
  std::string aDevice;
  unsigned long aTime = 0;
  std::map<std::string, std::string>::iterator itT = allMotionSensors.begin();
  while (itT != allMotionSensors.end())
  {
    anAddr = itT->second;
    aDevice = itT->first;

    std::map<std::string, unsigned long>::iterator itW = updateMotionCount.find(anAddr.c_str());
    if (itW != updateMotionCount.end())
    {
      aTime = itW->second;
      if ((millis() - aTime) > (timeToIgnoreDuplicates * 1000)) {
        std::map<std::string, int>::iterator itQQ = meshMotionCounts.find(anAddr.c_str());
        if (itQQ != meshMotionCounts.end())
        {
          motionCounts[anAddr.c_str()] = itQQ->second;
          updateMotionCount.erase(anAddr.c_str());
        }
      }
    }

    itW = updateNoMotionCount.find(anAddr.c_str());
    if (itW != updateNoMotionCount.end())
    {
      aTime = itW->second;
      if ((millis() - aTime) > (timeToIgnoreDuplicates * 1000)) {
        std::map<std::string, int>::iterator itQQ = meshNoMotionCounts.find(anAddr.c_str());
        if (itQQ != meshNoMotionCounts.end())
        {
          noMotionCounts[anAddr.c_str()] = itQQ->second;
          updateNoMotionCount.erase(anAddr.c_str());
        }
      }
    }

    itW = updateDarkCount.find(anAddr.c_str());
    if (itW != updateDarkCount.end())
    {
      aTime = itW->second;
      if ((millis() - aTime) > (timeToIgnoreDuplicates * 1000)) {
        std::map<std::string, int>::iterator itQQ = meshDarkCounts.find(anAddr.c_str());
        if (itQQ != meshDarkCounts.end())
        {
          darkCounts[anAddr.c_str()] = itQQ->second;
          updateDarkCount.erase(anAddr.c_str());
        }
      }
    }

    itW = updateBrightCount.find(anAddr.c_str());
    if (itW != updateBrightCount.end())
    {
      aTime = itW->second;
      if ((millis() - aTime) > (timeToIgnoreDuplicates * 1000)) {
        std::map<std::string, int>::iterator itQQ = meshBrightCounts.find(anAddr.c_str());
        if (itQQ != meshBrightCounts.end())
        {
          brightCounts[anAddr.c_str()] = itQQ->second;
          updateBrightCount.erase(anAddr.c_str());
        }
      }
    }
    itT++;
  }
}

void publishAllMQTT() {
  int attempts = 0;
  while (!(publishQueue.isEmpty()) && attempts < 3) {
    if (!(client.isConnected())) {
      client.loop();
    }
    else {
      attempts++;
    }
    bool success = false;
    QueuePublish aCommand = publishQueue.getHead();
    success = publishMQTT(aCommand);
    if (success) {
      publishQueue.dequeue();
    }
  }
}

void processAllAdvData() {
  int attempts = 0;
  while (!(advDataQueue.isEmpty()) && attempts < 3) {
    if (!(client.isConnected())) {
      client.loop();
    }
    else {
      attempts++;
    }
    bool success = false;
    QueueAdvData aAdvData = advDataQueue.getHead();
    processAdvData(aAdvData.macAddr, aAdvData.rssi, aAdvData.aValueString, aAdvData.useActiveScan);
    advDataQueue.dequeue();
  }
}

void processAdvData(std::string & deviceMac, long anRSSI,  std::string & aValueString, bool useActiveScan) {
  yield();
  bool shouldPublish = alwaysMQTTUpdate;
  if (!initialScanComplete) {
    shouldPublish = true;
  }
  if (!shouldPublish) {
    if (shouldMQTTUpdateOrActiveScanForDevice(deviceMac)) {
      shouldPublish = true;
    }
  }
  std::string aDevice;
  std::string aState = "";
  std::string deviceStateTopic;
  std::string deviceAttrTopic;

  std::map<std::string, std::string>::iterator itS = allSwitchbotsOpp.find(deviceMac);
  if (itS != allSwitchbotsOpp.end())
  {
    aDevice = itS->second.c_str();
  }
  else {
    return;
  }
  std::string deviceName;
  itS = deviceTypes.find(deviceMac);
  if (itS != deviceTypes.end())
  {
    deviceName = itS->second.c_str();
  }

  int aLength = aValueString.length();
  if (deviceName == botName) {
    StaticJsonDocument<200> aJsonDoc;
    char aBuffer[200];

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
      addToPublish(deviceAssumedStateTopic.c_str(), aState.c_str(), true);
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
        addToPublish(deviceAssumedStateTopic.c_str(), aState.c_str(), true);
      }
      else {
        aState = "OFF";
      }
    }

    std::map<std::string, std::string>::iterator itH = botStates.find(deviceMac.c_str());
    if (itH != botStates.end())
    {
      std::string botState = itH->second.c_str();
      if (strcmp(botState.c_str(), aState.c_str()) != 0) {
        shouldPublish = true;

      }
    }
    else {
      shouldPublish = true;
    }

    botStates[deviceMac] = aState.c_str();

    aJsonDoc["mode"] = aMode;
    aJsonDoc["state"] = aState;

    processBotBattery(aDevice, deviceMac, aValueString, useActiveScan, shouldPublish, aJsonDoc);
    processBotRSSI(aDevice, deviceMac, anRSSI, useActiveScan, shouldPublish, aJsonDoc);

    if (shouldPublish) {
      if (useActiveScan) {
        delay(50);
        serializeJson(aJsonDoc, aBuffer);
        addToPublish(deviceAttrTopic.c_str(), aBuffer, true);
        delay(50);
        addToPublish(deviceStateTopic.c_str(), aState.c_str(), true);
      }
      lastUpdateTimes[deviceMac] = millis();
    }
  }
  else if (deviceName == meterName) {
    StaticJsonDocument<200> aJsonDoc;
    char aBuffer[200];

    deviceStateTopic = meterTopic + aDevice + "/state";
    deviceAttrTopic = meterTopic + aDevice + "/attributes";

    uint8_t byte2 = 0;
    uint8_t byte3 = 0;
    uint8_t byte4 = 0;
    uint8_t byte5 = 0;

    if ( useActiveScan) {
      byte2 = (uint8_t) aValueString[2];
      byte3 = (uint8_t) aValueString[3];
      byte4 = (uint8_t) aValueString[4];
      byte5 = (uint8_t) aValueString[5];
    }
    else
    {
      byte3 = (uint8_t) aValueString[10];
      byte4 = (uint8_t) aValueString[11];
      byte5 = (uint8_t) aValueString[12];
    }

    int tempSign = (byte4 & 0b10000000) ? 1 : -1;
    float tempC = tempSign * ((byte4 & 0b01111111) + ((byte3 & 0b00001111) / 10.0));
    float tempF = (tempC * 9 / 5.0) + 32;
    tempF = round(tempF * 10) / 10.0;
    bool tempScale = (byte5 & 0b10000000) ;
    std::string str1 = (tempScale == true) ? "f" : "c";
    aJsonDoc["scale"] = str1;
    aJsonDoc["C"] = serialized(String(tempC, 1));
    aJsonDoc["F"] = serialized(String(tempF, 1));
    int humidity = byte5 & 0b01111111;
    aJsonDoc["hum"] = humidity;
    aState = String(tempC, 1).c_str();

    std::map<std::string, float>::iterator itH = meterTempCStates.find(deviceMac.c_str());
    if (itH != meterTempCStates.end())
    {
      float tempCState = itH->second;
      if (tempCState != tempC) {
        shouldPublish = true;

      }
    }
    else {
      shouldPublish = true;
    }
    std::map<std::string, float>::iterator itW = meterTempFStates.find(deviceMac.c_str());
    if (itW != meterTempFStates.end())
    {
      float tempFState = itW->second;
      if (tempFState != tempF) {
        shouldPublish = true;

      }
    }
    else {
      shouldPublish = true;
    }

    std::map<std::string, int>::iterator itR = meterHumidStates.find(deviceMac.c_str());
    if (itR != meterHumidStates.end())
    {
      int humidState = itR->second;
      if (humidState != humidity) {
        shouldPublish = true;

      }
    }
    else {
      shouldPublish = true;
    }

    meterTempCStates[deviceMac] = tempC;
    meterTempFStates[deviceMac] = tempF;
    meterHumidStates[deviceMac] = humidity;

    processMeterBattery(aDevice, deviceMac, aValueString, useActiveScan, shouldPublish, aJsonDoc);
    processMeterRSSI(aDevice, deviceMac, anRSSI, useActiveScan, shouldPublish, aJsonDoc);

    if (shouldPublish) {
      delay(50);
      serializeJson(aJsonDoc, aBuffer);
      addToPublish(deviceAttrTopic.c_str(), aBuffer, true);
      delay(50);
      addToPublish(deviceStateTopic.c_str(), aState.c_str(), true);
      lastUpdateTimes[deviceMac] = millis();
    }
  }
  else if (deviceName == motionName) {

    processMotionMotion(aDevice, deviceMac, aValueString, useActiveScan, shouldPublish);
    processLightMotion(aDevice, deviceMac, aValueString, useActiveScan, shouldPublish);
    processLED(aDevice, deviceMac, aValueString, useActiveScan, shouldPublish);
    processSenseDistance(aDevice, deviceMac, aValueString, useActiveScan, shouldPublish);
    processMotionBattery(aDevice, deviceMac, aValueString, useActiveScan, shouldPublish);
    processMotionRSSI(aDevice, deviceMac, anRSSI, useActiveScan, shouldPublish);
  }

  else if (deviceName == contactName) {

    processContact(aDevice, deviceMac, aValueString, useActiveScan, shouldPublish);
    processMotionContact(aDevice, deviceMac, aValueString, useActiveScan, shouldPublish);
    processLightContact(aDevice, deviceMac, aValueString, useActiveScan, shouldPublish);
    processButton(aDevice, deviceMac, aValueString, useActiveScan, shouldPublish);
    processEntrance(aDevice, deviceMac, aValueString, useActiveScan, shouldPublish);
    processExit(aDevice, deviceMac, aValueString, useActiveScan, shouldPublish);
    processContactBattery(aDevice, deviceMac, aValueString, useActiveScan, shouldPublish);
    processContactRSSI(aDevice, deviceMac, anRSSI, useActiveScan, shouldPublish);
  }

  else if (deviceName == curtainName) {

    StaticJsonDocument<200> aJsonDoc;
    char aBuffer[200];
    processCurtainBattery(aDevice, deviceMac, aValueString, useActiveScan, shouldPublish, aJsonDoc);
    processCurtainRSSI(aDevice, deviceMac, anRSSI, useActiveScan, shouldPublish, aJsonDoc);

    std::string devicePosTopic = curtainTopic + aDevice + "/position";
    deviceStateTopic = curtainTopic + aDevice + "/state";
    deviceAttrTopic = curtainTopic + aDevice + "/attributes";

    uint8_t byte1 = (uint8_t) aValueString[1];
    uint8_t byte2 = (uint8_t) aValueString[2];
    uint8_t byte3 = (uint8_t) aValueString[3];
    uint8_t byte4 = (uint8_t) aValueString[4];

    bool calibrated = byte1 & 0b01000000;
    //int battLevel = byte2 & 0b01111111;
    int currentPosition = 100 - (byte3 & 0b01111111);
    int lightLevel = (byte4 >> 4) & 0b00001111;
    aState = "OPEN";

    aJsonDoc["calib"] = calibrated;
    //aJsonDoc["batt"] = battLevel;
    aJsonDoc["pos"] = currentPosition;
    if (currentPosition <= curtainClosedPosition)
      aState = "CLOSE";
    aJsonDoc["state"] = aState;
    aJsonDoc["light"] = lightLevel;

    std::map<std::string, std::string>::iterator itH = curtainStates.find(deviceMac.c_str());
    if (itH != curtainStates.end())
    {
      std::string curtainState = itH->second.c_str();
      if (strcmp(curtainState.c_str(), aState.c_str()) != 0) {
        shouldPublish = true;

      }
    }
    else {
      shouldPublish = true;
    }

    std::map<std::string, int>::iterator itW = curtainPositionStates.find(deviceMac.c_str());
    if (itW != curtainPositionStates.end())
    {
      int positionState = itW->second;
      if (positionState != currentPosition) {
        shouldPublish = true;

      }
    }
    else {
      shouldPublish = true;
    }

    std::map<std::string, int>::iterator itE = curtainLightStates.find(deviceMac.c_str());
    if (itE != curtainLightStates.end())
    {
      int lightState = itE->second;
      if (lightState != lightLevel) {
        shouldPublish = true;

      }
    }
    else {
      shouldPublish = true;
    }

    curtainStates[deviceMac] = aState.c_str();
    curtainPositionStates[deviceMac] = currentPosition;
    curtainLightStates[deviceMac] = lightLevel;

    if (shouldPublish) {
      if (useActiveScan) {
        delay(50);
        serializeJson(aJsonDoc, aBuffer);
        addToPublish(deviceAttrTopic.c_str(), aBuffer, true);
        delay(50);
        addToPublish(deviceStateTopic.c_str(), aState.c_str(), true);
        StaticJsonDocument<50> docPos;
        docPos["pos"] = currentPosition;
        serializeJson(docPos, aBuffer);
        addToPublish(devicePosTopic.c_str(), aBuffer, true);
      }
      lastUpdateTimes[deviceMac] = millis();
    }
  }

  else if (deviceName == plugName) {
    StaticJsonDocument<200> aJsonDoc;
    char aBuffer[200];

    deviceStateTopic = plugTopic + aDevice + "/state";
    deviceAttrTopic = plugTopic + aDevice + "/attributes";
    std::string devicePowerTopic = plugTopic + aDevice + "/energy";
    std::string deviceOverloadTopic = plugTopic + aDevice + "/overload";

    uint8_t byte9 = (uint8_t) aValueString[9];
    uint8_t byte12 = (uint8_t) aValueString[12];
    uint8_t byte13 = (uint8_t) aValueString[13];

    bool overload = byte12 & 0b10000000;

    byte powerHIGH = (byte13 & 0b11111111);
    byte powerLOW = (byte12 & 0b01111111);
    byte data2[] = {powerHIGH, powerLOW};
    long powerData = le16_to_cpu_signed(data2);

    std::string overloadStr = "";
    aState = "UNKNOWN";
    if (byte9 == 0) {
      aState = "OFF";
    }
    else if (byte9 == 128) {
      aState = "ON";
    }

    if (overload) {
      overloadStr = "true";
    }
    else {
      overloadStr = "false";
    }

    std::map<std::string, std::string>::iterator itH = plugStates.find(deviceMac.c_str());
    if (itH != plugStates.end())
    {
      std::string plugState = itH->second.c_str();
      if (strcmp(plugState.c_str(), aState.c_str()) != 0) {
        shouldPublish = true;

      }
    }
    else {
      shouldPublish = true;
    }

    std::map<std::string, long>::iterator itP = plugPowerStates.find(deviceMac.c_str());
    if (itP != plugPowerStates.end())
    {
      long plugPowerState = itP->second;
      if (plugPowerState != powerData) {
        shouldPublish = true;

      }
    }
    else {
      shouldPublish = true;
    }

    std::map<std::string, bool>::iterator itW = plugOverloadStates.find(deviceMac.c_str());
    if (itW != plugOverloadStates.end())
    {
      bool plugOverloadState = itW->second;
      if (plugOverloadState != overload) {
        shouldPublish = true;

      }
    }
    else {
      shouldPublish = true;
    }

    plugStates[deviceMac] = aState.c_str();
    plugPowerStates[deviceMac] = powerData;
    plugOverloadStates[deviceMac] = overload;

    processPlugRSSI(aDevice, deviceMac, anRSSI, useActiveScan, shouldPublish, aJsonDoc);

    aJsonDoc["state"] = aState;
    float powerLevel = powerData / 10.0;
    aJsonDoc["energy"] = serialized(String(powerLevel, 1));
    aJsonDoc["overload"] = overload;

    if (shouldPublish) {
      delay(50);
      serializeJson(aJsonDoc, aBuffer);
      addToPublish(deviceAttrTopic.c_str(), aBuffer, true);
      delay(50);
      addToPublish(deviceStateTopic.c_str(), aState.c_str(), true);
      addToPublish(devicePowerTopic.c_str(), (String(powerLevel, 1)).c_str(), true);
      addToPublish(deviceOverloadTopic.c_str(), overloadStr, true);
      lastUpdateTimes[deviceMac] = millis();
    }
  }

  yield();

  if (shouldPublish) {
    lastUpdateTimes[deviceMac] = millis();
  }
  yield();

}

void publishLastwillOnline() {
  if ((millis() - lastOnlinePublished) > 30000) {
    if (client.isConnected()) {
      addToPublish(lastWill, "online", true);
      lastOnlinePublished = millis();
      String rssi = String(WiFi.RSSI());
      addToPublish(rssiStdStr.c_str(), rssi.c_str());
    }
  }
}

void publishHomeAssistantDiscoveryESPConfig() {
  String wifiMAC = String(WiFi.macAddress());
  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + host + "/linkquality/config").c_str(), ("{\"~\":\"" + esp32Topic + "\"," +
               + "\"name\":\"Linkquality\"," +
               + "\"device\": {\"identifiers\":[\"switchbotesp_" + host + "_" + wifiMAC.c_str() + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + "ESP32" + "\",\"name\": \"" + host + "\" }," +
               + "\"avty_t\": \"" + lastWill + "\"," +
               + "\"uniq_id\":\"switchbotesp_" + wifiMAC.c_str() + "_linkquality\"," +
               + "\"stat_t\":\"~/rssi\"," +
               + "\"icon\":\"mdi:signal\"," +
               + "\"unit_of_meas\": \"rssi\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + host + "/firmware/config").c_str(), ("{\"~\":\"" + esp32Topic + "\"," +
               + "\"name\":\"Firmware\"," +
               + "\"device\": {\"identifiers\":[\"switchbotesp_" + host + "_" + wifiMAC.c_str() + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + "ESP32" + "\",\"name\": \"" + host + "\" }," +
               + "\"avty_t\": \"" + lastWill + "\"," +
               + "\"uniq_id\":\"switchbotesp_" + wifiMAC.c_str() + "_firmware\"," +
               + "\"icon\":\"mdi:cog\"," +
               + "\"stat_t\":\"~/firmware\"}").c_str(), true);
}


void publishHomeAssistantDiscoveryPlugConfig(std::string & deviceName, std::string deviceMac, bool optimistic) {
  std::transform(deviceMac.begin(), deviceMac.end(), deviceMac.begin(), ::toupper);

  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/linkquality/config").c_str(), ("{\"~\":\"" + (plugTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Humidity\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + plugModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWill + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_linkquality\"," +
               + "\"icon\":\"mdi:signal\"," +
               + "\"unit_of_meas\": \"rssi\", " +
               + "\"stat_t\":\"~/rssi\"}").c_str(), true);

  std::string optiString;
  if (optimistic) {
    optiString = "true";
  }
  else {
    optiString = "false";
  }

  addToPublish((home_assistant_mqtt_prefix + "/switch/" + deviceName + "/config").c_str(), ("{\"~\":\"" + (plugTopic + deviceName) + "\", " +
               + "\"name\":\"" + " Switch\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + plugModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWill + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "\", " +
               + "\"stat_t\":\"~/state\", " +
               + "\"opt\":" + optiString + ", " +
               + "\"cmd_t\": \"~/set\" }").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/energy/config").c_str(), ("{\"~\":\"" + (plugTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Energy\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + plugModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWill + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_energy\"," +
               + "\"unit_of_meas\":\"W\"," +
               + "\"state_class\":\"measurement\"," +
               + "\"dev_cla\":\"power\"," +
               + "\"stat_t\":\"~/energy\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/binary_sensor/" + deviceName + "/overload/config").c_str(), ("{\"~\":\"" + (plugTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Overload\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + plugModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWill + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_overload\"," +
               + "\"icon\":\"mdi:cog\"," +
               + "\"pl_on\":\"true\"," +
               + "\"pl_off\":\"false\"," +
               + "\"stat_t\":\"~/overload\"}").c_str(), true);

}


void publishHomeAssistantDiscoveryBotConfig(std::string & deviceName, std::string deviceMac, bool optimistic) {
  std::transform(deviceMac.begin(), deviceMac.end(), deviceMac.begin(), ::toupper);
  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/battery/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Battery\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWill + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_battery\"," +
               + "\"dev_cla\":\"battery\"," +
               + "\"unit_of_meas\": \"%\", " +
               + "\"stat_t\":\"~/battery\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/linkquality/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Linkquality\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWill + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_linkquality\"," +
               + "\"icon\":\"mdi:signal\"," +
               + "\"unit_of_meas\": \"rssi\", " +
               + "\"stat_t\":\"~/rssi\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/binary_sensor/" + deviceName + "/inverted/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Inverted\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWill + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "inverted\"," +
               + "\"stat_t\":\"~/settings\"," +
               + "\"icon\":\"mdi:cog\"," +
               + "\"pl_on\":true," +
               + "\"pl_off\":false," +
               + "\"value_template\":\"{{ value_json.inverted }}\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/mode/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Mode\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWill + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_mode\"," +
               + "\"icon\":\"mdi:cog\"," +
               + "\"stat_t\":\"~/attributes\"," +
               + "\"value_template\":\"{{ value_json.mode }}\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/firmware/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Firmware\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWill + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_firmware\"," +
               + "\"icon\":\"mdi:cog\"," +
               + "\"stat_t\":\"~/settings\"," +
               + "\"value_template\":\"{{ value_json.firmware }}\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/holdsecs/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\"," +
               + "\"name\":\"" + " HoldSecs\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWill + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_holdsecs\"," +
               + "\"icon\":\"mdi:cog\"," +
               + "\"stat_t\":\"~/settings\"," +
               + "\"value_template\":\"{{ value_json.hold }}\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/timers/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Timers\"," +
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
    addToPublish((home_assistant_mqtt_prefix + "/light/" + deviceName + "/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\", " +
                 + "\"name\":\"" + " Light\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "\", " +
                 + "\"stat_t\":\"~/state\", " +
                 + "\"opt\":" + optiString + ", " +
                 + "\"cmd_t\": \"~/set\" }").c_str(), true);
  }
  else if (strcmp(aType.c_str(), "button") == 0) {
    addToPublish((home_assistant_mqtt_prefix + "/button/" + deviceName + "/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\", " +
                 + "\"name\":\"" + " Button\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "\", " +
                 + "\"cmd_t\": \"~/set\" }").c_str(), true);
  }
  else {
    addToPublish((home_assistant_mqtt_prefix + "/switch/" + deviceName + "/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\", " +
                 + "\"name\":\"" + " Switch\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "\", " +
                 + "\"stat_t\":\"~/state\", " +
                 + "\"opt\":" + optiString + ", " +
                 + "\"cmd_t\": \"~/set\" }").c_str(), true);
  }
}

void publishHomeAssistantDiscoveryCurtainConfig(std::string & deviceName, std::string deviceMac) {
  std::transform(deviceMac.begin(), deviceMac.end(), deviceMac.begin(), ::toupper);
  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/battery/config").c_str(), ("{\"~\":\"" + (curtainTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Battery\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + curtainModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWill + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_battery\"," +
               + "\"dev_cla\":\"battery\"," +
               + "\"unit_of_meas\": \"%\", " +
               + "\"stat_t\":\"~/battery\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/linkquality/config").c_str(), ("{\"~\":\"" + (curtainTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Linkquality\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + curtainModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWill + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_linkquality\"," +
               + "\"icon\":\"mdi:signal\"," +
               + "\"unit_of_meas\": \"rssi\", " +
               + "\"stat_t\":\"~/rssi\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/illuminance/config").c_str(), ("{\"~\":\"" + (curtainTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Illuminance\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + curtainModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWill + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_illuminance\"," +
               + "\"stat_t\":\"~/attributes\"," +
               + "\"dev_cla\":\"illuminance\"," +
               + "\"unit_of_meas\": \"lx\", " +
               + "\"value_template\":\"{{ value_json.light }}\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/binary_sensor/" + deviceName + "/calibrated/config").c_str(), ("{\"~\":\"" + (curtainTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Calibrated\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + curtainModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWill + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_calibrated\"," +
               + "\"stat_t\":\"~/attributes\"," +
               + "\"icon\":\"mdi:cog\"," +
               + "\"pl_on\":true," +
               + "\"pl_off\":false," +
               + "\"value_template\":\"{{ value_json.calib }}\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/cover/" + deviceName + "/config").c_str(), ("{\"~\":\"" + (curtainTopic + deviceName) + "\", " +
               + "\"name\":\"" + " Curtain\"," +
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
    addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/position/config").c_str(), ("{\"~\":\"" + (curtainTopic + deviceName) + "\"," +
                 + "\"name\":\"" + " Position\"," +
                 + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + curtainModel + "\",\"name\": \"" + deviceName + "\" }," +
                 + "\"avty_t\": \"" + lastWill + "\"," +
                 + "\"uniq_id\":\"switchbot_" + deviceMac + "_position\"," +
                 + "\"stat_t\":\"~/position\"," +
                 + "\"unit_of_meas\": \"%\", " +
                 + "\"value_template\":\"{{ value_json.pos }}\"}").c_str(), true);
  }
}

void publishHomeAssistantDiscoveryMeterConfig(std::string & deviceName, std::string deviceMac) {

  const char* lastWillToUse = lastWill;

  if (meshMeters) {
    lastWillToUse = lastWillScan;
  }

  std::transform(deviceMac.begin(), deviceMac.end(), deviceMac.begin(), ::toupper);
  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/battery/config").c_str(), ("{\"~\":\"" + (meterTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Battery\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + meterModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWillToUse + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_battery\"," +
               + "\"dev_cla\":\"battery\"," +
               + "\"unit_of_meas\": \"%\", " +
               + "\"stat_t\":\"~/battery\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/linkquality/config").c_str(), ("{\"~\":\"" + (meterTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Linkquality\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + meterModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWillToUse + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_linkquality\"," +
               + "\"icon\":\"mdi:signal\"," +
               + "\"unit_of_meas\": \"rssi\", " +
               + "\"stat_t\":\"~/rssi\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/temperature/config").c_str(), ("{\"~\":\"" + (meterTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Temperature\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + meterModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWillToUse + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_temperature\"," +
               + "\"stat_t\":\"~/attributes\"," +
               + "\"dev_cla\":\"temperature\", " +
               + "\"unit_of_meas\": \"°C\", " +
               + "\"value_template\":\"{{ value_json.C }}\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/humidity/config").c_str(), ("{\"~\":\"" + (meterTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Humidity\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + meterModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWillToUse + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_humidity\"," +
               + "\"stat_t\":\"~/attributes\"," +
               + "\"dev_cla\":\"humidity\", " +
               + "\"unit_of_meas\": \"%\", " +
               + "\"value_template\":\"{{ value_json.hum }}\"}").c_str(), true);
}


void publishHomeAssistantDiscoveryContactConfig(std::string & deviceName, std::string deviceMac) {

  const char* lastWillToUse = lastWill;

  if (meshContactSensors) {
    lastWillToUse = lastWillScan;
  }

  std::transform(deviceMac.begin(), deviceMac.end(), deviceMac.begin(), ::toupper);
  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/battery/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Battery\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWillToUse + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_battery\"," +
               + "\"dev_cla\":\"battery\"," +
               + "\"unit_of_meas\": \"%\", " +
               + "\"stat_t\":\"~/battery\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/linkquality/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Linkquality\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWillToUse + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_linkquality\"," +
               + "\"icon\":\"mdi:signal\"," +
               + "\"unit_of_meas\": \"rssi\", " +
               + "\"stat_t\":\"~/rssi\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/contact/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Contact\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWillToUse + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_contact\"," +
               + "\"icon\":\"mdi:door\"," +
               + "\"stat_t\":\"~/contact\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/binary_sensor/" + deviceName + "/motion/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Motion\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWillToUse + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_motion\"," +
               + "\"stat_t\":\"~/motion\"," +
               + "\"dev_cla\":\"motion\"," +
               + "\"pl_on\":\"MOTION\"," +
               + "\"pl_off\":\"NO MOTION\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/binary_sensor/" + deviceName + "/bincontact/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
               + "\"name\":\"" + " BinaryContact\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWillToUse + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_bincontact\"," +
               + "\"icon\":\"mdi:door\"," +
               + "\"stat_t\":\"~/bin\"," +
               + "\"pl_on\":\"OPEN\"," +
               + "\"pl_off\":\"CLOSED\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/binary_sensor/" + deviceName + "/in/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
               + "\"name\":\"" + " In\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWillToUse + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_in\"," +
               + "\"icon\":\"mdi:motion-sensor\"," +
               + "\"stat_t\":\"~/in\"," +
               + "\"pl_on\":\"ENTERED\"," +
               + "\"pl_off\":\"IDLE\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/binary_sensor/" + deviceName + "/out/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Out\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWillToUse + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_out\"," +
               + "\"icon\":\"mdi:exit-run\"," +
               + "\"stat_t\":\"~/out\"," +
               + "\"pl_on\":\"EXITED\"," +
               + "\"pl_off\":\"IDLE\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/binary_sensor/" + deviceName + "/button/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Button\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWillToUse + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_button\"," +
               + "\"stat_t\":\"~/button\"," +
               + "\"icon\":\"mdi:gesture-tap-button\"," +
               + "\"pl_on\":\"PUSHED\"," +
               + "\"pl_off\":\"IDLE\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/binary_sensor/" + deviceName + "/illuminance/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Illuminance\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWillToUse + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "__illuminance\"," +
               + "\"stat_t\":\"~/illuminance\"," +
               + "\"dev_cla\":\"light\"," +
               + "\"pl_on\":\"BRIGHT\"," +
               + "\"pl_off\":\"DARK\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/lastmotion/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
               + "\"name\":\"" + " LastMotion\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWillToUse + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_lastmotion\"," +
               + "\"dev_cla\":\"timestamp\"," +
               + "\"stat_t\":\"~/lastmotion\"," +
               + "\"value_template\":\"{{ now() - timedelta(seconds = (value | int)) }}\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/lastcontact/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
               + "\"name\":\"" + " LastContact\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWillToUse + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_lastcontact\"," +
               + "\"dev_cla\":\"timestamp\"," +
               + "\"stat_t\":\"~/lastcontact\"," +
               + "\"value_template\":\"{{ now() - timedelta(seconds = (value | int)) }}\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/buttoncount/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
               + "\"name\":\"" + " ButtonCount\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWillToUse + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_buttoncount\"," +
               + "\"icon\":\"mdi:counter\"," +
               + "\"stat_t\":\"~/buttoncount\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/incount/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
               + "\"name\":\"" + " InCount\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWillToUse + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_entrancecount\"," +
               + "\"icon\":\"mdi:counter\"," +
               + "\"stat_t\":\"~/incount\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/outcount/config").c_str(), ("{\"~\":\"" + (contactTopic + deviceName) + "\"," +
               + "\"name\":\"" + " OutCount\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + contactModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWillToUse + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_outcount\"," +
               + "\"icon\":\"mdi:counter\"," +
               + "\"stat_t\":\"~/outcount\"}").c_str(), true);;
}

void publishHomeAssistantDiscoveryMotionConfig(std::string & deviceName, std::string deviceMac) {

  const char* lastWillToUse = lastWill;

  if (meshMotionSensors) {
    lastWillToUse = lastWillScan;
  }

  std::transform(deviceMac.begin(), deviceMac.end(), deviceMac.begin(), ::toupper);
  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/battery/config").c_str(), ("{\"~\":\"" + (motionTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Battery\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + motionModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWillToUse + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_battery\"," +
               + "\"dev_cla\":\"battery\"," +
               + "\"unit_of_meas\": \"%\", " +
               + "\"stat_t\":\"~/battery\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/linkquality/config").c_str(), ("{\"~\":\"" + (motionTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Linkquality\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + motionModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWillToUse + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_linkquality\"," +
               + "\"icon\":\"mdi:signal\"," +
               + "\"unit_of_meas\": \"rssi\", " +
               + "\"stat_t\":\"~/rssi\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/binary_sensor/" + deviceName + "/motion/config").c_str(), ("{\"~\":\"" + (motionTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Motion\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + motionModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWillToUse + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_motion\"," +
               + "\"stat_t\":\"~/motion\"," +
               + "\"dev_cla\":\"motion\"," +
               + "\"pl_on\":\"MOTION\"," +
               + "\"pl_off\":\"NO MOTION\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/binary_sensor/" + deviceName + "/illuminance/config").c_str(), ("{\"~\":\"" + (motionTopic + deviceName) + "\"," +
               + "\"name\":\"" + " Illuminance\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + motionModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWillToUse + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_illuminance\"," +
               + "\"stat_t\":\"~/illuminance\"," +
               + "\"dev_cla\":\"light\"," +
               + "\"pl_on\":\"BRIGHT\"," +
               + "\"pl_off\":\"DARK\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/binary_sensor/" + deviceName + "/led/config").c_str(), ("{\"~\":\"" + (motionTopic + deviceName) + "\"," +
               + "\"name\":\"" + " LED\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + motionModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWillToUse + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_led\"," +
               + "\"icon\":\"mdi:led-on\"," +
               + "\"pl_on\":\"ON\"," +
               + "\"pl_off\":\"OFF\"," +
               + "\"stat_t\":\"~/led\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/lastmotion/config").c_str(), ("{\"~\":\"" + (motionTopic + deviceName) + "\"," +
               + "\"name\":\"" + " LastMotion\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + motionModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWillToUse + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_lastmotion\"," +
               + "\"dev_cla\":\"timestamp\"," +
               + "\"stat_t\":\"~/lastmotion\"," +
               + "\"value_template\":\"{{ now() - timedelta(seconds = (value | int)) }}\"}").c_str(), true);

  addToPublish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/sensedistance/config").c_str(), ("{\"~\":\"" + (motionTopic + deviceName) + "\"," +
               + "\"name\":\"" + " SenseDistance\"," +
               + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + motionModel + "\",\"name\": \"" + deviceName + "\" }," +
               + "\"avty_t\": \"" + lastWillToUse + "\"," +
               + "\"uniq_id\":\"switchbot_" + deviceMac + "_sensedistance\"," +
               + "\"icon\":\"mdi:cog\"," +
               + "\"stat_t\":\"~/sensedist\"}").c_str(), true);
}


class ClientCallbacks : public NimBLEClientCallbacks {

    void onConnect(NimBLEClient* pClient) {
      printAString("Connected");
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
      printAString("Client Passkey Request");
      return 123456;
    };

    bool onConfirmPIN(uint32_t pass_key) {
      printAString("The passkey YES/NO number: ");
      printAString(pass_key);
      return true;
    };

    void onAuthenticationComplete(ble_gap_conn_desc* desc) {
      if (!desc->sec_state.encrypted) {

        printAString("Encrypt connection failed - disconnecting");

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
    printAString("CUSTOM notify service not found.");
    return false;
  }
  printAString("unsubscribed to notify");
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
    printAString("CUSTOM notify service not found.");
    return false;
  }
  printAString("subscribed to notify");
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
          printAString("Wrote new value to: ");
          printAString(pChr->getUUID().toString().c_str());
        }
        else {
          return false;
        }
      }
      else {
        byte bArray[] = {0x57, 0x02}; // write to get settings of device
        if (pChr->writeValue(bArray, 2)) {
          printAString("Wrote new value to: ");
          printAString(pChr->getUUID().toString().c_str());
        }
        else {
          return false;
        }
      }
    }
    else {
      printAString("CUSTOM write service not found.");

      return false;
    }

    printAString("Success! subscribed and got settings");

    return true;
  }

  return false;
}






/** Define a class to handle the callbacks when advertisments are received */
class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {

    void checkToContinueScan() {
      bool stopScan = false;
      if (client.isConnected()) {
        if (((allContactSensors.size() + allMotionSensors.size() + allPlugs.size() + allMeters.size()) != 0) || alwaysActiveScan ) {
          if (allSwitchbotsDev.size() == (allBots.size() + allCurtains.size() + allMeters.size() + allContactSensors.size() + allMotionSensors.size() + allPlugs.size())) {
            if (!initialScanComplete) {
              initialScanComplete = true;
              stopScan = true;
            }
            else if (overrideScan) {
              stopScan = true;
            }
          }
          if (!(commandQueue.isEmpty()) && initialScanComplete && !overrideScan) {
            stopScan = true;
            forceRescan = true;
            lastUpdateTimes = {};
            allSwitchbotsScanned = {};
          }
        }
        else {
          if ((allSwitchbotsDev.size() == (allBots.size() + allCurtains.size() + allMeters.size() + allContactSensors.size() + allMotionSensors.size() + allPlugs.size())) && (allSwitchbotsScanned.size() == (allBots.size() + allCurtains.size() + allMeters.size() + allContactSensors.size() + allMotionSensors.size() + allPlugs.size())))  {
            stopScan = true;
            forceRescan = false;
            allSwitchbotsScanned = {};
          }
          else if (!(commandQueue.isEmpty()) && initialScanComplete && !overrideScan) {
            forceRescan = true;
            lastUpdateTimes = {};
            stopScan = true;
            allSwitchbotsScanned = {};
          }
          else if (overrideScan && (allSwitchbotsDev.size() == (allBots.size() + allCurtains.size() + allMeters.size() + allContactSensors.size() + allMotionSensors.size() + allPlugs.size()))) {
            stopScan = true;
            allSwitchbotsScanned = {};
          }

        }
      }

      if (stopScan) {
        printAString("Stopping Scan found devices ... ");
        NimBLEDevice::getScan()->stop();
      }
      else {

        bool shouldActiveScan = false;
        if (allSwitchbotsDev.size() != (allBots.size() + allCurtains.size() + allMeters.size() + allContactSensors.size() + allMotionSensors.size() + allPlugs.size()))
        {
          shouldActiveScan = true;
        }
        std::string anAddr;
        std::map<std::string, std::string>::iterator itT = allSwitchbotsOpp.begin();
        while ((itT != allSwitchbotsOpp.end()) && !shouldActiveScan)
        {
          anAddr = itT->first;
          shouldActiveScan = shouldActiveScanForDevice(anAddr);
          itT++;
        }

        if (!alwaysActiveScan && !onlyActiveScan) {
          if ( shouldActiveScan && !isActiveScan)   {
            isActiveScan = true;
            NimBLEDevice::getScan()->stop();
          }
          else if (!shouldActiveScan && isActiveScan) {
            isActiveScan = false;
            NimBLEDevice::getScan()->stop();
          }
        }
      }
    }
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
      printAString("START onResult");
      printAString("Advertised Device found: ");
      printAString(advertisedDevice->toString().c_str());
      if (ledOnScan) {
        digitalWrite(LED_BUILTIN, ledONValue);
      }
      publishLastwillOnline();
      std::string advStr = advertisedDevice->getAddress().toString();
      std::map<std::string, std::string>::iterator itS = allSwitchbotsOpp.find(advStr);
      bool gotAllStatus = false;

      if (itS != allSwitchbotsOpp.end())
      {
        /*if (!(NimBLEDevice::onWhiteList(advertisedDevice->getAddress()))) {
          NimBLEDevice::whiteListAdd(advertisedDevice->getAddress());
          }*/
        std::string deviceName = itS->second.c_str();
        if ((advertisedDevice->isAdvertisingService(NimBLEUUID("cba20d00-224d-11e6-9fb8-0002a5d5c51b"))) || isBotDevice(deviceName) || isCurtainDevice(deviceName) || isPlugDevice(deviceName) || isContactDevice(deviceName) || isMotionDevice(deviceName) || isMeterDevice(deviceName))
        {
          std::map<std::string, NimBLEAdvertisedDevice*>::iterator itY;
          itY = allSwitchbotsScanned.find(advStr);
          if (itY != allSwitchbotsScanned.end())
          {
            allSwitchbotsScanned.erase(advStr);
          }
          itY = allSwitchbotsScanned.find(advStr);
          if (((itY == allSwitchbotsScanned.end()) || alwaysMQTTUpdate) && client.isConnected())
          {
            if (home_assistant_mqtt_discovery) {
              std::map<std::string, bool>::iterator itM = discoveredDevices.find(advStr.c_str());
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
                else if (isPlugDevice(deviceName)) {
                  publishHomeAssistantDiscoveryPlugConfig(deviceName, advStr, home_assistant_use_opt_mode);
                }
                printAString("adding discovered device ... ");
                printAString(advStr.c_str());
                discoveredDevices[advStr.c_str()] = true;
                delay(100);
              }
            }

            printAString("Adding Our Service ... ");
            printAString(itS->second.c_str());

            std::string aValueString = "";
            if (isActiveScan) {

              if (client.isConnected()) {
                if (isPlugDevice(deviceName)) {
                  aValueString = advertisedDevice->getManufacturerData();
                  gotAllStatus = callForInfoAdvDev(advertisedDevice->getAddress().toString(), advertisedDevice->getRSSI() , aValueString);
                }
                else {
                  aValueString = advertisedDevice->getServiceData(0);
                  gotAllStatus = callForInfoAdvDev(advertisedDevice->getAddress().toString(), advertisedDevice->getRSSI() , aValueString);
                }
              }
              if (gotAllStatus) {
                allSwitchbotsScanned[advStr] = advertisedDevice;
                allSwitchbotsDev[advStr] = advertisedDevice;
                std::map<std::string, unsigned long>::iterator itR = rescanTimes.find(advStr);
                if (itR != rescanTimes.end())
                {
                  if (isCurtainDevice(deviceName)) {
                    if ((millis() - (itR->second) ) > (defaultCurtainScanAfterControlSecs * 1000)) {
                      rescanTimes.erase(advStr);
                    }
                  }
                  else  {
                    rescanTimes.erase(advStr);
                  }
                }
                lastActiveScanTimes[advStr] = millis();
                /* if (isContactDevice(deviceName) || isMotionDevice(deviceName) || isMeterDevice(deviceName)) {
                   rescanTimes[advStr] = millis();
                  }*/

                printAString("Assigned advDevService");
              }
            }
            else {
              if (isContactDevice(deviceName) || isMotionDevice(deviceName) || isPlugDevice(deviceName) || isMeterDevice(deviceName)) {
                aValueString = advertisedDevice->getManufacturerData();
                callForInfoAdvDev(advertisedDevice->getAddress().toString(), advertisedDevice->getRSSI() , aValueString);
              }

            }
          }
        }

      }
      else {
        NimBLEDevice::addIgnored(advStr);
      }
      //waitForDeviceCreation = false;

      checkToContinueScan();
      printAString("END onResult");
    };



    bool callForInfoAdvDev(std::string deviceMac, long anRSSI,  std::string & aValueString) {
      yield();
      printAString("START callForInfoAdvDev");
      printAString("callForInfoAdvDev");
      if ((strcmp(deviceMac.c_str(), "") == 0)) {
        return false;
      }
      if ((strcmp(aValueString.c_str(), "") == 0)) {
        return false;
      }

      std::map<std::string, std::string>::iterator itS = allSwitchbotsOpp.find(deviceMac);
      if (itS == allSwitchbotsOpp.end())
      {
        return false;
      }

      std::string deviceName;
      itS = deviceTypes.find(deviceMac);
      if (itS != deviceTypes.end())
      {
        deviceName = itS->second.c_str();
      }

      int aLength = aValueString.length();
      if (deviceName == botName) {
        if (aLength < 3) {
          return false;
        }
        addToAdvDevData(deviceMac, anRSSI,  aValueString, isActiveScan);
      }
      else if (deviceName == meterName) {
        if (aLength < 6) {
          return false;
        }

        if (((aLength < 6) && isActiveScan) || (!isActiveScan && (aLength < 13))) {
          return false;
        }

        addToAdvDevData(deviceMac, anRSSI,  aValueString, isActiveScan);
      }
      else if (deviceName == motionName) {

        if (aLength < 6) {
          return false;
        }

        if (((aLength < 6) && isActiveScan) || (!isActiveScan && (aLength < 12))) {
          return false;
        }

        addToAdvDevData(deviceMac, anRSSI,  aValueString, isActiveScan);
      }
      else if (deviceName == contactName) {
        if (aLength < 9) {
          return false;
        }

        if (((aLength < 9) && isActiveScan) || (!isActiveScan && (aLength < 15))) {
          return false;
        }
        addToAdvDevData(deviceMac, anRSSI,  aValueString, isActiveScan);
      }
      else if (deviceName == plugName) {
        if (aLength < 12) {
          return false;
        }

        addToAdvDevData(deviceMac, anRSSI,  aValueString, isActiveScan);
      }
      else if (deviceName == curtainName) {
        if (aLength < 5) {
          return false;
        }
        addToAdvDevData(deviceMac, anRSSI,  aValueString, isActiveScan);
      }
      else {
        return false;
      }
      yield();
      printAString("END callForInfoAdvDev");
      return true;

    };
};

void initialScanEndedCB(NimBLEScanResults results) {
  printAString("START initialScanEndedCB");
  //pScan->setFilterPolicy(BLE_HCI_SCAN_FILT_USE_WL);
  lastOnlinePublished = (((millis() - 60000) > 0) ? (millis() - 60000) : 0);
  yield();
  if (!alwaysActiveScan && !onlyActiveScan & initialScanComplete) {
    isActiveScan = false;
  }
  pScan->setActiveScan(isActiveScan);
  delay(50);
  printAString("initialScanEndedCB");
  if (ledOnBootScan) {
    digitalWrite(LED_BUILTIN, ledOFFValue);
  }
  initialScanComplete = true;
  isRescanning = false;
  allSwitchbotsScanned = {};
  delay(20);
  printAString("Scan Ended");
  /* if (allSwitchbotsDev.size() == (allBots.size() + allCurtains.size() + allMeters.size() + allContactSensors.size() + allMotionSensors.size())) {
     pScan->setFilterPolicy(BLE_HCI_SCAN_FILT_USE_WL);
     }*/

  client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"idle\"}");
  printAString("END initialScanEndedCB");
}

void scanEndedCB(NimBLEScanResults results) {
  printAString("START scanEndedCB");
  lastOnlinePublished = (((millis() - 60000) > 0) ? (millis() - 60000) : 0);
  yield();
  if (!alwaysActiveScan && !onlyActiveScan & initialScanComplete) {
    isActiveScan = false;
  }
  pScan->setActiveScan(isActiveScan);
  if (ledOnScan || ledOnCommand) {
    digitalWrite(LED_BUILTIN, ledOFFValue);
  }
  allSwitchbotsScanned = {};
  delay(50);
  printAString("Scan Ended");
  /*if (allSwitchbotsDev.size() == (allBots.size() + allCurtains.size() + allMeters.size() + allContactSensors.size() + allMotionSensors.size())) {

    pScan->setFilterPolicy(BLE_HCI_SCAN_FILT_USE_WL);
    } */

  client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"idle\"}");

  printAString("END scanEndedCB");
}

void rescanEndedCB(NimBLEScanResults results) {
  printAString("START rescanEndedCB");
  lastOnlinePublished = (((millis() - 60000) > 0) ? (millis() - 60000) : 0);
  yield();
  if (!alwaysActiveScan && !onlyActiveScan & initialScanComplete) {
    isActiveScan = false;
  }
  pScan->setActiveScan(isActiveScan);
  if (ledOnScan || ledOnCommand) {
    digitalWrite(LED_BUILTIN, ledOFFValue);
  }
  isRescanning = false;
  lastRescan = millis();
  allSwitchbotsScanned = {};
  delay(50);
  printAString("ReScan Ended");

  /* if (allSwitchbotsDev.size() == (allBots.size() + allCurtains.size() + allMeters.size() + allContactSensors.size() + allMotionSensors.size())) {

      pScan->setFilterPolicy(BLE_HCI_SCAN_FILT_USE_WL);
    } */
  client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"idle\"}");

  printAString("END rescanEndedCB");
}

void scanForeverEnded(NimBLEScanResults results) {
  printAString("START scanForeverEnded");
  lastOnlinePublished = (((millis() - 60000) > 0) ? (millis() - 60000) : 0);
  yield();
  pScan->setActiveScan(isActiveScan);
  if (ledOnScan || ledOnCommand) {
    digitalWrite(LED_BUILTIN, ledOFFValue);
  }
  isRescanning = false;
  allSwitchbotsScanned = {};
  delay(50);
  printAString("forever scan Ended");

  /*  if (allSwitchbotsDev.size() == (allBots.size() + allCurtains.size() + allMeters.size() + allContactSensors.size() + allMotionSensors.size())) {

     pScan->setFilterPolicy(BLE_HCI_SCAN_FILT_USE_WL);
     } */
  client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"idle\"}");

  printAString("END scanForeverEnded");
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

uint32_t getPassCRC(std::string & aDevice) {
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

  if (strcmp(hostForScan, hostForControl) != 0) {
    isMeshNode = true;
  }

  if (isMeshNode) {
    if (meshMeters) {
      meterTopic = ESPMQTTTopicMesh + "/meter/";
    }
    if (meshContactSensors) {
      contactTopic = ESPMQTTTopicMesh + "/contact/";
    }
    if (meshMotionSensors) {
      motionTopic = ESPMQTTTopicMesh + "/motion/";
    }
  }

  forceRescan = false;
  pinMode (LED_BUILTIN, OUTPUT);
  Serial.begin(115200);
  // Connect to WiFi network
  WiFi.begin(ssid, password);
  printAString("");

  // Wait for connection
  /*while (WiFi.status() != WL_CONNECTED) {
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
  */
  /*use mdns for host name resolution*/
  /*  if (!MDNS.begin(host)) { //http://esp32.local
      if (printSerialOutputForDebugging) {
        Serial.println("Error setting up MDNS responder!");
      }
      while (1) {
        delay(1000);
      }
    }
    if (printSerialOutputForDebugging) {
      Serial.println("mDNS responder started");
    }*/
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

  client.setMqttReconnectionAttemptDelay(10);
  client.enableLastWillMessage(lastWill, "offline", true);
  client.setKeepAlive(60);
  client.setMaxPacketSize(mqtt_packet_size);
  //client.enableMQTTPersistence();

  static std::map<std::string, std::string> allBotsTemp = {};
  static std::map<std::string, std::string> allCurtainsTemp = {};
  static std::map<std::string, std::string> allMetersTemp = {};
  static std::map<std::string, std::string> allContactSensorsTemp = {};
  static std::map<std::string, std::string> allMotionSensorsTemp = {};
  static std::map<std::string, std::string> allPlugsTemp = {};
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
    NimBLEDevice::whiteListAdd(NimBLEAddress(anAddr, 1));
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
    NimBLEDevice::whiteListAdd(NimBLEAddress(anAddr, 1));
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
    NimBLEDevice::whiteListAdd(NimBLEAddress(anAddr, 1));
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
    NimBLEDevice::whiteListAdd(NimBLEAddress(anAddr, 1));
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
    NimBLEDevice::whiteListAdd(NimBLEAddress(anAddr, 1));
    it++;
  }
  allMotionSensors = allMotionSensorsTemp;

  it = allPlugs.begin();
  while (it != allPlugs.end())
  {
    aName = it->first;
    anAddr = it->second;
    std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
    std::replace( aName.begin(), aName.end(), ' ', '_');
    allSwitchbotsOpp[anAddr.c_str()] = aName;
    allSwitchbots[aName] = anAddr.c_str();
    deviceTypes[anAddr.c_str()] = plugName;
    allPlugsTemp[aName] = anAddr.c_str();
    NimBLEDevice::whiteListAdd(NimBLEAddress(anAddr, 1));
    it++;
  }
  allPlugs = allPlugsTemp;

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

  Serial.println("Switchbot ESP32 starting...");
  if (!printSerialOutputForDebugging) {
    Serial.println("Set printSerialOutputForDebugging = true to see more Serial output");
  }
  printAString("Starting NimBLE Client");

  NimBLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM |*/ BLE_SM_PAIR_AUTHREQ_SC);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  //NimBLEDevice::setScanFilterMode(2);
  pScan = NimBLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  pScan->setInterval(70);
  pScan->setWindow(40);
  pScan->setDuplicateFilter(false);
  isActiveScan = true;
  pScan->setActiveScan(isActiveScan);
  pScan->setMaxResults(100);
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
  if (onlyPassiveScan && initialScanComplete) {
    isActiveScan = false;
    delay(50);
    addToPublish(ESPMQTTTopic.c_str(), "{\"status\":\"passivescanning\"}");
  }
  else {
    isRescanning = true;
    isActiveScan = true;
    delay(50);
    addToPublish(ESPMQTTTopic.c_str(), "{\"status\":\"activescanning\"}");
  }

  pScan->setActiveScan(isActiveScan);

  delay(50);
  if (ledOnScan) {
    digitalWrite(LED_BUILTIN, ledONValue);
  }
  pScan->start(seconds, rescanEndedCB, true);
}

void scanForever() {
  //lastRescan = millis();
  while (pScan->isScanning()) {
    delay(50);
  }
  //allSwitchbotsScanned = {};
  //lastRescan = millis();

  if (onlyPassiveScan && initialScanComplete) {
    isActiveScan = false;
  }

  if (isActiveScan) {
    lastRescan = millis();
    isRescanning = true;
    addToPublish(ESPMQTTTopic.c_str(), "{\"status\":\"activescanning\"}");
  }
  else {
    addToPublish(ESPMQTTTopic.c_str(), "{\"status\":\"passivescanning\"}");
  }
  delay(50);
  pScan->setActiveScan(isActiveScan);
  if (ledOnScan) {
    digitalWrite(LED_BUILTIN, ledONValue);
  }
  pScan->start(0, scanForeverEnded, true);
}

void rescanFind(std::string aMac) {
  if (isRescanning) {
    return;
  }
  while (pScan->isScanning()) {
    delay(50);
  }

  if (onlyPassiveScan && initialScanComplete) {
    isActiveScan = false;
    delay(100);
    addToPublish(ESPMQTTTopic.c_str(), "{\"status\":\"passivescanning\"}");
  }
  else {
    isActiveScan = true;
    delay(100);
    addToPublish(ESPMQTTTopic.c_str(), "{\"status\":\"activescanning\"}");
  }
  pScan->setActiveScan(isActiveScan);

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

  delay(50);
  if (ledOnScan) {
    digitalWrite(LED_BUILTIN, ledONValue);
  }
  pScan->start(infoScanTime, scanEndedCB, true);
}

void getAllBotSettings() {
  if (client.isConnected() && initialScanComplete) {
    if (ledOnBootScan) {
      digitalWrite(LED_BUILTIN, ledONValue);
    }

    printAString("In all get bot settings...");
    gotSettings = true;
    std::map<std::string, std::string>::iterator itT = allBots.begin();
    std::string aDevice;
    std::string aMac;
    while (itT != allBots.end())
    {
      aDevice = itT->first;
      aMac = itT->second;
      bool hasFirmware = false;
      bool hasTimers = false;
      bool hasHold = false;
      bool hasInverted = false;
      std::map<std::string, int>::iterator itP = botHoldSecs.find(aMac);
      if (itP != botHoldSecs.end()) {
        hasHold = true;
      }
      std::map<std::string, int>::iterator itX = botNumTimers.find(aMac);
      if (itX != botNumTimers.end()) {
        hasTimers = true;
      }
      std::map<std::string, bool>::iterator itU = botInverteds.find(aMac);
      if (itU != botInverteds.end()) {
        hasInverted = true;
      }
      std::map<std::string, const char *>::iterator itZ = botFirmwares.find(aMac);
      if (itZ != botFirmwares.end()) {
        hasFirmware = true;
      }
      if ((!hasFirmware) || (!hasTimers) || (!hasHold) || (!hasInverted)) {
        printAString("get settings");

        processing = true;
        client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"getsettings\"}");
        controlMQTT(aDevice, "REQUESTSETTINGS", true);
        client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"idle\"}");

      }
      else {
        printAString("Skip settings");
      }
      processing = false;
      itT++;
    }
    if (ledOnBootScan) {
      digitalWrite(LED_BUILTIN, ledOFFValue);
    }
    printAString("Added all get bot settings...");
  }
}

unsigned long retainStartTime = 0;
bool waitForRetained = true;
unsigned long lastWebServerReboot = 0;

void checkWebServer() {
  if ((millis() - lastWebServerReboot) > (30 * 1000 * 60)) {
    server.close();
    server.begin();
    lastWebServerReboot = millis();
  }
}

void loop () {
  //printAString("START loop...");
  client.loop();
  checkWebServer();
  server.handleClient();
  //printAString("at processAllAdvData...");
  processAllAdvData();
  //printAString("at publishLastwillOnline...");
  publishLastwillOnline();
  //printAString("at publishAllMQTT...");
  publishAllMQTT();
  //printAString("at processContactSensorTasks...");
  processContactSensorTasks();
  //printAString("at processMotionSensorTasks...");
  processMotionSensorTasks();
  //printAString("at loopcode...");
  if (waitForRetained && client.isConnected() && !manualDebugStartESP32WithMQTT) {
    if (retainStartTime == 0) {
      client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"waiting\"}");
      retainStartTime = millis();
    }
    else if ((millis() - retainStartTime) > (waitForMQTTRetainMessages * 1000)) {
      waitForRetained = false;
    }
    /* if ((botFirmwares.size() >= allBots.size()) && (botInverteds.size() >= allBots.size()) && (botNumTimers.size() >= allBots.size()) && (botHoldSecs.size() >= allBots.size()) && (botsSimulatedStates.size() >= botsSimulateONOFFinPRESSmode.size()))
      {
       waitForRetained = false;
      }*/
  }

  if ((!initialScanComplete) && client.isConnected() && (!waitForResponse) && (!processing) && (!(pScan->isScanning())) && (!isRescanning) && (!waitForRetained) && !manualDebugStartESP32WithMQTT) {
    addToPublish(ESPMQTTTopic.c_str(), "{\"status\":\"activescanning\"}");
    isRescanning = true;
    isActiveScan = true;
    pScan->setActiveScan(isActiveScan);
    delay(50);
    pScan->start(initialScan, initialScanEndedCB, true);
  }

  if (initialScanComplete && client.isConnected() && !manualDebugStartESP32WithMQTT) {
    if (isRescanning) {
      lastRescan = millis();
    }
    if ((!waitForResponse) && (!processing) && (!(pScan->isScanning())) && (!isRescanning)) {
      if (getSettingsOnBoot && !gotSettings ) {
        getAllBotSettings();
      }
    }

    if (((allContactSensors.size() + allMotionSensors.size() + allPlugs.size() + allMeters.size()) != 0) || alwaysActiveScan) {
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
          if (scanAfterControl || activeScanOnSchedule) {
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
  //printAString("END loop...");
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
      unsigned long tempVal = (millis() - ((rescanTime * 1000) - 5000));
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

bool shouldMQTTUpdateOrActiveScanForDevice(std::string & anAddr) {
  return (shouldMQTTUpdateForDevice(anAddr) || shouldActiveScanForDevice(anAddr)  );
}


bool shouldActiveScanForDevice(std::string & anAddr) {
  //yield();
  if (!client.isConnected()) {
    return false;
  }
  std::map<std::string, std::string>::iterator itB = allSwitchbotsOpp.find(anAddr);
  unsigned long lastActiveScanTime = 0;
  std::map<std::string, unsigned long>::iterator itX = lastActiveScanTimes.find(anAddr);
  if (itX != lastActiveScanTimes.end())
  {
    lastActiveScanTime = itX->second;
  }
  else
  {
    return true;
  }

  if ((millis() - lastActiveScanTime) >= (rescanTime * 1000)) {
    return true;
  }

  if (isBotDevice(itB->second)) {
    if ((millis() - lastActiveScanTime) >= (defaultBotActiveScanSecs * 1000)) {
      return true;
    }
  }
  else if (isCurtainDevice(itB->second)) {
    if ((millis() - lastActiveScanTime) >= (defaultCurtainActiveScanSecs * 1000)) {
      return true;
    }
  }
  else if (isMeterDevice(itB->second)) {
    if ((millis() - lastActiveScanTime) >= (defaultMeterActiveScanSecs * 1000)) {
      return true;
    }
  }
  else if (isContactDevice(itB->second)) {
    if ((millis() - lastActiveScanTime) >= (defaultContactActiveScanSecs * 1000)) {
      return true;
    }
  }
  else if (isMotionDevice(itB->second)) {
    if ((millis() - lastActiveScanTime) >= (defaultMotionActiveScanSecs * 1000)) {
      return true;
    }
  }
  else if (isPlugDevice(itB->second)) {
    if ((millis() - lastActiveScanTime) >= (defaultPlugActiveScanSecs * 1000)) {
      return true;
    }
  }

  if (isBotDevice(itB->second) || isCurtainDevice(itB->second)) {
    std::map<std::string, int>::iterator itS = botScanTime.find(itB->second);
    unsigned long lastTime;
    std::map<std::string, unsigned long>::iterator it = rescanTimes.find(anAddr);
    if (it != rescanTimes.end())
    {
      lastTime = it->second;
    }
    else {
      return false;
    }

    unsigned long scanTime = defaultBotScanAfterControlSecs;
    if (isCurtainDevice(itB->second)) {
      scanTime = defaultCurtainScanAfterControlSecs;
    }
    /* else if (isMeterDevice(itB->second)) {
       scanTime = defaultMeterMQTTUpdateSecs;
      }*/
    if (itS != botScanTime.end())
    {
      scanTime = itS->second;
    }
    if (isCurtainDevice(itB->second)) {
      if (scanWhileCurtainIsMoving && ((millis() - lastTime ) <= (scanTime * 1000))) {
        return true;
      }
    }
    else if (isBotDevice(itB->second)) {
      std::map<std::string, bool>::iterator itP = botsInPressMode.find(anAddr);
      if (itP != botsInPressMode.end())
      {
        std::map<std::string, int>::iterator itH = botHoldSecs.find(anAddr);
        if (itH != botHoldSecs.end())
        {
          int holdTimePlus = (itH->second) + defaultBotScanAfterControlSecs;
          if (holdTimePlus > scanTime) {
            scanTime =  holdTimePlus;
          }
        }
      }
    }

    if ((millis() - lastTime) >= (scanTime * 1000)) {
      return true;
    }
  }

  return false;
}

bool shouldMQTTUpdateForDevice(std::string & anAddr) {
  //yield();
  if (!client.isConnected()) {
    return false;
  }

  if (alwaysMQTTUpdate) {
    return true;
  }

  unsigned long lastUpdateTime = 0;
  std::map<std::string, unsigned long>::iterator itX = lastUpdateTimes.find(anAddr);
  if (itX != lastUpdateTimes.end())
  {
    lastUpdateTime = itX->second;
  }
  else
  {
    return true;
  }

  if ((millis() - lastUpdateTime) >= (rescanTime * 1000)) {
    return true;
  }
  std::map<std::string, std::string>::iterator itB = allSwitchbotsOpp.find(anAddr);
  std::map<std::string, std::string>::iterator itM = allMeters.find(itB->second);
  if (itM != allMeters.end())
  {
    if ((lastUpdateTime == 0 ) || ((millis() - lastUpdateTime) >= (defaultMeterMQTTUpdateSecs * 1000))) {
      return true;
    }
  }

  itM = allContactSensors.find(itB->second);
  if (itM != allContactSensors.end())
  {
    if ((lastUpdateTime == 0 ) || ((millis() - lastUpdateTime) >= (defaultContactMQTTUpdateSecs * 1000))) {
      return true;
    }
  }
  itM = allMotionSensors.find(itB->second);
  if (itM != allMotionSensors.end())
  {
    if ((lastUpdateTime == 0 ) || ((millis() - lastUpdateTime) >= (defaultMotionMQTTUpdateSecs * 1000))) {
      return true;
    }
  }

  itM = allPlugs.find(itB->second);
  if (itM != allPlugs.end())
  {
    if ((lastUpdateTime == 0 ) || ((millis() - lastUpdateTime) >= (defaultPlugMQTTUpdateSecs * 1000))) {
      return true;
    }
  }

  itM = allCurtains.find(itB->second);
  if (itM != allCurtains.end())
  {
    if ((lastUpdateTime == 0 ) || ((millis() - lastUpdateTime) >= (defaultCurtainMQTTUpdateSecs * 1000))) {
      return true;
    }
  }

  itM = allBots.find(itB->second);
  if (itM != allBots.end())
  {
    if ((lastUpdateTime == 0 ) || ((millis() - lastUpdateTime) >= (defaultBotMQTTUpdateSecs * 1000))) {
      return true;
    }
  }

  return false;
}

void recurringScan() {
  if ((millis() - lastScanCheck) >= 200) {
    std::string anAddr;
    std::map<std::string, std::string>::iterator itS = allSwitchbotsOpp.begin();
    while (itS != allSwitchbotsOpp.end())
    {
      anAddr = itS->first;
      bool shouldActiveScan = false;
      shouldActiveScan = shouldActiveScanForDevice(anAddr);
      if (shouldActiveScan) {
        if (onlyPassiveScan && initialScanComplete) {
          isActiveScan = false;
        }
        else {
          isActiveScan = true;
        }
        pScan->setActiveScan(isActiveScan);
        if (!processing && !(pScan->isScanning()) && !isRescanning) {
          rescanFind(anAddr);
          delay(100);
          std::map<std::string, unsigned long>::iterator itR = rescanTimes.find(anAddr);
          if (itR != rescanTimes.end())
          {
            std::map<std::string, std::string>::iterator itS = allSwitchbotsOpp.find(anAddr);
            std::string deviceName = itS->second.c_str();
            if (isCurtainDevice(deviceName)) {
              if ((millis() - (itR->second) ) > (defaultCurtainScanAfterControlSecs * 1000)) {
                rescanTimes.erase(anAddr);
              }
            }
            else {
              rescanTimes.erase(anAddr);
            }
          }
        }
      }
      itS++;
    }
    lastScanCheck = millis();
  }
}

/*void recurringMeterScan() {
  if (!allMeters.empty()) {
    std::map<std::string, std::string>::iterator it = allMeters.begin();
    std::string anAddr = it->second;
    while (it != allMeters.end())
    {
      bool shouldMQTTOrActiveScanUpdate = false;
      shouldMQTTOrActiveScanUpdate = shouldActiveScanForDevice(anAddr);
      if (shouldMQTTOrActiveScanUpdate) {
        rescanTimes[anAddr] = (((millis() - defaultMeterMQTTUpdateSecs) > 0) ? (millis() - defaultMeterMQTTUpdateSecs) : 0 ) ;
      }
      it++;
    }
  }
  }*/


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
        digitalWrite(LED_BUILTIN, ledONValue);
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
    char aBuffer[100];
    doc["id"] = aName.c_str();
    doc["status"] = "errorLocatingDevice";
    serializeJson(doc, aBuffer);
    addToPublish((deviceTopic + "/status").c_str(), aBuffer);
  }
  else {
    isSuccess = sendToDevice(advDevice, aName, command, deviceTopic, disconnectAfter);
  }
  return isSuccess;
}

bool waitToProcess(QueueCommand aCommand) {
  bool wait = false;
  unsigned long waitTimeLeft = 0;

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
        std::map<std::string, unsigned long>::iterator itZ = lastCommandSent.find(anAddr);
        if (itZ != lastCommandSent.end())
        {
          wait = true;
          unsigned long lastTime = itZ->second;
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
    printAString("Control for device: ");
    printAString(aCommand.device.c_str());
    printAString(" will wait ");
    printAString(waitTimeLeft);
    printAString(" millisecondSeconds");
  }
  return wait;
}

bool processQueue() {
  processing = true;
  struct QueueCommand aCommand;
  if (!commandQueue.isEmpty()) {
    bool disconnectAfter = true;
    if (ledOnCommand) {
      digitalWrite(LED_BUILTIN, ledONValue);
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
        printAString("Received something on ");
        printAString(aCommand.topic.c_str());
        printAString(aCommand.device.c_str());
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
                        addToPublish(deviceStateTopic.c_str(), "ON", true);
                        std::map<std::string, bool>::iterator itP = botsToWaitFor.find(aCommand.device);
                        if (itP != botsToWaitFor.end())
                        {
                          botsToWaitFor.erase(aCommand.device);
                        }
                      }
                      else if (!boolState && (strcmp(aCommand.payload.c_str(), "OFF") == 0)) {
                        skipProcess = true;
                        addToPublish(deviceStateTopic.c_str(), "OFF", true);
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
                digitalWrite(LED_BUILTIN, ledONValue);
              }
              noResponse = true;
              bool shouldContinue = true;
              unsigned long timeSent = millis();
              bool isSuccess = false;
              if (isBotDevice(aCommand.device.c_str()))
              {

                String tempPayload = aCommand.payload.c_str();
                int dotIndex = tempPayload.indexOf(".");
                if (dotIndex >= 0) {
                  tempPayload.remove(dotIndex, tempPayload.length() - 1);
                }
                bool isNum = is_number(tempPayload.c_str());

                if (isNum) {
                  isSuccess = controlMQTT(aCommand.device, tempPayload.c_str(), false);
                }
                else {
                  isSuccess = controlMQTT(aCommand.device, aCommand.payload, disconnectAfter);
                }
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
                        addToPublish(deviceStateTopic.c_str(), "OFF", true);
                        addToPublish(deviceAssumedStateTopic.c_str(), "OFF", true);
                      }
                      else if (strcmp(aCommand.payload.c_str(), "ON") == 0) {
                        botsSimulatedStates[aCommand.device] = true;
                        addToPublish(deviceStateTopic.c_str(), "ON", true);
                        addToPublish(deviceAssumedStateTopic.c_str(), "ON", true);
                      }
                      else if (strcmp(aCommand.payload.c_str(), "PRESS") == 0) {
                        botsSimulatedStates[aCommand.device] = !(botsSimulatedStates[aCommand.device]);
                        if (botsSimulatedStates[aCommand.device]) {
                          addToPublish(deviceStateTopic.c_str(), "ON", true);
                          addToPublish(deviceAssumedStateTopic.c_str(), "ON", true);
                        }
                        else {
                          addToPublish(deviceStateTopic.c_str(), "OFF", true);
                          addToPublish(deviceAssumedStateTopic.c_str(), "OFF", true);
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

                if (isNum && !lastCommandWasBusy) {
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
                  printAString("current retry...");
                  printAString(aCommand.currentTry);

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
                waitForResponse = false;
                noResponse = false;
              }


              else if (isPlugDevice(aCommand.device.c_str()))
              {

                String tempPayload = aCommand.payload.c_str();
                isSuccess = controlMQTT(aCommand.device, aCommand.payload, disconnectAfter);

                while (noResponse && shouldContinue )
                {
                  waitForResponse = true;
                  //if (printSerialOutputForDebugging) {Serial.println("waiting for response...");}
                  if ((millis() - timeSent) > (waitForResponseSec * 1000)) {
                    shouldContinue = false;
                  }
                }

                std::map<std::string, std::string>::iterator itN = allPlugs.find(aCommand.device);
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

                if (lastCommandWasBusy && retryPlugOnBusy) {
                  requeue = true;
                  botsToWaitFor[aCommand.device] = true;
                  lastCommandWasBusy = false;
                  getSettingsAfter = false;

                  lastCommandSent[anAddr] = 0;
                }
                else if ((retryPlugActionNoResponse && noResponse && (aCommand.currentTry <= noResponseRetryAmount))) {
                  printAString("current retry...");
                  printAString(aCommand.currentTry);

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
                waitForResponse = false;
                noResponse = false;
              }

              else if (isCurtainDevice(aCommand.device.c_str()))
              {
                controlMQTT(aCommand.device, aCommand.payload, disconnectAfter);
                std::string anAddr;

                while (noResponse && shouldContinue )
                {
                  waitForResponse = true;
                  printAString("waiting for response...");

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
                  printAString("current retry...");
                  printAString(aCommand.currentTry);

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

                waitForResponse = false;
                noResponse = false;
              }
            }

            std::map<std::string, bool>::iterator itE = botsSimulateONOFFinPRESSmode.find(aCommand.device);
            if (itE != botsSimulateONOFFinPRESSmode.end())
            {
              std::map<std::string, bool>::iterator itF = botsSimulatedStates.find(aCommand.device);
              if (itF != botsSimulatedStates.end())
              {
                bool boolState = itF->second;
                if (boolState && (strcmp(aCommand.payload.c_str(), "OFF") == 0))  {
                  addToPublish(deviceStateTopic.c_str(), "ON", true);
                }
                else if (!boolState && (strcmp(aCommand.payload.c_str(), "ON") == 0)) {
                  addToPublish(deviceStateTopic.c_str(), "OFF", true);
                }
              }
            }
          }
        }
        else if (aCommand.topic == ESPMQTTTopic + "/requestInfo") {
          if (ledOnCommand) {
            digitalWrite(LED_BUILTIN, ledONValue);
          }
          requestInfoMQTT(aCommand.payload);
        }
        else if (aCommand.topic == ESPMQTTTopic + "/rescan") {
          if (ledOnCommand) {
            digitalWrite(LED_BUILTIN, ledONValue);
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
          while (sendInitial || (lastCommandWasBusy && retryBotOnBusy) || (retryBotSetNoResponse && noResponse && (count <= noResponseRetryAmount))) {
            sendInitial = false;
            count++;
            shouldContinue = true;
            unsigned long timeSent = millis();
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
    digitalWrite(LED_BUILTIN, ledOFFValue);
  }
  processing = false;
  return true;
}

bool sendToDevice(NimBLEAdvertisedDevice * advDevice, std::string & aName, const char * command, std::string & deviceTopic, bool disconnectAfter) {
  bool isSuccess = false;
  NimBLEAdvertisedDevice* advDeviceToUse = advDevice;
  std::string addr = advDeviceToUse->getAddress().toString();
  //std::transform(addr.begin(), addr.end(), addr.begin(), ::toupper);
  addr = addr.c_str();
  std::string deviceStateTopic = deviceTopic + "/state";
  std::string deviceStatusTopic = deviceTopic + "/status";

  if ((advDeviceToUse != nullptr) && (advDeviceToUse != NULL))
  {
    char aBuffer[100];
    StaticJsonDocument<100> doc;
    //    doc["id"] = aName.c_str();
    if (strcmp(command, "requestInfo") == 0 || strcmp(command, "REQUESTINFO") == 0 || strcmp(command, "GETINFO") == 0) {
      isSuccess = requestInfo(advDeviceToUse);
      if (!isSuccess) {
        doc["status"] = "errorRequestInfo";
        doc["command"] = command;
        serializeJson(doc, aBuffer);
        client.publish(deviceStatusTopic.c_str(),  aBuffer);
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
        client.publish(deviceStatusTopic.c_str(),  aBuffer);
      }
      else {
        if (count > tryConnecting) {
          shouldContinue = false;
          doc["status"] = "errorConnect";
          doc["command"] = command;
          serializeJson(doc, aBuffer);
          client.publish(deviceStatusTopic.c_str(),  aBuffer);
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
            char aBuffer[100];
            doc["status"] = "commandSent";
            doc["command"] = command;
            serializeJson(doc, aBuffer);
            client.publish(deviceStatusTopic.c_str(), aBuffer);
          }
          lastCommandSentPublished = false;
          if (strcmp(command, "REQUESTSETTINGS") != 0 && strcmp(command, "GETSETTINGS") != 0) {

            String tempPayload = command;
            int dotIndex = tempPayload.indexOf(".");
            if (dotIndex >= 0) {
              tempPayload.remove(dotIndex, tempPayload.length() - 1);
            }
            bool isNum = is_number(tempPayload.c_str());
            bool scanAfterNum = true;
            std::string aDevice = "";
            std::map<std::string, std::string>::iterator itI = allSwitchbotsOpp.find(addr);
            aDevice = itI->second;
            if (isNum && isBotDevice(aDevice)) {
              scanAfterNum = false;
            }
            if (isNum) {
              int aVal;

              sscanf(tempPayload.c_str(), "%d", &aVal);
              if (aVal < 0) {
                aVal = 0;
              }
              else if (aVal > 100) {
                aVal = 100;
              }
              if (isCurtainDevice(aDevice)) {
                std::string devicePosTopic = deviceTopic + "/position";
                StaticJsonDocument<50> docPos;
                char aBuffer[100];
                docPos["pos"] = aVal;
                serializeJson(docPos, aBuffer);
                addToPublish(devicePosTopic.c_str(), aBuffer);
              }
            }
            else {
              std::map<std::string, bool>::iterator itP = botsInPressMode.find(addr);
              std::map<std::string, bool>::iterator itE = botsSimulateONOFFinPRESSmode.find(aDevice);
              if (itP != botsInPressMode.end() && itE == botsSimulateONOFFinPRESSmode.end())
              {
                addToPublish(deviceStateTopic.c_str(), "OFF", true);
              }
              else {
                addToPublish(deviceStateTopic.c_str(), command, true);
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
            client.publish(deviceStatusTopic.c_str(),  aBuffer);
          }
        }
      }
    }
  }
  printAString("Done sendCommand...");

  return isSuccess;
}

bool is_number(const std::string & s)
{
  std::string::const_iterator it = s.begin();
  while (it != s.end() && std::isdigit(*it)) ++it;
  return !s.empty() && it == s.end();
}

bool controlMQTT(std::string & device, std::string payload, bool disconnectAfter) {
  client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"controlling\"}");
  bool isSuccess = false;
  processing = true;
  printAString("Processing Control MQTT...");

  std::string deviceAddr = "";
  std::string deviceTopic;
  std::string anAddr;

  printAString("Device: ");
  printAString(device.c_str());
  printAString("Device value: ");
  printAString(payload.c_str());

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

  itS = allPlugs.find(device.c_str());
  if (itS != allPlugs.end())
  {
    anAddr = itS->second;
    std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
    deviceAddr = anAddr.c_str();
    deviceTopic = plugTopic;
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


    String tempPayload = payload.c_str();
    int dotIndex = tempPayload.indexOf(".");
    if (dotIndex >= 0) {
      tempPayload.remove(dotIndex, tempPayload.length() - 1);
    }

    bool isNum = is_number(tempPayload.c_str());
    deviceTopic = deviceTopic + device;
    if (isNum) {
      payload = tempPayload.c_str();
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
        char aBuffer[100];
        StaticJsonDocument<100> docOut;
        docOut["status"] = "errorJSONValue";
        serializeJson(docOut, aBuffer);
        printAString("Parsing failed = value not a valid command");
        addToPublish(ESPMQTTTopic.c_str(), aBuffer);
      }
    }
  }
  else {
    char aBuffer[100];
    StaticJsonDocument<100> docOut;
    docOut["status"] = "errorJSONDevice";
    serializeJson(docOut, aBuffer);
    printAString("Parsing failed = device not from list");
    addToPublish(ESPMQTTTopic.c_str(), aBuffer);
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

void rescanMQTT(std::string & payload) {
  isRescanning = true;
  processing = true;
  printAString("Processing Rescan MQTT...");
  StaticJsonDocument<100> docIn;
  deserializeJson(docIn, payload);

  if (docIn == nullptr) { //Check for errors in parsing
    printAString("Parsing failed");
    char aBuffer[100];
    StaticJsonDocument<100> docOut;
    docOut["status"] = "errorParsingJSON";
    serializeJson(docOut, aBuffer);
    addToPublish(ESPMQTTTopic.c_str(), aBuffer);
  }
  else {
    int value = docIn["sec"];
    String secString = String(value);
    if (strlen(secString.c_str()) != 0) {
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
        printAString("Parsing failed = device not from list");
        addToPublish(ESPMQTTTopic.c_str(), aBuffer);
      }
    }
  }
  processing = false;
}

void requestInfoMQTT(std::string & payload) {
  processing = true;
  printAString("Processing Request Info MQTT...");
  StaticJsonDocument<100> docIn;
  deserializeJson(docIn, payload);

  if (docIn == nullptr) { //Check for errors in parsing
    printAString("Parsing failed");
    char aBuffer[100];
    StaticJsonDocument<100> docOut;
    docOut["status"] = "errorParsingJSON";
    serializeJson(docOut, aBuffer);
    addToPublish(ESPMQTTTopic.c_str(), aBuffer);
  }
  else {
    const char * aName = docIn["id"]; //Get sensor type value
    printAString("Device: ");
    printAString(aName);

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
      itS = allPlugs.find(aName);
      if (itS != allPlugs.end())
      {
        anAddr = itS->second;
        std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
        deviceAddr = anAddr.c_str();
        deviceTopic = plugTopic;
      }
    }
    if (deviceAddr != "") {
      deviceTopic = deviceTopic + aName;
      processRequest(deviceAddr, aName, "requestInfo", deviceTopic, true);
    }
    else {
      char aBuffer[100];
      StaticJsonDocument<100> docOut;
      docOut["status"] = "errorJSONId";
      serializeJson(docOut, aBuffer);
      printAString("Parsing failed = device not from list");
      addToPublish(ESPMQTTTopic.c_str(), aBuffer);
    }
  }
  processing = false;
}

void onConnectionEstablished() {
  if (!MDNS.begin(host)) {
    printAString("Error starting mDNS");
  }
  printAString("Reconnected to WIFI/MQTT");
  server.close();
  server.begin();
  std::string anAddr;
  std::string aDevice;
  std::map<std::string, std::string>::iterator it;

  if (manualDebugStartESP32WithMQTT) {
    client.subscribe((ESPMQTTTopic + "/manualstart").c_str(), [aDevice] (const String & payload)  {
      if ((payload != NULL) && !(payload.isEmpty())) {
        printAString("Manually starting ESP32...");
        manualDebugStartESP32WithMQTT = false;
        client.unsubscribe((ESPMQTTTopic + "/manualstart").c_str());
        onConnectionEstablished();
      }
    });
  }

  else {
    if (!deviceHasBooted) {
      deviceHasBooted = true;
      if (ledOnBootScan) {
        digitalWrite(LED_BUILTIN, ledONValue);
      }
      client.publish(ESPMQTTTopic.c_str(), "{\"status\":\"boot\"}");
      client.publish((esp32Topic + "/firmware").c_str(), versionNum, true);

      delay(100);
      it = allBots.begin();
      while (it != allBots.end())
      {
        std::string deviceStr ;
        aDevice = it->first.c_str();
        client.subscribe((botTopic + aDevice + "/assumedstate").c_str(), [aDevice] (const String & payload)  {
          if ((payload != NULL) && !(payload.isEmpty())) {
            printAString("state MQTT Received (from retained)...updating ON/OFF simulate states");

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
                  printAString("settings the value");
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
          }
          client.unsubscribe((botTopic + aDevice + "/assumedstate").c_str());
        });

        client.subscribe((botTopic + aDevice + "/settings").c_str(), [aDevice] (const String & payload)  {
          if ((payload != NULL) && !(payload.isEmpty())) {
            printAString("settings MQTT Received (from retained)...updating firmware/timers/hold");
            StaticJsonDocument<100> docIn;
            if (isBotDevice(aDevice)) {
              printAString("going thru bot settings retained");
              std::map<std::string, std::string>::iterator itP = allBots.find(aDevice);
              if (itP != allBots.end())
              {
                std::string aMac = itP->second.c_str();
                deserializeJson(docIn, payload.c_str());
                if (docIn.containsKey("firmware")) {
                  printAString("contains firmware");
                  const char * firmware = docIn["firmware"];
                  botFirmwares[aMac] = firmware;
                }
                if (docIn.containsKey("timers")) {
                  printAString("contains timers");
                  botNumTimers[aMac] = docIn["timers"];
                }
                if (docIn.containsKey("hold")) {
                  printAString("contains hold");
                  botHoldSecs[aMac] = docIn["hold"];
                }
                if (docIn.containsKey("inverted")) {
                  printAString("contains inverted");
                  botInverteds[aMac] = docIn["inverted"];
                }
              }
            }
          }
          client.unsubscribe((botTopic + aDevice + "/settings").c_str());
        });
        it++;
      }
    }
    addToPublish(lastWill, "online", true);

    it = allCurtains.begin();
    while (it != allCurtains.end())
    {
      std::string deviceStr ;
      aDevice = it->first.c_str();
      client.subscribe((curtainTopic + aDevice + "/set").c_str(), [aDevice] (const String & payload)  {
        if ((payload != NULL) && !(payload.isEmpty())) {
          printAString("Control MQTT Received...");
          if (pScan->isScanning() || isRescanning) {
            if (pScan->isScanning()) {
              pScan->stop();
            }
            allSwitchbotsScanned = {};
            forceRescan = true;
            lastUpdateTimes = {};
          }
          if (!commandQueue.isFull()) {
            if (immediateCurtainStateUpdate && isCurtainDevice(aDevice)) {
              std::string deviceStateTopic = curtainTopic + aDevice + "/state";
              std::string devicePosTopic = curtainTopic + aDevice + "/position";
              std::map<std::string, std::string>::iterator itP = allCurtains.find(aDevice);
              if (itP != allCurtains.end())
              {
                std::string aMac = itP->second.c_str();

                String tempPayload = payload.c_str();
                int dotIndex = tempPayload.indexOf(".");
                if (dotIndex >= 0) {
                  tempPayload.remove(dotIndex, tempPayload.length() - 1);
                }

                bool isNum = is_number(tempPayload.c_str());

                if (isNum) {
                  int aVal;
                  sscanf(tempPayload.c_str(), "%d", &aVal);
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
                  addToPublish(devicePosTopic.c_str(), aBuffer);
                }
                else if ((strcmp(payload.c_str(), "OPEN") == 0))  {
                  addToPublish(deviceStateTopic.c_str(), "OPEN", true);
                } else if ((strcmp(payload.c_str(), "CLOSE") == 0))  {
                  addToPublish(deviceStateTopic.c_str(), "CLOSE", true);
                } else if ((strcmp(payload.c_str(), "PAUSE") == 0)) {
                  addToPublish(deviceStateTopic.c_str(), "PAUSE", true);
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
        if ((payload != NULL) && !(payload.isEmpty())) {
          printAString("Control MQTT Received...");

          std::map<std::string, bool>::iterator itE = botsSimulateONOFFinPRESSmode.find(aDevice);
          std::string deviceStateTopic = botTopic + aDevice + "/state";
          if (itE != botsSimulateONOFFinPRESSmode.end() && ((strcmp(payload.c_str(), "STATEOFF") == 0) || (strcmp(payload.c_str(), "STATEON") == 0))) {
            std::string deviceAssumedStateTopic = botTopic + aDevice + "/assumedstate";
            if (strcmp(payload.c_str(), "STATEOFF") == 0) {
              botsSimulatedStates[aDevice] = false;
              addToPublish(deviceStateTopic.c_str(), "OFF", true);
              addToPublish(deviceAssumedStateTopic.c_str(), "OFF", true);
            }
            else if (strcmp(payload.c_str(), "STATEON") == 0) {
              botsSimulatedStates[aDevice] = true;
              addToPublish(deviceStateTopic.c_str(), "ON", true);
              addToPublish(deviceAssumedStateTopic.c_str(), "ON", true);
            }
          }
          else {
            if (pScan->isScanning() || isRescanning) {
              if (pScan->isScanning()) {
                pScan->stop();
              }
              allSwitchbotsScanned = {};
              forceRescan = true;
              lastUpdateTimes = {};
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
                      addToPublish(deviceStateTopic.c_str(), "OFF", true);
                    }
                    else {
                      if ((strcmp(payload.c_str(), "OFF") == 0)) {
                        addToPublish(deviceStateTopic.c_str(), "OFF", true);
                      } else if ((strcmp(payload.c_str(), "ON") == 0)) {
                        addToPublish(deviceStateTopic.c_str(), "ON", true);
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
        }
      });

      it++;
    }


    it = allPlugs.begin();
    while (it != allPlugs.end())
    {
      std::string deviceStr ;
      aDevice = it->first.c_str();

      client.subscribe((plugTopic + aDevice + "/set").c_str(), [aDevice] (const String & payload)  {
        if ((payload != NULL) && !(payload.isEmpty())) {
          printAString("Control MQTT Received...");
          std::string deviceStateTopic = plugTopic + aDevice + "/state";

          if (pScan->isScanning() || isRescanning) {
            if (pScan->isScanning()) {
              pScan->stop();
            }
            allSwitchbotsScanned = {};
            forceRescan = true;
            lastUpdateTimes = {};
          }
          if (!commandQueue.isFull()) {
            if (isPlugDevice(aDevice)) {
              std::map<std::string, std::string>::iterator itP = allPlugs.find(aDevice);
              if (itP != allPlugs.end())
              {
                std::string aMac = itP->second.c_str();
                if (immediatePlugStateUpdate) {
                  if ((strcmp(payload.c_str(), "OFF") == 0)) {
                    addToPublish(deviceStateTopic.c_str(), "OFF", true);
                  } else if ((strcmp(payload.c_str(), "ON") == 0)) {
                    addToPublish(deviceStateTopic.c_str(), "ON", true);
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
        if ((payload != NULL) && !(payload.isEmpty())) {
          printAString("Control MQTT Received...");
          bool skip = false;
          if (isRescanning) {
            if (pScan->isScanning() || isRescanning) {
              if (pScan->isScanning()) {
                pScan->stop();
              }
              allSwitchbotsScanned = {};
              forceRescan = true;
              lastUpdateTimes = {};
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
        if ((payload != NULL) && !(payload.isEmpty())) {
          printAString("Control MQTT Received...");
          bool skip = false;
          if (isRescanning) {
            if (pScan->isScanning() || isRescanning) {
              if (pScan->isScanning()) {
                pScan->stop();
              }
              allSwitchbotsScanned = {};
              forceRescan = true;
              lastUpdateTimes = {};
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
        }
      });



      if (!isMeshNode && meshContactSensors && enableMesh) {
        client.subscribe((contactTopic + aDevice + "/buttoncount").c_str(), [aDevice] (const String & payload)  {

          printAString("START contactTopic + aDevice + buttoncount");
          if ((payload != NULL) && !(payload.isEmpty())) {
            printAString("MQTT meshButtonCount received...");
            std::string anAddr;
            std::map<std::string, std::string>::iterator itY = allSwitchbots.find(aDevice);
            if (itY != allSwitchbots.end())
            {
              anAddr = itY->second;
              bool isNum = is_number(payload.c_str());
              if (isNum) {
                int buttonCount;
                sscanf(payload.c_str(), "%d", &buttonCount);
                if (buttonCount != 0) {
                  std::map<std::string, int>::iterator itE = buttonCounts.find(anAddr);
                  if (itE != buttonCounts.end())
                  {
                    int bCount = itE->second;

                    if ((bCount < buttonCount) ||  ((bCount > 10) && (buttonCount < 5))) {
                      buttonCounts[anAddr] = buttonCount;
                      std::string deviceButtonTopic = contactTopic + aDevice + "/button";
                      //addToPublish(deviceButtonTopic.c_str(), "PUSHED", false);
                      client.publish(deviceButtonTopic.c_str(), "PUSHED", false);
                      lastButton[anAddr] = millis();
                    }
                  }
                }
              }
            }
          }
          printAString("END contactTopic + aDevice + buttoncount");
        });

        client.subscribe((contactTopic + aDevice + "/outcount").c_str(), [aDevice] (const String & payload)  {
          printAString("START contactTopic + aDevice + outcount");
          if ((payload != NULL) && !(payload.isEmpty())) {
            printAString("MQTT meshOutCount received...");
            std::string anAddr;
            std::map<std::string, std::string>::iterator itY = allSwitchbots.find(aDevice);
            if (itY != allSwitchbots.end())
            {
              anAddr = itY->second;
              bool isNum = is_number(payload.c_str());
              if (isNum) {
                int outCount;
                sscanf(payload.c_str(), "%d", &outCount);
                if (outCount != 0) {
                  std::map<std::string, int>::iterator itE = outCounts.find(anAddr);
                  if (itE != outCounts.end())
                  {
                    int bCount = itE->second;

                    if ((bCount < outCount) ||  ((bCount == 3) && (outCount == 1))) {
                      outCounts[anAddr] = outCount;
                      std::string deviceOutTopic = contactTopic + aDevice + "/out";
                      //addToPublish(deviceOutTopic.c_str(), "EXITED", false);
                      client.publish(deviceOutTopic.c_str(), "EXITED", false);
                      lastOut[anAddr] = millis();
                    }
                  }
                }
              }
            }
          }
          printAString("END contactTopic + aDevice + outcount");
        });

        client.subscribe((contactTopic + aDevice + "/incount").c_str(), [aDevice] (const String & payload)  {
          printAString("START contactTopic + aDevice + incount");
          if ((payload != NULL) && !(payload.isEmpty())) {
            printAString("MQTT meshInCount received...");
            std::string anAddr;
            std::map<std::string, std::string>::iterator itY = allSwitchbots.find(aDevice);
            if (itY != allSwitchbots.end())
            {
              anAddr = itY->second;
              bool isNum = is_number(payload.c_str());
              if (isNum) {
                int inCount;
                sscanf(payload.c_str(), "%d", &inCount);
                if (inCount != 0) {
                  std::map<std::string, int>::iterator itE = entranceCounts.find(anAddr);
                  if (itE != entranceCounts.end())
                  {
                    int bCount = itE->second;
                    if ((bCount < inCount) ||  ((bCount == 3) && (inCount == 1))) {
                      entranceCounts[anAddr] = inCount;
                      std::string deviceInTopic = contactTopic + aDevice + "/in";
                      //addToPublish(deviceInTopic.c_str(), "ENTERED", false);
                      client.publish(deviceInTopic.c_str(), "ENTERED", false);
                      lastIn[anAddr] = millis();
                    }
                  }
                }
              }
            }
          }
          printAString("END contactTopic + aDevice + outcount");
        });

      }

      if (meshContactSensors && enableMesh) {
        if (countMotionToAvoidDuplicates) {
          client.subscribe((contactTopic + aDevice + "/motioncount").c_str(), [aDevice] (const String & payload)  {
            printAString("START contactTopic + aDevice + motioncount");
            if ((payload != NULL) && !(payload.isEmpty())) {
              printAString("MQTT meshMotionCount received...");
              std::string anAddr;
              std::map<std::string, std::string>::iterator itY = allSwitchbots.find(aDevice);
              if (itY != allSwitchbots.end())
              {
                anAddr = itY->second;
                bool isNum = is_number(payload.c_str());
                if (isNum) {
                  int newMeshMotionCount;
                  sscanf(payload.c_str(), "%d", &newMeshMotionCount);
                  if (newMeshMotionCount != 0) {
                    std::map<std::string, int>::iterator itWE = motionCounts.find(anAddr.c_str());
                    int motionCount = 0;
                    if (itWE != motionCounts.end())
                    {
                      motionCount = itWE->second;
                    }

                    itWE = meshMotionCounts.find(anAddr.c_str());
                    int meshMotionCount = 0;
                    if (itWE != meshMotionCounts.end())
                    {
                      meshMotionCount = itWE->second;
                    }

                    itWE = noMotionCounts.find(anAddr.c_str());
                    int noMotionCount = 0;
                    if (itWE != noMotionCounts.end())
                    {
                      noMotionCount = itWE->second;
                    }

                    itWE = meshNoMotionCounts.find(anAddr.c_str());
                    int meshNoMotionCount = 0;
                    if (itWE != meshNoMotionCounts.end())
                    {
                      meshNoMotionCount = itWE->second;
                    }

                    if ((motionCount == 0) && (meshMotionCount == 0)) {
                      meshMotionCounts[anAddr.c_str()] = newMeshMotionCount;
                      motionCounts[anAddr.c_str()] = newMeshMotionCount;
                    }
                    else if (meshMotionCount != 0)
                    {
                      if ((meshMotionCount < newMeshMotionCount) ||  ((meshMotionCount > 40) && (newMeshMotionCount < 10))) {
                        motionMeshStates[anAddr.c_str()] = "MOTION";
                        meshMotionCounts[anAddr.c_str()] = newMeshMotionCount;
                        if (!isMeshNode && onlyAllowRootESPToPublishMotion) {
                          std::string deviceMotionTopic = contactTopic + aDevice + "/motion";
                          addToPublish(deviceMotionTopic.c_str(), "MOTION", true);
                        }
                        if (motionCount != newMeshMotionCount) {
                          updateMotionCount[anAddr.c_str()] = millis();
                          updateMeshMotionCount[anAddr.c_str()] = newMeshMotionCount;
                        }
                      }
                      else if ((meshMotionCount == newMeshMotionCount) && (noMotionCount == meshNoMotionCount)) {
                        std::map<std::string, std::string>::iterator itH = motionMeshStates.find(anAddr.c_str());
                        if (itH != motionMeshStates.end())
                        {
                          std::string motionState = itH->second.c_str();
                          if (strcmp(motionState.c_str(), "MOTION") == 0) {
                            if (!isMeshNode && onlyAllowRootESPToPublishMotion) {
                              std::string deviceMotionTopic = contactTopic + aDevice + "/motion";
                              addToPublish(deviceMotionTopic.c_str(), "MOTION", true);
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
            printAString("END contactTopic + aDevice + motioncount");
          });

          client.subscribe((contactTopic + aDevice + "/nomotioncount").c_str(), [aDevice] (const String & payload)  {
            printAString("START contactTopic + aDevice + nomotioncount");
            if ((payload != NULL) && !(payload.isEmpty())) {
              printAString("MQTT meshNoMotionCount received...");
              std::string anAddr;
              std::map<std::string, std::string>::iterator itY = allSwitchbots.find(aDevice);
              if (itY != allSwitchbots.end())
              {
                anAddr = itY->second;
                bool isNum = is_number(payload.c_str());
                if (isNum) {
                  int newMeshNoMotionCount;
                  sscanf(payload.c_str(), "%d", &newMeshNoMotionCount);
                  if (newMeshNoMotionCount != 0) {
                    std::map<std::string, int>::iterator itWE = noMotionCounts.find(anAddr.c_str());
                    int noMotionCount = 0;
                    if (itWE != noMotionCounts.end())
                    {
                      noMotionCount = itWE->second;
                    }

                    itWE = meshNoMotionCounts.find(anAddr.c_str());
                    int meshNoMotionCount = 0;
                    if (itWE != meshNoMotionCounts.end())
                    {
                      meshNoMotionCount = itWE->second;
                    }

                    itWE = motionCounts.find(anAddr.c_str());
                    int motionCount = 0;
                    if (itWE != motionCounts.end())
                    {
                      motionCount = itWE->second;
                    }

                    itWE = meshMotionCounts.find(anAddr.c_str());
                    int meshMotionCount = 0;
                    if (itWE != meshMotionCounts.end())
                    {
                      meshMotionCount = itWE->second;
                    }

                    if ((noMotionCount == 0) && (meshNoMotionCount == 0)) {
                      meshNoMotionCounts[anAddr.c_str()] = newMeshNoMotionCount;
                      noMotionCounts[anAddr.c_str()] = newMeshNoMotionCount;
                    }
                    else if (meshNoMotionCount != 0)
                    {
                      if ((meshNoMotionCount < newMeshNoMotionCount) ||  ((meshNoMotionCount > 40) && (newMeshNoMotionCount < 10))) {
                        motionMeshStates[anAddr.c_str()] = "NO MOTION";
                        meshNoMotionCounts[anAddr.c_str()] = newMeshNoMotionCount;
                        if (!isMeshNode && onlyAllowRootESPToPublishMotion) {
                          std::string deviceMotionTopic = contactTopic + aDevice + "/motion";
                          addToPublish(deviceMotionTopic.c_str(), "NO MOTION", true);
                        }
                        if (noMotionCount != newMeshNoMotionCount) {
                          updateNoMotionCount[anAddr.c_str()] = millis();
                          updateMeshNoMotionCount[anAddr.c_str()] = newMeshNoMotionCount;
                        }
                      }
                      else if ((meshNoMotionCount == newMeshNoMotionCount) && (motionCount == meshMotionCount)) {
                        std::map<std::string, std::string>::iterator itH = motionMeshStates.find(anAddr.c_str());
                        if (itH != motionMeshStates.end())
                        {
                          std::string motionState = itH->second.c_str();
                          if (strcmp(motionState.c_str(), "NO MOTION") == 0) {
                            if (!isMeshNode && onlyAllowRootESPToPublishMotion) {
                              std::string deviceMotionTopic = contactTopic + aDevice + "/motion";
                              addToPublish(deviceMotionTopic.c_str(), "NO MOTION", true);
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
            printAString("END contactTopic + aDevice + nomotioncount");
          });

        }

        if (countContactToAvoidDuplicates) {
          client.subscribe((contactTopic + aDevice + "/closedcount").c_str(), [aDevice] (const String & payload)  {
            printAString("START contactTopic + aDevice + closedcount");
            if ((payload != NULL) && !(payload.isEmpty())) {
              printAString("MQTT meshClosedCount received...");
              std::string anAddr;
              std::map<std::string, std::string>::iterator itY = allSwitchbots.find(aDevice);
              if (itY != allSwitchbots.end())
              {
                anAddr = itY->second;
                bool isNum = is_number(payload.c_str());
                if (isNum) {
                  int newMeshClosedCount;
                  sscanf(payload.c_str(), "%d", &newMeshClosedCount);
                  if (newMeshClosedCount != 0) {
                    std::map<std::string, int>::iterator itWE = closedCounts.find(anAddr.c_str());
                    int closedCount = 0;
                    if (itWE != closedCounts.end())
                    {
                      closedCount = itWE->second;
                    }

                    itWE = meshClosedCounts.find(anAddr.c_str());
                    int meshClosedCount = 0;
                    if (itWE != meshClosedCounts.end())
                    {
                      meshClosedCount = itWE->second;
                    }

                    itWE = openCounts.find(anAddr.c_str());
                    int openCount = 0;
                    if (itWE != openCounts.end())
                    {
                      openCount = itWE->second;
                    }

                    itWE = meshOpenCounts.find(anAddr.c_str());
                    int meshOpenCount = 0;
                    if (itWE != meshOpenCounts.end())
                    {
                      meshOpenCount = itWE->second;
                    }

                    itWE = timeoutCounts.find(anAddr.c_str());
                    int timeoutCount = 0;
                    if (itWE != timeoutCounts.end())
                    {
                      timeoutCount = itWE->second;
                    }

                    itWE = meshTimeoutCounts.find(anAddr.c_str());
                    int meshTimeoutCount = 0;
                    if (itWE != meshTimeoutCounts.end())
                    {
                      meshTimeoutCount = itWE->second;
                    }

                    if ((closedCount == 0) && (meshClosedCount == 0)) {
                      meshClosedCounts[anAddr.c_str()] = newMeshClosedCount;
                      closedCounts[anAddr.c_str()] = newMeshClosedCount;
                    }
                    else if (meshClosedCount != 0)
                    {
                      if ((meshClosedCount < newMeshClosedCount) ||  ((meshClosedCount > 40) && (newMeshClosedCount < 10))) {
                        contactMeshStates[anAddr.c_str()] = "CLOSED";
                        meshClosedCounts[anAddr.c_str()] = newMeshClosedCount;
                        if (!isMeshNode && onlyAllowRootESPToPublishContact) {
                          std::string deviceContactTopic = contactTopic + aDevice + "/contact";
                          std::string deviceStateTopic = contactTopic + aDevice + "/state";
                          std::string deviceBinContactTopic = contactTopic + aDevice + "/bin";
                          addToPublish(deviceContactTopic.c_str(), "CLOSED", true);
                          addToPublish(deviceStateTopic.c_str(), "CLOSED", true);
                          addToPublish(deviceBinContactTopic.c_str(), "CLOSED", true);
                        }
                        if (closedCount != newMeshClosedCount) {
                          updateClosedCount[anAddr.c_str()] = millis();
                          updateMeshClosedCount[anAddr.c_str()] = newMeshClosedCount;
                        }
                      }
                      else if ((meshClosedCount == newMeshClosedCount) && (openCount == meshOpenCount) && (timeoutCount == meshTimeoutCount)) {
                        std::map<std::string, std::string>::iterator itH = contactMeshStates.find(anAddr.c_str());
                        if (itH != contactMeshStates.end())
                        {
                          std::string contactState = itH->second.c_str();
                          if (strcmp(contactState.c_str(), "CLOSED") == 0) {
                            if (!isMeshNode && onlyAllowRootESPToPublishContact) {
                              std::string deviceContactTopic = contactTopic + aDevice + "/contact";
                              std::string deviceStateTopic = contactTopic + aDevice + "/state";
                              std::string deviceBinContactTopic = contactTopic + aDevice + "/bin";
                              addToPublish(deviceContactTopic.c_str(), "CLOSED", true);
                              addToPublish(deviceStateTopic.c_str(), "CLOSED", true);
                              addToPublish(deviceBinContactTopic.c_str(), "CLOSED", true);
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
            printAString("END contactTopic + aDevice + closedcount");
          });

          client.subscribe((contactTopic + aDevice + "/opencount").c_str(), [aDevice] (const String & payload)  {
            printAString("START contactTopic + aDevice + opencount");
            if ((payload != NULL) && !(payload.isEmpty())) {
              printAString("MQTT meshOpenCount received...");
              std::string anAddr;
              std::map<std::string, std::string>::iterator itY = allSwitchbots.find(aDevice);
              if (itY != allSwitchbots.end())
              {
                anAddr = itY->second;
                bool isNum = is_number(payload.c_str());
                if (isNum) {
                  int newMeshOpenCount;
                  sscanf(payload.c_str(), "%d", &newMeshOpenCount);
                  if (newMeshOpenCount != 0) {
                    std::map<std::string, int>::iterator itWE = closedCounts.find(anAddr.c_str());
                    int closedCount = 0;
                    if (itWE != closedCounts.end())
                    {
                      closedCount = itWE->second;
                    }

                    itWE = meshClosedCounts.find(anAddr.c_str());
                    int meshClosedCount = 0;
                    if (itWE != meshClosedCounts.end())
                    {
                      meshClosedCount = itWE->second;
                    }

                    itWE = openCounts.find(anAddr.c_str());
                    int openCount = 0;
                    if (itWE != openCounts.end())
                    {
                      openCount = itWE->second;
                    }

                    itWE = meshOpenCounts.find(anAddr.c_str());
                    int meshOpenCount = 0;
                    if (itWE != meshOpenCounts.end())
                    {
                      meshOpenCount = itWE->second;
                    }

                    itWE = timeoutCounts.find(anAddr.c_str());
                    int timeoutCount = 0;
                    if (itWE != timeoutCounts.end())
                    {
                      timeoutCount = itWE->second;
                    }

                    itWE = meshTimeoutCounts.find(anAddr.c_str());
                    int meshTimeoutCount = 0;
                    if (itWE != meshTimeoutCounts.end())
                    {
                      meshTimeoutCount = itWE->second;
                    }

                    if ((openCount == 0) && (meshOpenCount == 0)) {
                      meshOpenCounts[anAddr.c_str()] = newMeshOpenCount;
                      openCounts[anAddr.c_str()] = newMeshOpenCount;
                    }
                    else if (meshOpenCount != 0)
                    {
                      if ((meshOpenCount < newMeshOpenCount) || ((meshOpenCount > 40) && (newMeshOpenCount < 10)))  {
                        contactMeshStates[anAddr.c_str()] = "OPEN";
                        meshOpenCounts[anAddr.c_str()] = newMeshOpenCount;
                        lastContacts[aDevice.c_str()] = millis();
                        if (!isMeshNode && onlyAllowRootESPToPublishContact) {
                          // addToPublish("esp32mesh1/" + aDevice + "/contact", "OPENFROMMESH+", true);
                          std::string deviceContactTopic = contactTopic + aDevice + "/contact";
                          std::string deviceStateTopic = contactTopic + aDevice + "/state";
                          std::string deviceBinContactTopic = contactTopic + aDevice + "/bin";
                          addToPublish(deviceContactTopic.c_str(), "OPEN", true);
                          addToPublish(deviceStateTopic.c_str(), "OPEN", true);
                          addToPublish(deviceBinContactTopic.c_str(), "OPEN", true);
                          addToPublish((contactTopic + aDevice + "/lastcontact"), 0, true);
                        }

                        if (openCount != newMeshOpenCount) {
                          updateOpenCount[anAddr.c_str()] = millis();
                          updateMeshOpenCount[anAddr.c_str()] = newMeshOpenCount;
                        }
                      }
                      else if ((meshOpenCount == newMeshOpenCount) && (closedCount == meshClosedCount) && (timeoutCount == meshTimeoutCount)) {
                        std::map<std::string, std::string>::iterator itH = contactMeshStates.find(anAddr.c_str());
                        if (itH != contactMeshStates.end())
                        {
                          std::string contactState = itH->second.c_str();
                          if (strcmp(contactState.c_str(), "OPEN") == 0) {
                            lastContacts[aDevice.c_str()] = millis();
                            if (!isMeshNode && onlyAllowRootESPToPublishContact) {
                              std::string deviceContactTopic = contactTopic + aDevice + "/contact";
                              std::string deviceStateTopic = contactTopic + aDevice + "/state";
                              std::string deviceBinContactTopic = contactTopic + aDevice + "/bin";
                              addToPublish(deviceContactTopic.c_str(), "OPEN", true);
                              addToPublish(deviceStateTopic.c_str(), "OPEN", true);
                              addToPublish(deviceBinContactTopic.c_str(), "OPEN", true);
                              addToPublish((contactTopic + aDevice + "/lastcontact"), 0, true);
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
            printAString("END contactTopic + aDevice + closedcount");
          });

          client.subscribe((contactTopic + aDevice + "/timeoutcount").c_str(), [aDevice] (const String & payload)  {
            printAString("START contactTopic + aDevice + timeoutcount");
            if ((payload != NULL) && !(payload.isEmpty())) {
              printAString("MQTT meshTimeoutCount received...");
              std::string anAddr;
              std::map<std::string, std::string>::iterator itY = allSwitchbots.find(aDevice);
              if (itY != allSwitchbots.end())
              {
                anAddr = itY->second;
                bool isNum = is_number(payload.c_str());
                if (isNum) {
                  int newMeshTimeoutCount;
                  sscanf(payload.c_str(), "%d", &newMeshTimeoutCount);
                  if (newMeshTimeoutCount != 0) {
                    std::map<std::string, int>::iterator itWE = closedCounts.find(anAddr.c_str());
                    int closedCount = 0;
                    if (itWE != closedCounts.end())
                    {
                      closedCount = itWE->second;
                    }

                    itWE = meshClosedCounts.find(anAddr.c_str());
                    int meshClosedCount = 0;
                    if (itWE != meshClosedCounts.end())
                    {
                      meshClosedCount = itWE->second;
                    }

                    itWE = openCounts.find(anAddr.c_str());
                    int openCount = 0;
                    if (itWE != openCounts.end())
                    {
                      openCount = itWE->second;
                    }

                    itWE = meshOpenCounts.find(anAddr.c_str());
                    int meshOpenCount = 0;
                    if (itWE != meshOpenCounts.end())
                    {
                      meshOpenCount = itWE->second;
                    }

                    itWE = timeoutCounts.find(anAddr.c_str());
                    int timeoutCount = 0;
                    if (itWE != timeoutCounts.end())
                    {
                      timeoutCount = itWE->second;
                    }

                    itWE = meshTimeoutCounts.find(anAddr.c_str());
                    int meshTimeoutCount = 0;
                    if (itWE != meshTimeoutCounts.end())
                    {
                      meshTimeoutCount = itWE->second;
                    }

                    if ((timeoutCount == 0) && (meshTimeoutCount == 0)) {
                      meshTimeoutCounts[anAddr.c_str()] = newMeshTimeoutCount;
                      timeoutCounts[anAddr.c_str()] = newMeshTimeoutCount;
                    }
                    else if (meshTimeoutCount != 0)
                    {
                      if ((meshTimeoutCount < newMeshTimeoutCount) ||  ((meshTimeoutCount > 40) && (newMeshTimeoutCount < 10))) {
                        contactMeshStates[anAddr.c_str()] = "TIMEOUT";
                        meshTimeoutCounts[anAddr.c_str()] = newMeshTimeoutCount;
                        lastContacts[aDevice.c_str()] = millis();
                        if (!isMeshNode && onlyAllowRootESPToPublishContact) {
                          std::string deviceContactTopic = contactTopic + aDevice + "/contact";
                          std::string deviceStateTopic = contactTopic + aDevice + "/state";
                          std::string deviceBinContactTopic = contactTopic + aDevice + "/bin";
                          addToPublish(deviceContactTopic.c_str(), "TIMEOUT", true);
                          addToPublish(deviceStateTopic.c_str(), "TIMEOUT", true);
                          addToPublish(deviceBinContactTopic.c_str(), "OPEN", true);
                        }
                        if (timeoutCount != newMeshTimeoutCount) {
                          updateTimeoutCount[anAddr.c_str()] = millis();
                          updateMeshTimeoutCount[anAddr.c_str()] = newMeshTimeoutCount;
                        }
                      }
                      else if ((meshTimeoutCount == newMeshTimeoutCount) && (openCount == meshOpenCount) && (closedCount == meshClosedCount)) {
                        std::map<std::string, std::string>::iterator itH = contactMeshStates.find(anAddr.c_str());
                        if (itH != contactMeshStates.end())
                        {
                          std::string contactState = itH->second.c_str();
                          if (strcmp(contactState.c_str(), "TIMEOUT") == 0) {
                            lastContacts[aDevice.c_str()] = millis();
                            if (!isMeshNode && onlyAllowRootESPToPublishContact) {
                              std::string deviceContactTopic = contactTopic + aDevice + "/contact";
                              std::string deviceStateTopic = contactTopic + aDevice + "/state";
                              std::string deviceBinContactTopic = contactTopic + aDevice + "/bin";
                              addToPublish(deviceContactTopic.c_str(), "TIMEOUT", true);
                              addToPublish(deviceStateTopic.c_str(), "TIMEOUT", true);
                              addToPublish(deviceBinContactTopic.c_str(), "OPEN", true);
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
            printAString("END contactTopic + aDevice + timeoutcount");
          });
        }

        if (countLightToAvoidDuplicates) {
          client.subscribe((contactTopic + aDevice + "/darkcount").c_str(), [aDevice] (const String & payload)  {
            printAString("START contactTopic + aDevice + darkcount");
            if ((payload != NULL) && !(payload.isEmpty())) {
              printAString("MQTT meshDarkCount received...");
              std::string anAddr;
              std::map<std::string, std::string>::iterator itY = allSwitchbots.find(aDevice);
              if (itY != allSwitchbots.end())
              {
                anAddr = itY->second;
                bool isNum = is_number(payload.c_str());
                if (isNum) {
                  int newMeshDarkCount;
                  sscanf(payload.c_str(), "%d", &newMeshDarkCount);
                  if (newMeshDarkCount != 0) {
                    std::map<std::string, int>::iterator itWE = darkCounts.find(anAddr.c_str());
                    int darkCount = 0;
                    if (itWE != darkCounts.end())
                    {
                      darkCount = itWE->second;
                    }

                    itWE = meshDarkCounts.find(anAddr.c_str());
                    int meshDarkCount = 0;
                    if (itWE != meshDarkCounts.end())
                    {
                      meshDarkCount = itWE->second;
                    }

                    itWE = brightCounts.find(anAddr.c_str());
                    int brightCount = 0;
                    if (itWE != brightCounts.end())
                    {
                      brightCount = itWE->second;
                    }

                    itWE = meshBrightCounts.find(anAddr.c_str());
                    int meshBrightCount = 0;
                    if (itWE != meshBrightCounts.end())
                    {
                      meshBrightCount = itWE->second;
                    }

                    if ((darkCount == 0) && (meshDarkCount == 0)) {
                      meshDarkCounts[anAddr.c_str()] = newMeshDarkCount;
                      darkCounts[anAddr.c_str()] = newMeshDarkCount;
                    }
                    else if (meshDarkCount != 0)
                    {
                      if ((meshDarkCount < newMeshDarkCount) ||  ((meshDarkCount > 40) && (newMeshDarkCount < 10))) {
                        lightMeshStates[anAddr.c_str()] = "DARK";
                        meshDarkCounts[anAddr.c_str()] = newMeshDarkCount;
                        if (!isMeshNode && onlyAllowRootESPToPublishLight) {
                          std::string deviceLightTopic = contactTopic + aDevice + "/illuminance";
                          addToPublish(deviceLightTopic.c_str(), "DARK", true);
                        }
                        if (darkCount != newMeshDarkCount) {
                          updateDarkCount[anAddr.c_str()] = millis();
                          updateMeshDarkCount[anAddr.c_str()] = newMeshDarkCount;
                        }
                      }
                      else if ((meshDarkCount == newMeshDarkCount) && (brightCount == meshBrightCount)) {
                        std::map<std::string, std::string>::iterator itH = lightMeshStates.find(anAddr.c_str());
                        if (itH != lightMeshStates.end())
                        {
                          std::string illuminanceState = itH->second.c_str();
                          if (strcmp(illuminanceState.c_str(), "DARK") == 0) {
                            if (!isMeshNode && onlyAllowRootESPToPublishLight) {
                              std::string deviceLightTopic = contactTopic + aDevice + "/illuminance";
                              addToPublish(deviceLightTopic.c_str(), "DARK", true);
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
            printAString("END contactTopic + aDevice + darkcount");
          });

          client.subscribe((contactTopic + aDevice + "/brightcount").c_str(), [aDevice] (const String & payload)  {
            printAString("START contactTopic + aDevice + brightcount");
            if ((payload != NULL) && !(payload.isEmpty())) {
              printAString("MQTT meshBrightCount received...");
              std::string anAddr;
              std::map<std::string, std::string>::iterator itY = allSwitchbots.find(aDevice);
              if (itY != allSwitchbots.end())
              {
                anAddr = itY->second;
                bool isNum = is_number(payload.c_str());
                if (isNum) {
                  int newMeshBrightCount;
                  sscanf(payload.c_str(), "%d", &newMeshBrightCount);
                  if (newMeshBrightCount != 0) {
                    std::map<std::string, int>::iterator itWE = brightCounts.find(anAddr.c_str());
                    int brightCount = 0;
                    if (itWE != brightCounts.end())
                    {
                      brightCount = itWE->second;
                    }

                    itWE = meshBrightCounts.find(anAddr.c_str());
                    int meshBrightCount = 0;
                    if (itWE != meshBrightCounts.end())
                    {
                      meshBrightCount = itWE->second;
                    }

                    itWE = darkCounts.find(anAddr.c_str());
                    int darkCount = 0;
                    if (itWE != darkCounts.end())
                    {
                      darkCount = itWE->second;
                    }

                    itWE = meshDarkCounts.find(anAddr.c_str());
                    int meshDarkCount = 0;
                    if (itWE != meshDarkCounts.end())
                    {
                      meshDarkCount = itWE->second;
                    }

                    if ((brightCount == 0) && (meshBrightCount == 0)) {
                      meshBrightCounts[anAddr.c_str()] = newMeshBrightCount;
                      brightCounts[anAddr.c_str()] = newMeshBrightCount;
                    }
                    else if (meshBrightCount != 0)
                    {
                      if ((meshBrightCount < newMeshBrightCount) ||  ((meshBrightCount > 40) && (newMeshBrightCount < 10))) {
                        lightMeshStates[anAddr.c_str()] = "BRIGHT";
                        meshBrightCounts[anAddr.c_str()] = newMeshBrightCount;
                        if (!isMeshNode && onlyAllowRootESPToPublishLight) {
                          std::string deviceLightTopic = contactTopic + aDevice + "/illuminance";
                          addToPublish(deviceLightTopic.c_str(), "BRIGHT", true);
                        }
                        if (brightCount != newMeshBrightCount) {
                          updateBrightCount[anAddr.c_str()] = millis();
                          updateMeshBrightCount[anAddr.c_str()] = newMeshBrightCount;
                        }
                      }
                      else if ((meshBrightCount == newMeshBrightCount) && (darkCount == meshDarkCount)) {
                        std::map<std::string, std::string>::iterator itH = lightMeshStates.find(anAddr.c_str());
                        if (itH != lightMeshStates.end())
                        {
                          std::string illuminanceState = itH->second.c_str();
                          if (strcmp(illuminanceState.c_str(), "BRIGHT") == 0) {
                            if (!isMeshNode && onlyAllowRootESPToPublishLight) {
                              std::string deviceLightTopic = contactTopic + aDevice + "/illuminance";
                              addToPublish(deviceLightTopic.c_str(), "BRIGHT", true);
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
            printAString("END contactTopic + aDevice + brightcount");
          });
        }
      }

      it++;
    }

    it = allMotionSensors.begin();
    while (it != allMotionSensors.end())
    {
      std::string deviceStr ;
      aDevice = it->first.c_str();
      client.subscribe((motionTopic + aDevice + "/set").c_str(), [aDevice] (const String & payload)  {
        if ((payload != NULL) && !(payload.isEmpty())) {
          printAString("Control MQTT Received...");
          bool skip = false;
          if (isRescanning) {
            if (pScan->isScanning() || isRescanning) {
              if (pScan->isScanning()) {
                pScan->stop();
              }
              allSwitchbotsScanned = {};
              forceRescan = true;
              lastUpdateTimes = {};
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
        }
      });

      if (meshMotionSensors && enableMesh) {
        if (countMotionToAvoidDuplicates) {
          client.subscribe((motionTopic + aDevice + "/motioncount").c_str(), [aDevice] (const String & payload)  {
            printAString("START motionTopic + aDevice + motioncount");
            if ((payload != NULL) && !(payload.isEmpty())) {
              printAString("MQTT meshMotionCount received...");
              std::string anAddr;
              std::map<std::string, std::string>::iterator itY = allSwitchbots.find(aDevice);
              if (itY != allSwitchbots.end())
              {
                anAddr = itY->second;
                bool isNum = is_number(payload.c_str());
                if (isNum) {
                  int newMeshMotionCount;
                  sscanf(payload.c_str(), "%d", &newMeshMotionCount);
                  if (newMeshMotionCount != 0) {
                    std::map<std::string, int>::iterator itWE = motionCounts.find(anAddr.c_str());
                    int motionCount = 0;
                    if (itWE != motionCounts.end())
                    {
                      motionCount = itWE->second;
                    }

                    itWE = meshMotionCounts.find(anAddr.c_str());
                    int meshMotionCount = 0;
                    if (itWE != meshMotionCounts.end())
                    {
                      meshMotionCount = itWE->second;
                    }

                    itWE = noMotionCounts.find(anAddr.c_str());
                    int noMotionCount = 0;
                    if (itWE != noMotionCounts.end())
                    {
                      noMotionCount = itWE->second;
                    }

                    itWE = meshNoMotionCounts.find(anAddr.c_str());
                    int meshNoMotionCount = 0;
                    if (itWE != meshNoMotionCounts.end())
                    {
                      meshNoMotionCount = itWE->second;
                    }

                    if ((motionCount == 0) && (meshMotionCount == 0)) {
                      meshMotionCounts[anAddr.c_str()] = newMeshMotionCount;
                      motionCounts[anAddr.c_str()] = newMeshMotionCount;
                    }
                    else if (meshMotionCount != 0)
                    {
                      if ((meshMotionCount < newMeshMotionCount) ||  ((meshMotionCount > 40) && (newMeshMotionCount < 10))) {
                        motionMeshStates[anAddr.c_str()] = "MOTION";
                        meshMotionCounts[anAddr.c_str()] = newMeshMotionCount;
                        if (!isMeshNode && onlyAllowRootESPToPublishMotion) {
                          std::string deviceMotionTopic = motionTopic + aDevice + "/motion";
                          addToPublish(deviceMotionTopic.c_str(), "MOTION", true);
                          std::string deviceStateTopic = motionTopic + aDevice + "/state";
                          addToPublish(deviceStateTopic.c_str(), "MOTION", true);
                        }
                        if (motionCount != newMeshMotionCount) {
                          updateMotionCount[anAddr.c_str()] = millis();
                          updateMeshMotionCount[anAddr.c_str()] = newMeshMotionCount;
                        }
                      }
                      else if ((meshMotionCount == newMeshMotionCount) && (noMotionCount == meshNoMotionCount)) {
                        std::map<std::string, std::string>::iterator itH = motionMeshStates.find(anAddr.c_str());
                        if (itH != motionMeshStates.end())
                        {
                          std::string motionState = itH->second.c_str();
                          if (strcmp(motionState.c_str(), "MOTION") == 0) {
                            if (!isMeshNode && onlyAllowRootESPToPublishMotion) {
                              std::string deviceMotionTopic = motionTopic + aDevice + "/motion";
                              addToPublish(deviceMotionTopic.c_str(), "MOTION", true);
                              std::string deviceStateTopic = motionTopic + aDevice + "/state";
                              addToPublish(deviceStateTopic.c_str(), "MOTION", true);
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
            printAString("END motionTopic + aDevice + motioncount");
          });

          client.subscribe((motionTopic + aDevice + "/nomotioncount").c_str(), [aDevice] (const String & payload)  {
            printAString("START motionTopic + aDevice + nomotioncount");
            if ((payload != NULL) && !(payload.isEmpty())) {
              printAString("MQTT meshNoMotionCount received...");
              std::string anAddr;
              std::map<std::string, std::string>::iterator itY = allSwitchbots.find(aDevice);
              if (itY != allSwitchbots.end())
              {
                anAddr = itY->second;
                bool isNum = is_number(payload.c_str());
                if (isNum) {
                  int newMeshNoMotionCount;
                  sscanf(payload.c_str(), "%d", &newMeshNoMotionCount);
                  if (newMeshNoMotionCount != 0) {
                    std::map<std::string, int>::iterator itWE = noMotionCounts.find(anAddr.c_str());
                    int noMotionCount = 0;
                    if (itWE != noMotionCounts.end())
                    {
                      noMotionCount = itWE->second;
                    }

                    itWE = meshNoMotionCounts.find(anAddr.c_str());
                    int meshNoMotionCount = 0;
                    if (itWE != meshNoMotionCounts.end())
                    {
                      meshNoMotionCount = itWE->second;
                    }

                    itWE = motionCounts.find(anAddr.c_str());
                    int motionCount = 0;
                    if (itWE != motionCounts.end())
                    {
                      motionCount = itWE->second;
                    }

                    itWE = meshMotionCounts.find(anAddr.c_str());
                    int meshMotionCount = 0;
                    if (itWE != meshMotionCounts.end())
                    {
                      meshMotionCount = itWE->second;
                    }

                    if ((noMotionCount == 0) && (meshNoMotionCount == 0)) {
                      meshNoMotionCounts[anAddr.c_str()] = newMeshNoMotionCount;
                      noMotionCounts[anAddr.c_str()] = newMeshNoMotionCount;
                    }
                    else if (meshNoMotionCount != 0)
                    {
                      if ((meshNoMotionCount < newMeshNoMotionCount) ||  ((meshNoMotionCount > 40) && (newMeshNoMotionCount < 10))) {
                        motionMeshStates[anAddr.c_str()] = "NO MOTION";
                        meshNoMotionCounts[anAddr.c_str()] = newMeshNoMotionCount;
                        if (!isMeshNode && onlyAllowRootESPToPublishMotion) {
                          std::string deviceMotionTopic = motionTopic + aDevice + "/motion";
                          addToPublish(deviceMotionTopic.c_str(), "NO MOTION", true);
                        }
                        if (noMotionCount != newMeshNoMotionCount) {
                          updateNoMotionCount[anAddr.c_str()] = millis();
                          updateMeshNoMotionCount[anAddr.c_str()] = newMeshNoMotionCount;
                        }
                      }
                      else if ((meshNoMotionCount == newMeshNoMotionCount) && (motionCount == meshMotionCount)) {
                        std::map<std::string, std::string>::iterator itH = motionMeshStates.find(anAddr.c_str());
                        if (itH != motionMeshStates.end())
                        {
                          std::string motionState = itH->second.c_str();
                          if (strcmp(motionState.c_str(), "NO MOTION") == 0) {
                            if (!isMeshNode && onlyAllowRootESPToPublishMotion) {
                              std::string deviceMotionTopic = motionTopic + aDevice + "/motion";
                              addToPublish(deviceMotionTopic.c_str(), "NO MOTION", true);
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
            printAString("END motionTopic + aDevice + nomotioncount");
          });

        }

        if (countLightToAvoidDuplicates) {
          client.subscribe((motionTopic + aDevice + "/darkcount").c_str(), [aDevice] (const String & payload)  {
            printAString("START motionTopic + aDevice + darkcount");
            if ((payload != NULL) && !(payload.isEmpty())) {
              printAString("MQTT meshDarkCount received...");
              std::string anAddr;
              std::map<std::string, std::string>::iterator itY = allSwitchbots.find(aDevice);
              if (itY != allSwitchbots.end())
              {
                anAddr = itY->second;
                bool isNum = is_number(payload.c_str());
                if (isNum) {
                  int newMeshDarkCount;
                  sscanf(payload.c_str(), "%d", &newMeshDarkCount);
                  if (newMeshDarkCount != 0) {
                    std::map<std::string, int>::iterator itWE = darkCounts.find(anAddr.c_str());
                    int darkCount = 0;
                    if (itWE != darkCounts.end())
                    {
                      darkCount = itWE->second;
                    }

                    itWE = meshDarkCounts.find(anAddr.c_str());
                    int meshDarkCount = 0;
                    if (itWE != meshDarkCounts.end())
                    {
                      meshDarkCount = itWE->second;
                    }

                    itWE = brightCounts.find(anAddr.c_str());
                    int brightCount = 0;
                    if (itWE != brightCounts.end())
                    {
                      brightCount = itWE->second;
                    }

                    itWE = meshBrightCounts.find(anAddr.c_str());
                    int meshBrightCount = 0;
                    if (itWE != meshBrightCounts.end())
                    {
                      meshBrightCount = itWE->second;
                    }

                    if ((darkCount == 0) && (meshDarkCount == 0)) {
                      meshDarkCounts[anAddr.c_str()] = newMeshDarkCount;
                      darkCounts[anAddr.c_str()] = newMeshDarkCount;
                    }
                    else if (meshDarkCount != 0)
                    {
                      if ((meshDarkCount < newMeshDarkCount) ||  ((meshDarkCount > 40) && (newMeshDarkCount < 10))) {
                        lightMeshStates[anAddr.c_str()] = "DARK";
                        meshDarkCounts[anAddr.c_str()] = newMeshDarkCount;
                        if (!isMeshNode && onlyAllowRootESPToPublishLight) {
                          std::string deviceLightTopic = motionTopic + aDevice + "/illuminance";
                          addToPublish(deviceLightTopic.c_str(), "DARK", true);
                        }
                        if (darkCount != newMeshDarkCount) {
                          updateDarkCount[anAddr.c_str()] = millis();
                          updateMeshDarkCount[anAddr.c_str()] = newMeshDarkCount;
                        }
                      }
                      else if ((meshDarkCount == newMeshDarkCount) && (brightCount == meshBrightCount)) {
                        std::map<std::string, std::string>::iterator itH = lightMeshStates.find(anAddr.c_str());
                        if (itH != lightMeshStates.end())
                        {
                          std::string illuminanceState = itH->second.c_str();
                          if (strcmp(illuminanceState.c_str(), "DARK") == 0) {
                            if (!isMeshNode && onlyAllowRootESPToPublishLight) {
                              std::string deviceLightTopic = motionTopic + aDevice + "/illuminance";
                              addToPublish(deviceLightTopic.c_str(), "DARK", true);
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
            printAString("END motionTopic + aDevice + darkcount");
          });

          client.subscribe((motionTopic + aDevice + "/brightcount").c_str(), [aDevice] (const String & payload)  {
            printAString("START motionTopic + aDevice + brightcount");
            if ((payload != NULL) && !(payload.isEmpty())) {
              printAString("MQTT meshBrightCount received...");
              std::string anAddr;
              std::map<std::string, std::string>::iterator itY = allSwitchbots.find(aDevice);
              if (itY != allSwitchbots.end())
              {
                anAddr = itY->second;
                bool isNum = is_number(payload.c_str());
                if (isNum) {
                  int newMeshBrightCount;
                  sscanf(payload.c_str(), "%d", &newMeshBrightCount);
                  if (newMeshBrightCount != 0) {
                    std::map<std::string, int>::iterator itWE = brightCounts.find(anAddr.c_str());
                    int brightCount = 0;
                    if (itWE != brightCounts.end())
                    {
                      brightCount = itWE->second;
                    }

                    itWE = meshBrightCounts.find(anAddr.c_str());
                    int meshBrightCount = 0;
                    if (itWE != meshBrightCounts.end())
                    {
                      meshBrightCount = itWE->second;
                    }

                    itWE = darkCounts.find(anAddr.c_str());
                    int darkCount = 0;
                    if (itWE != darkCounts.end())
                    {
                      darkCount = itWE->second;
                    }

                    itWE = meshDarkCounts.find(anAddr.c_str());
                    int meshDarkCount = 0;
                    if (itWE != meshDarkCounts.end())
                    {
                      meshDarkCount = itWE->second;
                    }

                    if ((brightCount == 0) && (meshBrightCount == 0)) {
                      meshBrightCounts[anAddr.c_str()] = newMeshBrightCount;
                      brightCounts[anAddr.c_str()] = newMeshBrightCount;
                    }
                    else if (meshBrightCount != 0)
                    {
                      if ((meshBrightCount < newMeshBrightCount) ||  ((meshBrightCount > 40) && (newMeshBrightCount < 10))) {
                        lightMeshStates[anAddr.c_str()] = "BRIGHT";
                        meshBrightCounts[anAddr.c_str()] = newMeshBrightCount;
                        if (!isMeshNode && onlyAllowRootESPToPublishLight) {
                          std::string deviceLightTopic = motionTopic + aDevice + "/illuminance";
                          addToPublish(deviceLightTopic.c_str(), "BRIGHT", true);
                        }
                        if (brightCount != newMeshBrightCount) {
                          updateBrightCount[anAddr.c_str()] = millis();
                          updateMeshBrightCount[anAddr.c_str()] = newMeshBrightCount;
                        }
                      }
                      else if ((meshBrightCount == newMeshBrightCount) && (darkCount == meshDarkCount)) {
                        std::map<std::string, std::string>::iterator itH = lightMeshStates.find(anAddr.c_str());
                        if (itH != lightMeshStates.end())
                        {
                          std::string illuminanceState = itH->second.c_str();
                          if (strcmp(illuminanceState.c_str(), "BRIGHT") == 0) {
                            if (!isMeshNode && onlyAllowRootESPToPublishLight) {
                              std::string deviceLightTopic = motionTopic + aDevice + "/illuminance";
                              addToPublish(deviceLightTopic.c_str(), "BRIGHT", true);
                            }
                          }
                        }
                      }
                    }
                  }
                }
              }
            }
            printAString("END motionTopic + aDevice + brightcount");
          });
        }
      }
      it++;
    }

    client.subscribe(requestInfoStdStr.c_str(), [] (const String & payload)  {
      if ((payload != NULL) && !(payload.isEmpty())) {
        printAString("Request Info MQTT Received...");
        bool skip = false;
        if (isRescanning) {
          if (pScan->isScanning() || isRescanning) {
            if (pScan->isScanning()) {
              pScan->stop();
            }
            allSwitchbotsScanned = {};
            forceRescan = true;
            lastUpdateTimes = {};
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
      }
    });

    client.subscribe(requestSettingsStdStr.c_str(), [] (const String & payload)  {
      if ((payload != NULL) && !(payload.isEmpty())) {
        printAString("Request Settings MQTT Received...");
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
      }
    });

    client.subscribe(setModeStdStr.c_str(), [] (const String & payload)  {
      if ((payload != NULL) && !(payload.isEmpty())) {
        printAString("setMode  MQTT Received...");
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
      }
    });

    client.subscribe(setHoldStdStr.c_str(), [] (const String & payload)  {
      if ((payload != NULL) && !(payload.isEmpty())) {
        printAString("setHold MQTT Received...");
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
      }
    });

    client.subscribe(holdPressStdStr.c_str(), [] (const String & payload)  {
      if ((payload != NULL) && !(payload.isEmpty())) {
        printAString("holdPress MQTT Received...");
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
      }
    });

    client.subscribe(rescanStdStr.c_str(), [] (const String & payload)  {
      if ((payload != NULL) && !(payload.isEmpty())) {
        printAString("Rescan MQTT Received...");

        bool skip = false;
        if (isRescanning) {
          if (pScan->isScanning() || isRescanning) {
            if (pScan->isScanning()) {
              pScan->stop();
            }
            allSwitchbotsScanned = {};
            forceRescan = true;
            lastUpdateTimes = {};
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
      }
    });

    publishHomeAssistantDiscoveryESPConfig();
    discoveredDevices = {};
  }
}

bool connectToServer(NimBLEAdvertisedDevice * advDeviceToUse) {
  printAString("Try to connect. Try a reconnect first...");
  NimBLEClient* pClient = nullptr;
  if (NimBLEDevice::getClientListSize()) {

    pClient = NimBLEDevice::getClientByPeerAddress(advDeviceToUse->getAddress());
    if (pClient) {
      if (!pClient->connect(advDeviceToUse, false)) {
        printAString("Reconnect failed");
      }
      else {
        printAString("Reconnected client");
      }
    }
    else {
      pClient = NimBLEDevice::getDisconnectedClient();
    }
  }
  if (!pClient) {
    if (NimBLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS) {
      printAString("Max clients reached - no more connections available");
      return false;
    }
    pClient = NimBLEDevice::createClient();
    printAString("New client created");
    pClient->setClientCallbacks(&clientCB, false);
    pClient->setConnectionParams(12, 12, 0, 51);
    pClient->setConnectTimeout(10);

  }
  if (!pClient->isConnected()) {
    if (!pClient->connect(advDeviceToUse)) {
      NimBLEDevice::deleteClient(pClient);
      printAString("Failed to connect, deleted client");
      return false;
    }
  }
  printAString("Connected to: ");
  printAString(pClient->getPeerAddress().toString().c_str());
  printAString("RSSI: ");
  printAString(pClient->getRssi());
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
  return sendCommandBytesWithResponse(pChr, bArray, aSize);
}

bool sendBotCommandBytes(NimBLERemoteCharacteristic * pChr, byte * bArray, int aSize ) {
  if (pChr == nullptr) {
    return false;
  }
  return sendCommandBytesWithResponse(pChr, bArray, aSize);
}

bool sendPlugCommandBytes(NimBLERemoteCharacteristic * pChr, byte * bArray, int aSize ) {
  if (pChr == nullptr) {
    return false;
  }
  return sendCommandBytesWithResponse(pChr, bArray, aSize);
}

bool isBotDevice(std::string aDevice) {
  std::map<std::string, std::string>::iterator itS = allBots.find(aDevice);
  if (itS != allBots.end())
  {
    return true;
  }
  return false;
}

bool isPlugDevice(std::string aDevice) {
  std::map<std::string, std::string>::iterator itS = allPlugs.find(aDevice);
  if (itS != allPlugs.end())
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

bool isMeterDevice(std::string & aDevice) {
  std::map<std::string, std::string>::iterator itS = allMeters.find(aDevice);
  if (itS != allMeters.end())
  {
    return true;
  }
  return false;
}

bool isContactDevice(std::string & aDevice) {
  std::map<std::string, std::string>::iterator itS = allContactSensors.find(aDevice);
  if (itS != allContactSensors.end())
  {
    return true;
  }
  return false;
}

bool isMotionDevice(std::string & aDevice) {
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
  printAString("Sending command...");

  byte bArrayPress[] = {0x57, 0x01};
  byte bArrayOn[] = {0x57, 0x01, 0x01};
  byte bArrayOff[] = {0x57, 0x01, 0x02};
  byte bArrayPlugOn[] = {0x57, 0x0F, 0x50, 0x01, 0x01, 0x80};
  byte bArrayPlugOff[] = {0x57, 0x0F, 0x50, 0x01, 0x01, 0x00};
  byte bArrayOpen[] =  {0x57, 0x0F, 0x45, 0x01, 0x05, 0xFF, 0x00};
  byte bArrayClose[] = {0x57, 0x0F, 0x45, 0x01, 0x05, 0xFF, 0x64};
  byte bArrayPause[] = {0x57, 0x0F, 0x45, 0x01, 0x00, 0xFF};
  byte bArrayPos[] =  {0x57, 0x0F, 0x45, 0x01, 0x05, 0xFF, NULL};
  byte bArrayGetSettings[] = {0x57, 0x02};
  byte bArrayHoldSecs[] = {0x57, 0x0F, 0x08, NULL };
  byte bArrayBotMode[] = {0x57, 0x03, 0x64, NULL, NULL};

  byte bArrayPressPass[] = {0x57, 0x11, NULL, NULL, NULL, NULL};
  byte bArrayOnPass[] = {0x57, 0x11, NULL , NULL, NULL, NULL, 0x01};
  byte bArrayOffPass[] = {0x57, 0x11, NULL, NULL, NULL, NULL, 0x02};
  byte bArrayGetSettingsPass[] = {0x57, 0x12, NULL, NULL, NULL, NULL};
  byte bArrayHoldSecsPass[] = {0x57, 0x1F, NULL, NULL, NULL, NULL, 0x08, NULL };
  byte bArrayBotModePass[] = {0x57, 0x13, NULL, NULL, NULL, NULL, 0x64, NULL};       // The proper array to use for setting mode with password (firmware 4.9)

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
      printAString("Failed to connect for sending command");
      return false;
    }
    count++;
    printAString("Attempt to send command. Not connecting. Try connecting...");
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
    returnValue = subscribeToNotify(advDeviceToUse);
  }
  else if (isCurtainDevice(aDevice))
  {
    returnValue = subscribeToNotify(advDeviceToUse);
  }
  else if (isPlugDevice(aDevice))
  {
    returnValue = subscribeToNotify(advDeviceToUse);
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
              printAString("Num is for a bot device - no pass");
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
            if (isBotDevice(aDevice)) {
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
            if (isPlugDevice(aDevice)) {
              wasSuccess = sendPlugCommandBytes(pChr, bArrayPlugOn, 6);

            }

          }
          else if (strcmp(type, "OFF") == 0) {
            if (isBotDevice(aDevice)) {
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
            if (isPlugDevice(aDevice)) {
              wasSuccess = sendPlugCommandBytes(pChr, bArrayPlugOff, 6);
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
            printAString("Wrote new value to: ");
            printAString(pChr->getUUID().toString().c_str());
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
      printAString("CUSTOM write service not found.");
      returnValue = false;
    }
  }
  if (!returnValue) {
    if (attempts >= 10) {
      printAString("Sending failed. Disconnecting client");
      pClient->disconnect();
    } return false;
  }
  if (disconnectAfter) {
    pClient->disconnect();
  }
  printAString("Success! Command sent/received to/from SwitchBot");
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
      printAString(pChr->getUUID().toString().c_str());
      printAString(" Value: ");
      printAString(pChr->readValue().c_str());
      // should return WoHand
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
  printAString("Requesting info...");
  rescanFind(advDeviceToUse->getAddress().toString().c_str());
  return true;
}

void notifyCB(NimBLERemoteCharacteristic * pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  printAString("notifyCB");
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

  char aBuffer[120];
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
      printAString("The response value from bot set mode or holdSecs: ");
      printAString(byte1);
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
        //HACK to send battery 1% if bot cannot be controlled
        std::string deviceBatteryTopic = botTopic + aDevice + "/battery";
        addToPublish(deviceBatteryTopic, 1, true);
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
      printAString("The response value from bot action: ");
      printAString(byte1);
      if (byte1 == 3) {
        statDoc["status"] = "busy";
        lastCommandWasBusy = true;
      }
      //SUCCESS == 1 (on/off) or == 5 (press) for bot
      else if (byte1 == 1 || byte1 == 5) {
        statDoc["status"] = "success";
        lastCommandWasBusy = false;
        if (strcmp(aCommand.c_str(), "OFF") == 0) {
          addToPublish(deviceAssumedStateTopic.c_str(), "OFF", true);
        }
        else if (strcmp(aCommand.c_str(), "ON") == 0) {
          addToPublish(deviceAssumedStateTopic.c_str(), "ON", true);
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
            addToPublish(deviceAssumedStateTopic.c_str(), botsSimulatedStates[aDevice] ? "ON" : "OFF", true);
          }
        }
      }
      else if (byte1 == 6) {
        statDoc["status"] = "lowbattery";
        lastCommandWasBusy = false;
        //HACK to send battery 1% if bot cannot be controlled
        std::string deviceBatteryTopic = botTopic + aDevice + "/battery";
        addToPublish(deviceBatteryTopic, 1, true);
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
            addToPublish(deviceAttrTopic.c_str(), aBuffer, true);
            addToPublish(deviceStateTopic.c_str(), aState.c_str(), true);*/
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
      addToPublish(deviceSettingsTopic.c_str(), aBuffer, true);

      botHoldSecs[deviceMac] = holdSecs;
      botFirmwares[deviceMac] = (String(fwVersion, 1)).c_str();
      botNumTimers[deviceMac] = timersNumber;
      botInverteds[deviceMac] = inverted;
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

      printAString("The response value from curtain: ");
      printAString(byte1);
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
        //HACK to send battery 1% if curtain cannot be controlled
        std::string deviceBatteryTopic = curtainTopic + aDevice + "/battery";
        addToPublish(deviceBatteryTopic, 1, true);
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

  else if (deviceName == plugName) {
    deviceStatusTopic = plugTopic + aDevice + "/status";
    deviceSettingsTopic = plugTopic + aDevice + "/settings";
    deviceAttrTopic = plugTopic + aDevice + "/attributes";

    if (!lastCommandSentPublished) {
      StaticJsonDocument<50> statDoc;
      statDoc["status"] = "commandSent";
      statDoc["command"] = aCommand;
      serializeJson(statDoc, aBuffer);
      client.publish(deviceStatusTopic.c_str(), aBuffer);
      lastCommandSentPublished = true;
    }
    if (length < 2) {
      return;
    }
    else if (length == 2) {
      Serial.println("length:");
      Serial.println(length);
      StaticJsonDocument<50> statDoc;
      uint8_t byte1 = pData[0];

      printAString("The response value from plug: ");
      printAString(byte1);
      if (byte1 == 3) {
        statDoc["status"] = "busy";
        lastCommandWasBusy = true;
      }
      //SUCCESS == 1 or == 5 for plugTopic ????? just assuming based on bot
      else if (byte1 == 1 || byte1 == 5) {
        statDoc["status"] = "success";
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

}
