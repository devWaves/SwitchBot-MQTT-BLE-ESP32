/** SwitchBot-MQTT-BLE-ESP32:

  https://github.com/devWaves/SwitchBot-MQTT-BLE-ESP32

  **does not use/require switchbot hub

  Code can be installed using Arduino IDE for ESP32
  Allows for "unlimited" switchbots devices to be controlled via MQTT sent to ESP32. ESP32 will send BLE commands to switchbots and return MQTT responses to the broker
     * I do not know where performance will be affected by number of devices
     ** This is an unofficial SwitchBot integration. User takes full responsibility with the use of this code**

  v2.0
  
    Created: on May 20 2021
        Author: devWaves

        Contributions from:
		HardcoreWR
          
  based off of the work from https://github.com/combatistor/ESP32_BLE_Gateway

  Notes:
    - Supports Home Assistant MQTT Discovery

    - Support bots and curtains and meters

    - It works for button press/on/off

    - It works for curtain open/close/pause/position(%)

    - It can request status values (bots/curtain/meter: battery, mode, state, position, temp etc) using a "rescan" for all devices

    - It can request individual device status values (bots/curtain/meter: battery, mode, state, position, temp etc) using a "requestInfo"

    - Good for placing one ESP32 in a zone with 1 or more devices that has a bad bluetooth signal from your smart hub. MQTT will use Wifi to "boost" the bluetooth signal

    - ESP32 bluetooth is pretty strong and one ESP32 can work for entire house. The code will try around 60 times to connect/push button. It should not need this many but it depends on ESP32 bluetooth signal to switchbots. If one alone doesn't work, get another esp32 and place it in the problem area

    - OTA update added. Go to ESP32 IP address in browser. In Arduino IDE menu - Sketch / Export compile Binary . Upload the .bin file

    - Supports passwords on bot

    - Automatically rescan every X seconds

    - Automatically requestInfo X seconds after successful control command


    ESP32 will Suscribe to MQTT topic for control
      -<ESPMQTTTopic>/control

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
      - {"status":"press"}
      - {"status":"errorConnect"}
      - {"status":"errorCommand"}
      - {"status":"commandSent"}

  ESP32 will respond with MQTT on esp32Topic with ESP32 status
    - <ESPMQTTTopic>/ESP32

    example payloads:
      {status":"idle"}

  ESP32 will Suscribe to MQTT topic to rescan for all device information
      - switchbotMQTT/rescan

    send a JSON payload of how many seconds you want to rescan for
          example payloads =
            {"sec":"30"}

  ESP32 will Suscribe to MQTT topic for device information
      - <ESPMQTTTopic>/requestInfo

    send a JSON payload of the device you want to control
          example payloads =
            {"id":"switchbotone"}

    ESP32 will respond with MQTT on
    - <ESPMQTTTopic>/#

    Example reponses as device are detected:
      - <ESPMQTTTopic>/bot/<name>/attributes
      - <ESPMQTTTopic>/curtain/<name>/attributes
      - <ESPMQTTTopic>/meter/<name>/attributes
        Example payloads:
          - {"rssi":-78,"mode":"Press","state":"OFF","batt":94}
          - {"rssi":-66,"calib":true,"batt":55,"pos":50,"state":"open","light":1}
          - {"rssi":-66,"scale":"c","batt":55,"C":"21.5","F":"70.7","hum":"65"}
          
      - <ESPMQTTTopic>/bot/<name>/state
      - <ESPMQTTTopic>/curtain/<name>/state
      - <ESPMQTTTopic>/meter/<name>/state
        Example payloads:
          - ON
          - OFF
          - OPEN
          - CLOSE
          - 50

  Errors that cannot be linked to a specific device will be published to
      - <ESPMQTTTopic>/ESP32

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

/* ESP32 LED Settings */
#define LED_PIN LED_BUILTIN                          // If your board doesn't have a defined LED_BUILTIN (You will get a compile error), comment this line out
//#define LED_PIN 2                                  // If your board doesn't have a defined LED_BUILTIN, uncomment this line out and replace 2 with the LED pin value
static bool ledHighEqualsON = true;                  // ESP32 board LED ON=HIGH (Default). If your ESP32 is turning OFF on scanning and turning ON while IDLE, then set this value to false 
static bool ledOnBootScan = true;                    // Turn on LED during initial boot scan
static bool ledOnScan = true;                        // Turn on LED while scanning (non-boot)
static bool ledOnCommand = true;                     // Turn on LED while MQTT command is processing. If scanning, LED will blink after scan completes. You may not notice it, there is no delay after scan

/* Wifi Settings */
static const char* host = "esp32";                   //  hostname defaults is esp32
static const char* ssid = "SSID";                    //  WIFI SSID
static const char* password = "Password";            //  WIFI Password

/* Webserver Settings */
static String otaUserId = "admin";                   //  user Id for OTA update
static String otaPass = "admin";                     //  password for OTA update
static WebServer server(80);                         //  default port 80

/* MQTT Settings */
static const char* mqtt_host = "192.168.0.1";         //  MQTT Broker server ip
static const char* mqtt_user = "switchbot";           //  MQTT Broker username. If empty, no authentication will be used
static const char* mqtt_pass = "switchbot";           //  MQTT Broker password.
static const char* mqtt_clientname = "switchbot";     //  Client name that uniquely identify your device
static const int mqtt_port = 1883;                    //  MQTT Port
static std::string ESPMQTTTopic = "switchbot";        //  MQTT main topic
static const uint16_t mqtt_packet_size = 1024;

/* Home Assistant Settings */
static bool home_assistant_mqtt_discovery = true;                    //  Enable to publish Home Assistant MQTT Discovery config
static std::string home_assistant_mqtt_prefix = "homeassistant";     //  MQTT Home Assistant prefix
static bool home_assistant_expose_seperate_curtain_position = true; // When enabled, a seperate sensor will be added that will expose the curtain position. This is useful when using the Prometheus integration to graph curtain positions. The cover entity doesn't expose the position for Prometheus

/* Switchbot General Settings */
static int tryConnecting = 60;          // How many times to try connecting to bot
static int trySending = 30;             // How many times to try sending command to bot
static int initialScan = 120;           // How many seconds to scan for bots on ESP reboot and autoRescan
static int infoScanTime = 60;           // How many seconds to scan for single device status updates

static bool autoRescan = true;          // perform automatic rescan (uses rescanTime and initialScan)
static bool scanAfterControl = true;    // perform requestInfo after successful control command (uses botScanTime)
static int rescanTime = 600;            // Automatically rescan for device info every X seconds (default 10 min)
static int queueSize = 50;              // Max number of control/requestInfo/rescan MQTT commands stored in the queue. If you send more then queueSize, they will be ignored

/* Switchbot Bot Settings */
/* Make sure to use a lower case mac address because some Bluetooth scan functions return a lower case mac address! */
static std::map<std::string, std::string> allBots = {
  { "switchbotone", "xX:xX:xX:xX:xX:xX" },
  { "switchbottwo", "yY:yY:yY:yY:yY:yY" }
};

/* Switchbot Meter Settings */
/* Make sure to use a lower case mac address because some Bluetooth scan functions return a lower case mac address! */
static std::map<std::string, std::string> allMeters = {
  /*{ "meterone", "xX:xX:xX:xX:xX:xX" },
    { "metertwo", "yY:yY:yY:yY:yY:yY" }*/
};

/* Switchbot Curtain Settings */
/* Make sure to use a lower case mac address because some Bluetooth scan functions return a lower case mac address! */
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

/* Switchbot Bot/Meter/Curtain scan interval */
/* Meters don't support commands so will be scanned every <int> interval */
static std::map<std::string, int> botScanTime = {     // X seconds after a successful control command ESP32 will perform a requestInfo on the bot. If a "hold time" is set on the bot include that value + 5to10 secs. Default is 30 sec if not in list
  /*,{ "curtainone", 20 },
    { "curtaintwo", 20 }*/
};

/*************************************************************/

/* ANYTHING CHANGED BELOW THIS COMMENT MAY RESULT IN ISSUES - ALL SETTINGS TO CONFIGURE ARE ABOVE THIS LINE */

/*
   Login page
*/

static const String versionNum = "v2.0";
static const String loginIndex =
  "<form name='loginForm'>"
  "<table width='20%' bgcolor='A09F9F' align='center'>"
  "<tr>"
  "<td colspan=2>"
  "<center><font size=4><b>SwitchBot ESP32 MQTT version: " + versionNum + "</b></font></center>"
  "<center><font size=2><b>(Unofficial)</b></font></center>"
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
  "<script src='https://ajax.googleapis.com/ajax/libs/jquery/3.2.1/jquery.min.js'></script>"
  "<form method='POST' action='#' enctype='multipart/form-data' id='upload_form'>"
  "<input type='file' name='update'>"
  "<input type='submit' value='Update'>"
  "</form>"
  "<div id='prg'>progress: 0%</div>"
  "<script>"
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
  "$('#prg').html('progress: ' + Math.round(per*100) + '%');"
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
  "</script>";

static EspMQTTClient client(
  ssid,
  password,
  mqtt_host,                            //  MQTT Broker server ip
  mqtt_user,                            //  Can be omitted if not needed
  mqtt_pass,                            //  Can be omitted if not needed
  mqtt_clientname,                      //  Client name that uniquely identify your device
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
static std::map<std::string, NimBLEAdvertisedDevice*> allSwitchbotsDev = {};
static std::map<std::string, long> rescanTimes = {};
static std::map<std::string, std::string> allSwitchbotsOpp;
static std::map<std::string, bool> discoveredDevices = {};
static std::map<std::string, bool> botsInPressMode = {}; 
static std::map<std::string, std::string> deviceTypes;
static NimBLEScan* pScan;
static bool isRescanning = false;
static bool processing = false;
static bool initialScanComplete = false;
static bool firstScan = true;
static std::string esp32Topic = ESPMQTTTopic + "/ESP32";
static std::string lastWillStr = ESPMQTTTopic + "/lastwill";
static const char* lastWill = lastWillStr.c_str();
static std::string botTopic = ESPMQTTTopic + "/bot/";
static std::string curtainTopic = ESPMQTTTopic + "/curtain/";
static std::string meterTopic = ESPMQTTTopic + "/meter/";
static std::string rescanStdStr = ESPMQTTTopic + "/rescan";
//static std::string controlStdStr = ESPMQTTTopic + "/control";
static std::string controlStdStr = ESPMQTTTopic + "/#/#/set";
static std::string requestInfoStdStr = ESPMQTTTopic + "/requestInfo";
static const String rescanTopic = rescanStdStr.c_str();
static const String controlTopic = controlStdStr.c_str();
static const String requestInfoTopic = requestInfoStdStr.c_str();

static byte bArrayPress[] = {0x57, 0x01};
static byte bArrayOn[] = {0x57, 0x01, 0x01};
static byte bArrayOff[] = {0x57, 0x01, 0x02};
static byte bArrayOpen[] =  {0x57, 0x0F, 0x45, 0x01, 0x05, 0xFF, 0x00};
static byte bArrayClose[] = {0x57, 0x0F, 0x45, 0x01, 0x05, 0xFF, 0x64};
static byte bArrayPause[] = {0x57, 0x0F, 0x45, 0x01, 0x00, 0xFF};
//static byte bArrayPos[] =  {0x57, 0x0F, 0x45, 0x01, 0x05, 0xFF, NULL};

static byte bArrayPressPass[] = {0x57, 0x11, NULL, NULL, NULL, NULL};
static byte bArrayOnPass[] = {0x57, 0x11, NULL , NULL, NULL, NULL, 0x01};
static byte bArrayOffPass[] = {0x57, 0x11, NULL, NULL, NULL, NULL, 0x02};

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

long lastRescan = 0;
long lastScanCheck = 0;

void publishHomeAssistantDiscoveryBotConfig(std::string deviceName, std::string deviceMac) {
  std::transform(deviceMac.begin(), deviceMac.end(), deviceMac.begin(), ::toupper);
  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/battery/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\"," +
                                                                                                    + "\"name\":\"" + deviceName + " Battery\"," +
                                                                                                    + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
                                                                                                    + "\"uniq_id\":\"switchbot_" + deviceMac + "_battery\"," +
                                                                                                    + "\"stat_t\":\"~/attributes\"," +
                                                                                                    + "\"dev_cla\":\"battery\"," +
                                                                                                    + "\"unit_of_meas\": \"%\", " +
                                                                                                    + "\"value_template\":\"{{ value_json.batt }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/linkquality/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\"," +
                                                                                                    + "\"name\":\"" + deviceName + " Linkquality\"," +
                                                                                                    + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
                                                                                                    + "\"uniq_id\":\"switchbot_" + deviceMac + "_linkquality\"," +
                                                                                                    + "\"stat_t\":\"~/attributes\"," +
                                                                                                    + "\"icon\":\"mdi:signal\"," +
                                                                                                    + "\"unit_of_meas\": \"rssi\", " +
                                                                                                    + "\"value_template\":\"{{ value_json.rssi }}\"}").c_str(), true);
  
  client.publish((home_assistant_mqtt_prefix + "/switch/" + deviceName + "/config").c_str(), ("{\"~\":\"" + (botTopic + deviceName) + "\", " +
                                                                                                    + "\"name\":\"" + deviceName + " Switch\"," +
                                                                                                    + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + botModel + "\",\"name\": \"" + deviceName + "\" }," +
                                                                                                    + "\"uniq_id\":\"switchbot_" + deviceMac + "\", " +
                                                                                                    + "\"stat_t\":\"~/state\", " +
                                                                                                    + "\"cmd_t\": \"~/set\" }").c_str(), true);
}

void publishHomeAssistantDiscoveryCurtainConfig(std::string deviceName, std::string deviceMac) {
  std::transform(deviceMac.begin(), deviceMac.end(), deviceMac.begin(), ::toupper);
  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/battery/config").c_str(), ("{\"~\":\"" + (curtainTopic + deviceName) + "\"," +
                                                                                                    + "\"name\":\"" + deviceName + " Battery\"," +
                                                                                                    + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + curtainModel + "\",\"name\": \"" + deviceName + "\" }," +
                                                                                                    + "\"uniq_id\":\"switchbot_" + deviceMac + "_battery\"," +
                                                                                                    + "\"stat_t\":\"~/attributes\"," +
                                                                                                    + "\"dev_cla\":\"battery\"," +
                                                                                                    + "\"unit_of_meas\": \"%\", " +
                                                                                                    + "\"value_template\":\"{{ value_json.batt }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/linkquality/config").c_str(), ("{\"~\":\"" + (curtainTopic + deviceName) + "\"," +
                                                                                                    + "\"name\":\"" + deviceName + " Linkquality\"," +
                                                                                                    + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + curtainModel + "\",\"name\": \"" + deviceName + "\" }," +
                                                                                                    + "\"uniq_id\":\"switchbot_" + deviceMac + "_linkquality\"," +
                                                                                                    + "\"stat_t\":\"~/attributes\"," +
                                                                                                    + "\"icon\":\"mdi:signal\"," +
                                                                                                    + "\"unit_of_meas\": \"rssi\", " +
                                                                                                    + "\"value_template\":\"{{ value_json.rssi }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/illuminance/config").c_str(), ("{\"~\":\"" + (curtainTopic + deviceName) + "\"," +
                                                                                                    + "\"name\":\"" + deviceName + " Illuminance\"," +
                                                                                                    + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + curtainModel + "\",\"name\": \"" + deviceName + "\" }," +
                                                                                                    + "\"uniq_id\":\"switchbot_" + deviceMac + "_illuminance\"," +
                                                                                                    + "\"stat_t\":\"~/attributes\"," +
                                                                                                    + "\"dev_cla\":\"illuminance\"," +
                                                                                                    + "\"value_template\":\"{{ value_json.light }}\"}").c_str(), true);

  client.publish((home_assistant_mqtt_prefix + "/binary_sensor/" + deviceName + "/calibrated/config").c_str(), ("{\"~\":\"" + (curtainTopic + deviceName) + "\"," +
                                                                                                    + "\"name\":\"" + deviceName + " Calibrated\"," +
                                                                                                    + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + curtainModel + "\",\"name\": \"" + deviceName + "\" }," +
                                                                                                    + "\"uniq_id\":\"switchbot_" + deviceMac + "_calibrated\"," +
                                                                                                    + "\"stat_t\":\"~/attributes\"," +
                                                                                                    + "\"pl_on\":true," +
                                                                                                    + "\"pl_off\":false," +
                                                                                                    + "\"value_template\":\"{{ value_json.calib }}\"}").c_str(), true);
  
  client.publish((home_assistant_mqtt_prefix + "/cover/" + deviceName + "/config").c_str(), ("{\"~\":\"" + (curtainTopic + deviceName) + "\", " +
                                                                                                    + "\"name\":\"" + deviceName + " Curtain\"," +
                                                                                                    + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + curtainModel + "\",\"name\": \"" + deviceName + "\" }," +
                                                                                                    + "\"uniq_id\":\"switchbot_" + deviceMac + "\", " +
                                                                                                    + "\"dev_cla\":\"curtain\", " +
                                                                                                    + "\"stat_t\":\"~/state\", " +
                                                                                                    + "\"stat_open\": \"OPEN\", "+
                                                                                                    + "\"stat_clsd\": \"CLOSE\", "+
                                                                                                    + "\"pl_stop\":\"PAUSE\", " +
                                                                                                    + "\"pos_open\": 100, "+
                                                                                                    + "\"pos_clsd\": 0, "+
                                                                                                    + "\"cmd_t\": \"~/set\", "+
                                                                                                    + "\"pos_t\":\"~/attributes\", " +
                                                                                                    + "\"pos_tpl\":\"{{ value_json.pos }}\", " +
                                                                                                    + "\"set_pos_t\": \"~/set\", "+
                                                                                                    + "\"set_pos_tpl\": \"{{ position }}\" }").c_str(), true);
  if(home_assistant_expose_seperate_curtain_position) {
    client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/position/config").c_str(), ("{\"~\":\"" + (curtainTopic + deviceName) + "\"," +
                                                                                                      + "\"name\":\"" + deviceName + " Position\"," +
                                                                                                      + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + curtainModel + "\",\"name\": \"" + deviceName + "\" }," +
                                                                                                      + "\"uniq_id\":\"switchbot_" + deviceMac + "_position\"," +
                                                                                                      + "\"stat_t\":\"~/attributes\"," +
                                                                                                      + "\"unit_of_meas\": \"%\", " +
                                                                                                      + "\"value_template\":\"{{ value_json.pos }}\"}").c_str(), true);
  }
}

void publishHomeAssistantDiscoveryMeterConfig(std::string deviceName, std::string deviceMac) {
  std::transform(deviceMac.begin(), deviceMac.end(), deviceMac.begin(), ::toupper);
  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/battery/config").c_str(), ("{\"~\":\"" + (meterTopic + deviceName) + "\"," +
                                                                                                    + "\"name\":\"" + deviceName + " Battery\"," +
                                                                                                    + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + meterModel + "\",\"name\": \"" + deviceName + "\" }," +
                                                                                                    + "\"uniq_id\":\"switchbot_" + deviceMac + "_battery\"," +
                                                                                                    + "\"stat_t\":\"~/attributes\"," +
                                                                                                    + "\"dev_cla\":\"battery\"," +
                                                                                                    + "\"unit_of_meas\": \"%\", " +
                                                                                                    + "\"value_template\":\"{{ value_json.batt }}\"}").c_str());
  
  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/linkquality/config").c_str(), ("{\"~\":\"" + (meterTopic + deviceName) + "\"," +
                                                                                                    + "\"name\":\"" + deviceName + " Linkquality\"," +
                                                                                                    + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + meterModel + "\",\"name\": \"" + deviceName + "\" }," +
                                                                                                    + "\"uniq_id\":\"switchbot_" + deviceMac + "_linkquality\"," +
                                                                                                    + "\"stat_t\":\"~/attributes\"," +
                                                                                                    + "\"icon\":\"mdi:signal\"," +
                                                                                                    + "\"unit_of_meas\": \"rssi\", " +
                                                                                                    + "\"value_template\":\"{{ value_json.rssi }}\"}").c_str());

  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/temperature/config").c_str(), ("{\"~\":\"" + (meterTopic + deviceName) + "\"," +
                                                                                                    + "\"name\":\"" + deviceName + " Temperature\"," +
                                                                                                    + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + meterModel + "\",\"name\": \"" + deviceName + "\" }," +
                                                                                                    + "\"uniq_id\":\"switchbot_" + deviceMac + "_temperature\"," +
                                                                                                    + "\"stat_t\":\"~/attributes\"," +
                                                                                                    + "\"dev_cla\":\"temperature\", " +
                                                                                                    + "\"unit_of_meas\": \"°C\", " +
                                                                                                    + "\"value_template\":\"{{ value_json.C }}\"}").c_str());
  
  client.publish((home_assistant_mqtt_prefix + "/sensor/" + deviceName + "/humidity/config").c_str(), ("{\"~\":\"" + (meterTopic + deviceName) + "\"," +
                                                                                                    + "\"name\":\"" + deviceName + " Humidity\"," +
                                                                                                    + "\"device\": {\"identifiers\":[\"switchbot_" + deviceMac + "\"],\"manufacturer\":\"" + manufacturer + "\",\"model\":\"" + meterModel + "\",\"name\": \"" + deviceName + "\" }," +
                                                                                                    + "\"uniq_id\":\"switchbot_" + deviceMac + "_humidity\"," +
                                                                                                    + "\"stat_t\":\"~/attributes\"," +
                                                                                                    + "\"dev_cla\":\"humidity\", " +
                                                                                                    + "\"unit_of_meas\": \"%\", " +
                                                                                                    + "\"value_template\":\"{{ value_json.hum }}\"}").c_str());
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

/** Define a class to handle the callbacks when advertisments are received */
class AdvertisedDeviceCallbacks: public NimBLEAdvertisedDeviceCallbacks {
    void onResult(NimBLEAdvertisedDevice* advertisedDevice) {
      Serial.print("Advertised Device found: ");
      Serial.println(advertisedDevice->toString().c_str());
      std::string advStr = advertisedDevice->getAddress().toString().c_str();
      std::map<std::string, std::string>::iterator itS = allSwitchbotsOpp.find(advStr);

      bool gotAllStatus = false;

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
        
        if(home_assistant_mqtt_discovery) {
          if(itM == discoveredDevices.end()) {
            Serial.printf("Publishing MQTT Discovery for %s (%s)\n", aDevice.c_str(), deviceMac.c_str());
            publishHomeAssistantDiscoveryBotConfig(aDevice, deviceMac);
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

        if(home_assistant_mqtt_discovery) {
          if(itM == discoveredDevices.end()) {
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
        deviceStateTopic = curtainTopic + aDevice + "/state";
        deviceAttrTopic = curtainTopic + aDevice + "/attributes";
        
        uint8_t byte1 = (uint8_t) aValueString[1];
        uint8_t byte2 = (uint8_t) aValueString[2];
        uint8_t byte3 = (uint8_t) aValueString[3];
        uint8_t byte4 = (uint8_t) aValueString[4];

        bool calibrated = byte1 & 0b01000000;;
        int battLevel = byte2 & 0b01111111;
        int currentPosition = 100-(byte3 & 0b01111111);
        int lightLevel = (byte4 >> 4) & 0b00001111;
        aState = "OPEN";
        
        doc["calib"] = calibrated;
        doc["batt"] = battLevel;
        doc["pos"] = currentPosition;
        if(currentPosition <= curtainClosedPosition)
          aState = "CLOSE";
        doc["state"] = aState;
        doc["light"] = lightLevel;
        
        if(home_assistant_mqtt_discovery) {
          if(itM == discoveredDevices.end()) {
            Serial.printf("Publishing MQTT Discovery for %s (%s)\n", aDevice.c_str(), deviceMac.c_str());
            publishHomeAssistantDiscoveryCurtainConfig(aDevice, deviceMac);
            discoveredDevices.insert( std::pair<std::string, bool>(deviceMac, true) );
          }
        }
      }
      else {
        return false;
      }
      Serial.println("serializing");
      serializeJson(doc, aBuffer);
      client.publish(deviceStateTopic.c_str(), aState.c_str());
      client.publish(deviceAttrTopic.c_str(), aBuffer);
      Serial.println("published");
      return true;
    };
};

void scanEndedCB(NimBLEScanResults results) {
  if ((ledOnScan && !firstScan) || (ledOnBootScan && firstScan)  || ledOnCommand) {
    digitalWrite(LED_PIN, ledOFFValue);
  }
  firstScan = false;
  Serial.println("Scan Ended");
  client.publish(esp32Topic.c_str(), "{\"status\":\"idle\"}");
}

void rescanEndedCB(NimBLEScanResults results) {
  if (ledOnScan || ledOnCommand) {
    digitalWrite(LED_PIN, ledOFFValue);
  }
  isRescanning = false;
  lastRescan = millis();
  Serial.println("ReScan Ended");
  client.publish(esp32Topic.c_str(), "{\"status\":\"idle\"}");
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
    server.send(200, "text/html", loginIndex);
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
  client.enableLastWillMessage(lastWill, "Offline");
  client.setKeepAlive(60);
  client.setMaxPacketSize(mqtt_packet_size);

  std::map<std::string, std::string>::iterator it = allBots.begin();
  std::string anAddr;

  while (it != allBots.end())
  {
    anAddr = it->second;
    std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
    allSwitchbotsOpp.insert ( std::pair<std::string, std::string>(anAddr.c_str(), it->first) );
    deviceTypes.insert ( std::pair<std::string, std::string>(anAddr.c_str(), botName) );
    it++;
  }

  it = allCurtains.begin();
  while (it != allCurtains.end())
  {
    anAddr = it->second;
    std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
    allSwitchbotsOpp.insert ( std::pair<std::string, std::string>(anAddr.c_str(), it->first) );
    deviceTypes.insert ( std::pair<std::string, std::string>(anAddr.c_str(), curtainName) );
    it++;
  }

  it = allMeters.begin();
  while (it != allMeters.end())
  {
    anAddr = it->second;
    std::transform(anAddr.begin(), anAddr.end(), anAddr.begin(), to_lower());
    allSwitchbotsOpp.insert ( std::pair<std::string, std::string>(anAddr.c_str(), it->first) );
    deviceTypes.insert ( std::pair<std::string, std::string>(anAddr.c_str(), meterName) );
    it++;
  }

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
  client.publish(esp32Topic.c_str(), "{\"status\":\"scanning\"}");
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
  client.publish(esp32Topic.c_str(), "{\"status\":\"scanning\"}");
  delay(50);
  if (ledOnScan) {
    digitalWrite(LED_PIN, ledONValue);
  }
  pScan->start(infoScanTime, scanEndedCB, true);
}

void loop () {
  server.handleClient();
//  client.setMaxPacketSize(mqtt_packet_size);
  client.loop();

  if (isRescanning) {
    lastRescan = millis();
  }
  if (!processing && !(pScan->isScanning()) && !isRescanning) {
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
      while (it != rescanTimes.end())
      {
        itB = allSwitchbotsOpp.find(it->first);
        itS = botScanTime.find(itB->second);
        long lastTime = it->second;
        long scanTime = 30; //default if not in list
        if (itS != botScanTime.end())
        {
          scanTime = itS->second;
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

bool processQueue() {
  struct QueueCommand aCommand;
  while (!commandQueue.isEmpty()) {
    aCommand = commandQueue.getHead();
    if ((aCommand.topic == ESPMQTTTopic + "/rescan") && isRescanning) {
      commandQueue.dequeue();
    }
    else {
      if ( processing || pScan->isScanning() || isRescanning ) {
        return false;
      }
      Serial.print("Received something on ");
      Serial.println(aCommand.topic.c_str());
      Serial.println(aCommand.device.c_str());
      if (aCommand.topic == ESPMQTTTopic + "/control") {
        if (ledOnCommand) {
          digitalWrite(LED_PIN, ledONValue);
        }
        controlMQTT(aCommand.device, aCommand.payload);
        if (ledOnCommand) {
          digitalWrite(LED_PIN, ledOFFValue);
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
      commandQueue.dequeue();
    }
  }
  return true;
}

void sendToDevice(NimBLEAdvertisedDevice* advDevice, std::string aName, const char * command, std::string deviceTopic) {

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
    if (strcmp(command, "requestInfo") == 0) {
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
          shouldContinue = false;
          doc["status"] = "commandSent";
          serializeJson(doc, aBuffer);
          client.publish(deviceTopic.c_str(), aBuffer);
          if(!is_number(command)) {
            client.publish(deviceStateTopic.c_str(), command);
          }
          std::map<std::string, bool>::iterator itP = botsInPressMode.find(addr);
          if (itP != botsInPressMode.end())
          {
            client.publish(deviceStateTopic.c_str(), "OFF");
          }
          if (scanAfterControl) {
            std::map<std::string, std::string>::iterator itI = allSwitchbotsOpp.find(addr);
            std::map<std::string, long>::iterator itW = rescanTimes.find(addr);
            if (itW != rescanTimes.end())
            {
              rescanTimes.erase(addr);
            }
            rescanTimes.insert ( std::pair<std::string, long>(addr, millis()));
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
}

bool is_number(const std::string& s)
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
      if ((strcmp(payload.c_str(), "PRESS") == 0) || (strcmp(payload.c_str(), "ON") == 0) || (strcmp(payload.c_str(), "OFF") == 0) || (strcmp(payload.c_str(), "OPEN") == 0) || (strcmp(payload.c_str(), "CLOSE") == 0) || (strcmp(payload.c_str(), "PAUSE") == 0)) {
        processRequest(deviceAddr, device, payload.c_str(), deviceTopic);
      }
      else {
        char aBuffer[100];
        StaticJsonDocument<100> docOut;
        docOut["status"] = "errorJSONValue";
        serializeJson(docOut, aBuffer);
        Serial.println("Parsing failed = value not a valid command");
        client.publish(esp32Topic.c_str(), aBuffer);
      }
    }
  }
  else {
    char aBuffer[100];
    StaticJsonDocument<100> docOut;
    docOut["status"] = "errorJSONDevice";
    serializeJson(docOut, aBuffer);
    Serial.println("Parsing failed = device not from list");
    client.publish(esp32Topic.c_str(), aBuffer);
  }

  delay(100);
  client.publish(esp32Topic.c_str(), "{\"status\":\"idle\"}");
  processing = false;
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
    client.publish(esp32Topic.c_str(), aBuffer);
  }
  else {
    const char * value = docIn["sec"];
    if (value != "") {
      bool isNum = is_number(value);
      if (isNum) {
        int aVal;
        sscanf(value, "%d", &aVal);
        if (aVal < 0) {
          return;
        }
        else if (aVal > 120) {
          aVal = 120;
        }
        rescan(aVal);
      }
      else {
        char aBuffer[100];
        StaticJsonDocument<100> docOut;
        docOut["status"] = "errorJSONValue";
        serializeJson(docOut, aBuffer);
        Serial.println("Parsing failed = device not from list");
        client.publish(esp32Topic.c_str(), aBuffer);
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
    client.publish(esp32Topic.c_str(), aBuffer);
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
      client.publish(esp32Topic.c_str(), aBuffer);
    }
  }
  processing = false;
}

void onConnectionEstablished() {
  std::string anAddr;
  std::string aDevice;
  std::map<std::string, std::string>::iterator it;

  if (!initialScanComplete) {
    if (ledOnBootScan) {
      digitalWrite(LED_PIN, ledONValue);
    }
    client.publish(esp32Topic.c_str(), "{\"status\":\"boot\"}");
    delay(100);
  }

  if (!initialScanComplete) {
    initialScanComplete = true;
    firstScan = true;
    client.publish(esp32Topic.c_str(), "{\"status\":\"scanning\"}");
    pScan->start(initialScan, scanEndedCB, true);
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
        client.publish(esp32Topic.c_str(), "{\"status\":\"errorQueueFull\"}");
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
        struct QueueCommand queueCommand;
        queueCommand.payload = payload.c_str();
        queueCommand.topic = ESPMQTTTopic + "/control";
        queueCommand.device = aDevice;
        commandQueue.enqueue(queueCommand);
      }
      else {
        client.publish(esp32Topic.c_str(), "{\"status\":\"errorQueueFull\"}");
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
        client.publish(esp32Topic.c_str(), "{\"status\":\"errorQueueFull\"}");
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
      client.publish(esp32Topic.c_str(), "{\"status\":\"errorQueueFull\"}");
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
      client.publish(esp32Topic.c_str(), "{\"status\":\"errorQueueFull\"}");
    }
  });
}

bool connectToServer(NimBLEAdvertisedDevice* advDeviceToUse) {
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

bool sendCommandBytes(NimBLERemoteCharacteristic* pChr, byte* bArray, int aSize ) {
  if (pChr == nullptr) {
    return false;
  }
  return pChr->writeValue(bArray, aSize, false);
}

bool sendCommand(NimBLEAdvertisedDevice* advDeviceToUse, const char * type, int attempts) {
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

  //getGeneric(advDeviceToUse);
  NimBLERemoteService* pSvc = nullptr;
  NimBLERemoteCharacteristic* pChr = nullptr;
  bool returnValue = true;
  pSvc = pClient->getService("cba20d00-224d-11e6-9fb8-0002a5d5c51b");
  if (pSvc) {
    pChr = pSvc->getCharacteristic("cba20002-224d-11e6-9fb8-0002a5d5c51b");
  }
  if (pChr) {
    if (pChr->canWrite()) {
      bool wasSuccess = false;
      bool isNum = is_number(type);
      std::string aPass = "";
      std::map<std::string, std::string>::iterator itU = allSwitchbotsOpp.find(anAddr);
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
      }
      if (isNum) {
        int aVal;
        sscanf(type, "%d", &aVal);
        byte bArrayPos[] =  {0x57, 0x0F, 0x45, 0x01, 0x05, 0xFF, (100-aVal)};
        wasSuccess = sendCommandBytes(pChr, bArrayPos, 7);
      }
      else {
        if (strcmp(type, "PRESS") == 0) {
          if (aPass == "") {
            wasSuccess = sendCommandBytes(pChr, bArrayPress, 2);
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
            wasSuccess = sendCommandBytes(pChr, anArray , 6);
          }
        }
        else if (strcmp(type, "ON") == 0) {
          if (aPass == "") {
            wasSuccess = sendCommandBytes(pChr, bArrayOn, 3);
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
            wasSuccess = sendCommandBytes(pChr, anArray , 7);
          }
        }
        else if (strcmp(type, "OFF") == 0) {
          if (aPass == "") {
            wasSuccess = sendCommandBytes(pChr, bArrayOff, 3);
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
            wasSuccess = sendCommandBytes(pChr, anArray , 7);
          }
        }
        else if (strcmp(type, "OPEN") == 0) {
          wasSuccess = sendCommandBytes(pChr, bArrayOpen, 7);
        }
        else if (strcmp(type, "CLOSE") == 0) {
          wasSuccess = sendCommandBytes(pChr, bArrayClose, 7);
        }
        else if (strcmp(type, "PAUSE") == 0) {
          wasSuccess = sendCommandBytes(pChr, bArrayPause, 6);
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

  if (!returnValue) {
    if (attempts >= 10) {
      Serial.println("Sending failed. Disconnecting client");
      pClient->disconnect();
    } return false;
  }
  pClient->disconnect();
  Serial.println("Success! Command sent/received to/from SwitchBot");
  return true;
}

bool getGeneric(NimBLEAdvertisedDevice* advDeviceToUse) {
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

bool requestInfo(NimBLEAdvertisedDevice* advDeviceToUse) {
  if (advDeviceToUse == nullptr) {
    return false;
  }
  Serial.println("Requesting info...");
  rescanFind(advDeviceToUse->getAddress().toString().c_str());
  return true;
}
