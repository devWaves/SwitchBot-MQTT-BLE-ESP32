/** SwitchBot-MQTT-BLE-ESP32:

  https://github.com/devWaves/SwitchBot-MQTT-BLE-ESP32

  does not use/require switchbot hub

  Code can be installed using Arduino IDE for ESP32
  Allows for "unlimited" switchbots devices to be controlled via MQTT sent to ESP32. ESP32 will send BLE commands to switchbots and return MQTT responses to the broker
     *** I do not know where performance will be affected by number of devices

  v0.12

    Created: on March 18 2021
        Author: devWaves

  based off of the work from https://github.com/combatistor/ESP32_BLE_Gateway

  Notes:
    - It works for button press/on/off

    - It works for curtain open/close/pause/position(%)

    - It can request setting values (battery, mode, firmware version, Number of timers, Press mode, inverted (yes/no), Hold seconds) using a rescan

    - Good for placing one ESP32 in a zone with 1 or 2 devices that has a bad bluetooth signal from your smart hub. MQTT will use Wifi to "boost" the bluetooth signal

    - ESP32 bluetooth is pretty strong and one ESP32 can work for entire house. The code will try around 60 times to connect/push button. It should not need this many but it depends on ESP32 bluetooth signal to switchbots. If one alone doesn't work, get another esp32 and place it in the problem area

    ESP32 will Suscribe to MQTT topic for control
      -switchbotMQTT/control

    send a JSON payload of the device you want to control
      device = device to control
        value = string value
          "press"
          "on"
          "off"
          "open"
          "close"
          "pause"
          any number 0-100 (for curtain position) Example: "50"

          example payloads =
            {"id":"switchbotone","value":"press"}
            {"id":"switchbotone","value":"open"}
            {"id":"switchbotone","value":"50"}

  ESP32 will respond with MQTT on
      -switchbotMQTT/#

    Example reponses:
      -switchbotMQTT/bot/switchbotone  or  switchbotMQTT/curtain/curtainone   or  switchbotMQTT/meter/meterone

          example payloads =
            {"id":"switchbotone","status":"connected"}
            {"id":"switchbotone","status":"press"}

            {"id":"switchbotone","status":"errorConnect"}
            {"id":"switchbotone","status":"errorCommand"}

      -switchbotMQTT/ESP32

          example payloads =
            {status":"idle"}

  ESP32 will Suscribe to MQTT topic to rescan for all device information
      -switchbotMQTT/rescan

    send a JSON payload of how many seconds you want to rescan for
          example payloads =
            {"sec":"30"}

    ESP32 will respond with MQTT on
    -switchbotMQTT/#

    Example reponses as device are detected:
      -switchbotMQTT/bot/switchbotone  or  switchbotMQTT/curtain/curtainone   or  switchbotMQTT/meter/meterone
          example payloads =
            {"id":"switchbottwo","status":"info","rssi":-78,"mode":"Press","state":"OFF","batt":94}


	Errors that cannot be linked to a specific device will be published to
      -switchbotMQTT/ESP32

*/

#include <NimBLEDevice.h>
#include "EspMQTTClient.h"
#include "ArduinoJson.h"

/****************** CONFIGURATIONS TO CHANGE *******************/

String ESPMQTTTopic = "switchbotMQTT";

EspMQTTClient client(
  "SSID",
  "Password",
  "192.168.1.XXX",  // MQTT Broker server ip
  "MQTTUsername",   // Can be omitted if not needed
  "MQTTPassword",   // Can be omitted if not needed
  "ESPMQTT",      // Client name that uniquely identify your device
  1883            // MQTT Port
);

static std::map<std::string, std::string> allBots = {
  { "switchbotone", "xx:xx:xx:xx:xx:xx" },
  { "switchbottwo", "yy:yy:yy:yy:yy:yy" }
};

static std::map<std::string, std::string> allMeters = {
  /*{ "meterone", "xx:xx:xx:xx:xx:xx" },
	{ "metertwo", "yy:yy:yy:yy:yy:yy" }*/
};

static std::map<std::string, std::string> allCurtains = {
  /*{ "curtainone", "xx:xx:xx:xx:xx:xx" },
	{ "curtaintwo", "yy:yy:yy:yy:yy:yy" }*/
};

static int tryConnecting = 60;  // How many times to try connecting to bot
static int trySending = 30;     // How many times to try sending command to bot
static int initialScan = 30;    // How many seconds to scan for bots on ESP reboot

/*************************************************************/

void scanEndedCB(NimBLEScanResults results);
static std::map<std::string, NimBLEAdvertisedDevice*> allSwitchbotsDev = {};
static std::map<std::string, std::string> allSwitchbotsOpp;
static std::map<std::string, std::string> deviceTypes;
static NimBLEScan* pScan;
static bool processing = false;
String esp32Str = ESPMQTTTopic + "/ESP32";
String lastWillStr = ESPMQTTTopic + "/lastwill";
const char* lastWill = lastWillStr.c_str();
String buttonStr = ESPMQTTTopic + "/bot/";
String curtainStr = ESPMQTTTopic + "/curtain/";
String tempStr = ESPMQTTTopic + "/meter/";

byte bArrayPress[] = {0x57, 0x01};
byte bArrayOn[] = {0x57, 0x01, 0x01};
byte bArrayOff[] = {0x57, 0x01, 0x02};
byte bArrayOpen[] =  {0x57, 0x0F, 0x45, 0x01, 0x05, 0xFF, 0x00};
byte bArrayClose[] = {0x57, 0x0F, 0x45, 0x01, 0x05, 0xFF, 0x64};
byte bArrayPause[] = {0x57, 0x0F, 0x45, 0x01, 0x00, 0xFF};

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

      if (itS != allSwitchbotsOpp.end())
      {
        if (advertisedDevice->isAdvertisingService(NimBLEUUID("cba20d00-224d-11e6-9fb8-0002a5d5c51b")))
        {
          std::map<std::string, NimBLEAdvertisedDevice*>::iterator itY = allSwitchbotsDev.find(advStr);
          if (itY == allSwitchbotsDev.end())
          {
            Serial.println("Adding Our Service ... ");
            Serial.println(itS->second.c_str());
            allSwitchbotsDev.insert ( std::pair<std::string, NimBLEAdvertisedDevice*>(advStr, advertisedDevice) );
            Serial.println("Assigned advDevService");
            std::string aValueString = advertisedDevice->getServiceData(0);
            callForInfoAdvDev( advertisedDevice, aValueString);
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
      String aDevice ;
      String deviceStr ;

      StaticJsonDocument<200> doc;
      char aBuffer[200];
      std::map<std::string, std::string>::iterator itS = allSwitchbotsOpp.find(advDev->getAddress().toString().c_str());
      if (itS != allSwitchbotsOpp.end())
      {
        aDevice = itS->second.c_str();
        doc["id"] = aDevice;
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

      Serial.println("deviceName:");
      Serial.print(deviceName.c_str());

      std::string buttonName = "WoHand";
      std::string tempName = "WoSensorTH";
      std::string curtainName = "WoCurtain";

      doc["status"] = "info";
      doc["rssi"] = advDev->getRSSI();

      if (deviceName == buttonName) {
        deviceStr = buttonStr + aDevice;
        uint8_t byte1 = (uint8_t) aValueString[1];
        uint8_t byte2 = (uint8_t) aValueString[2];

        String aMode = (byte1 & 0b10000000) ? "Switch" : "Press"; // Whether the light switch Add-on is used or not
        String aState = (byte1 & 0b01000000) ? "OFF" : "ON"; // Mine is opposite, not sure why
        int battLevel = byte2 & 0b01111111; // %

        doc["mode"] = aMode;
        doc["state"] = aState;
        doc["batt"] = battLevel;

      }
      else if (deviceName == tempName) {

        deviceStr = tempStr + aDevice;
        uint8_t byte2 = (uint8_t) aValueString[2];
        uint8_t byte3 = (uint8_t) aValueString[3];
        uint8_t byte4 = (uint8_t) aValueString[4];
        uint8_t byte5 = (uint8_t) aValueString[5];

        int tempSign = (byte4 & 0b10000000) ? 1 : -1;
        float tempC = tempSign * ((byte4 & 0b01111111) + (byte3 / 10));
        float tempF = (tempC * 9 / 5) + 32;
        tempF = round(tempF * 10) / 10;
        bool tempScale = (byte5 & 0b10000000) ;
        String str1 = (tempScale == true) ? "f" : "c";
        doc["scale"] = str1;
        int battLevel = (byte2 & 0b01111111);
        doc["batt"] = battLevel;
        doc["C"] = tempC;
        doc["F"] = tempF;
        int humidity = byte5 & 0b01111111;
        doc["hum"] = humidity;

      }
      else if (deviceName == curtainName) {
        deviceStr = curtainStr + aDevice;

        uint8_t byte1 = (uint8_t) aValueString[1];
        uint8_t byte2 = (uint8_t) aValueString[2];
        uint8_t byte3 = (uint8_t) aValueString[3];
        uint8_t byte4 = (uint8_t) aValueString[4];

        bool calibrated = byte1 & 0b01000000;;
        int battLevel = byte2 & 0b01111111;
        int currentPosition = byte3 & 0b01111111;
        int lightLevel = (byte4 >> 4) & 0b00001111;
        doc["calib"] = calibrated;
        doc["batt"] = battLevel;
        doc["pos"] = currentPosition;
        doc["light"] = lightLevel;

      }
      else {
        return false;
      }
      Serial.println("serializing");
      serializeJson(doc, aBuffer);
      client.publish(deviceStr.c_str(), aBuffer);
      Serial.println("published");
      return true;
    };
};

void scanEndedCB(NimBLEScanResults results) {
  Serial.println("Scan Ended");
}

static ClientCallbacks clientCB;

void setup () {
  client.setMqttReconnectionAttemptDelay(100);
  client.enableLastWillMessage(lastWill, "Offline");
  client.setKeepAlive(60);
  std::map<std::string, std::string>::iterator it = allBots.begin();
  while (it != allBots.end())
  {
    allSwitchbotsOpp.insert ( std::pair<std::string, std::string>(it->second, it->first) );
    deviceTypes.insert ( std::pair<std::string, std::string>(it->second, "WoHand") );
    it++;
  }

  it = allCurtains.begin();
  while (it != allCurtains.end())
  {
    allSwitchbotsOpp.insert ( std::pair<std::string, std::string>(it->second, it->first) );
    deviceTypes.insert ( std::pair<std::string, std::string>(it->second, "WoCurtain") );
    it++;
  }

  it = allMeters.begin();
  while (it != allMeters.end())
  {
    allSwitchbotsOpp.insert ( std::pair<std::string, std::string>(it->second, it->first) );
    deviceTypes.insert ( std::pair<std::string, std::string>(it->second, "WoSensorTH") );
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
  pScan->start(initialScan, scanEndedCB);
}

void rescan(int seconds) {
  if (pScan->isScanning()) {
    return;
  }
  allSwitchbotsDev = {};
  pScan->clearResults();
  pScan->start(seconds, scanEndedCB);
}

void loop () {
  client.loop();
}

void processRequest(std::string macAdd, std::string aName, const char * command, String deviceTopic) {
  int count = 1;
  std::map<std::string, NimBLEAdvertisedDevice*>::iterator itS = allSwitchbotsDev.find(macAdd);
  NimBLEAdvertisedDevice* advDevice =  itS->second;
  bool shouldContinue = (advDevice == NULL);
  while (shouldContinue) {
    if (count > 3) {
      shouldContinue = false;
    }
    else {
      count++;
      if (pScan->isScanning()) {
        while (pScan->isScanning()) {
          delay(50);
        }
      }
      else {
        pScan->start(5 * count, scanEndedCB);
        while (pScan->isScanning()) {
          Serial.println("Scanning#" + count);
          delay(500);
        }
      }
      itS = allSwitchbotsDev.find(macAdd);
      advDevice =  itS->second;
      shouldContinue = (advDevice == NULL);
    }
  }
  if (advDevice == NULL)
  {
    StaticJsonDocument<100> doc;
    char aBuffer[100];
    doc["id"] = aName.c_str();
    doc["status"] = "errorLocatingDevice";
    serializeJson(doc, aBuffer);
    client.publish(deviceTopic.c_str(), aBuffer);
  }
  else {
    sendToDevice(advDevice, aName, command, deviceTopic);
  }
}

void sendToDevice(NimBLEAdvertisedDevice* advDevice, std::string aName, const char * command, String deviceTopic) {

  NimBLEAdvertisedDevice* advDeviceToUse = advDevice;
  std::string addr = advDeviceToUse->getAddress().toString().c_str();

  // TODO gets settings from devices
  if (strcmp(command, "requestInfo") == 0) {
    client.publish(esp32Str.c_str(), "requestInfoDoesNotWorkYet");
    return;
  }

  if (advDeviceToUse != NULL)
  {
    bool isConnected = false;
    int count = 0;
    bool shouldContinue = true;
    char aBuffer[100];
    StaticJsonDocument<100> doc;
    doc["id"] = aName.c_str();
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
        if (strcmp(command, "requestInfo") == 0) {
          isSuccess = requestInfo(advDeviceToUse, command, count);
          count++;
          if (isSuccess) {
            shouldContinue = false;
          }
          else {
            if (count > trySending) {
              shouldContinue = false;
              doc["status"] = "errorRequestInfo";
              serializeJson(doc, aBuffer);
              client.publish(deviceTopic.c_str(),  aBuffer);
            }
          }
        }
        else {
          isSuccess = sendCommand(advDeviceToUse, command, count);
          count++;
          if (isSuccess) {
            shouldContinue = false;
            doc["status"] = command;
            serializeJson(doc, aBuffer);
            client.publish(deviceTopic.c_str(),  aBuffer);
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
}

bool is_number(const std::string& s)
{
  std::string::const_iterator it = s.begin();
  while (it != s.end() && std::isdigit(*it)) ++it;
  return !s.empty() && it == s.end();
}

void onConnectionEstablished() {
  client.subscribe(ESPMQTTTopic + "/control", [] (const String & payload)  {
    Serial.println("Control MQTT Received...");
    int count = 0;
    while (( processing || pScan->isScanning()) && (count < 600) ) {
      delay(100);
      count++;
    }
    processing = true;
    Serial.println("Processing Control MQTT...");
    StaticJsonDocument<100> docIn;
    deserializeJson(docIn, payload);

    if (docIn == NULL) { //Check for errors in parsing
      char aBuffer[100];
      StaticJsonDocument<100> docOut;
      Serial.println("Parsing failed");
      docOut["status"] = "errorParsingJSON";
      serializeJson(docOut, aBuffer);
      client.publish(esp32Str.c_str(), aBuffer);
    }
    else {
      const char * aName = docIn["id"]; //Get sensor type value
      const char * value = docIn["value"];        //Get value of sensor measurement
      std::string deviceAddr = "";
      String deviceTopic;
      if (aName != NULL && value != NULL) {
        Serial.print("Device: ");
        Serial.println(aName);
        Serial.print("Device value: ");
        Serial.println(value);

        std::map<std::string, std::string>::iterator itS = allBots.find(aName);
        if (itS != allBots.end())
        {
          deviceAddr = itS->second;
          deviceTopic = buttonStr;
        }
        itS = allCurtains.find(aName);
        if (itS != allCurtains.end())
        {
          deviceAddr = itS->second;
          deviceTopic = curtainStr;
        }
        itS = allMeters.find(aName);
        if (itS != allMeters.end())
        {
          deviceAddr = itS->second;
          deviceTopic = tempStr;
        }
      }
      if (deviceAddr != "") {
        bool isNum = is_number(value);
        deviceTopic = deviceTopic + aName;
        if (isNum) {
          int aVal;
          sscanf(value, "%d", &aVal);
          if (aVal < 0) {
            value = "0";
          }
          else if (aVal > 100) {
            value = "100";
          }
          processRequest(deviceAddr, aName, value, deviceTopic);
        }
        else {
          if ((strcmp(value, "press") == 0) || (strcmp(value, "on") == 0) || (strcmp(value, "off") == 0) || (strcmp(value, "open") == 0) || (strcmp(value, "close") == 0) || (strcmp(value, "pause") == 0)) {
            processRequest(deviceAddr, aName, value, deviceTopic);
          }
          else {
            char aBuffer[100];
            StaticJsonDocument<100> docOut;
            docOut["status"] = "errorJSONValue";
            serializeJson(docOut, aBuffer);
            Serial.println("Parsing failed = value not a valid command");
            client.publish(esp32Str.c_str(), aBuffer);
          }
        }
      }
      else {
        char aBuffer[100];
        StaticJsonDocument<100> docOut;
        docOut["status"] = "errorJSONDevice";
        serializeJson(docOut, aBuffer);
        Serial.println("Parsing failed = device not from list");
        client.publish(esp32Str.c_str(), aBuffer);
      }
    }
    delay(500);
    client.publish(esp32Str.c_str(), "{\"status\":\"idle\"}");
    processing = false;
  });

  client.subscribe(ESPMQTTTopic + "/requestInfo", [] (const String & payload)  {
    Serial.println("Request Info MQTT Received...");
    int count = 0;
    while ((processing || pScan->isScanning()) && (count < 600) ) {
      delay(100);
      count++;
    }
    processing = true;
    Serial.println("Processing Request Info MQTT...");
    StaticJsonDocument<100> docIn;
    deserializeJson(docIn, payload);

    if (docIn == NULL) { //Check for errors in parsing
      Serial.println("Parsing failed");
      char aBuffer[100];
      StaticJsonDocument<100> docOut;
      docOut["status"] = "errorParsingJSON";
      serializeJson(docOut, aBuffer);
      client.publish(esp32Str.c_str(), aBuffer);
    }
    else {
      const char * aName = docIn["id"]; //Get sensor type value
      Serial.print("Device: ");
      Serial.println(aName);

      std::string deviceAddr = "";
      String deviceTopic;

      if (aName != NULL) {
        std::map<std::string, std::string>::iterator itS = allBots.find(aName);
        if (itS != allBots.end())
        {
          deviceAddr = itS->second;
          deviceTopic = buttonStr;
        }
        itS = allCurtains.find(aName);
        if (itS != allCurtains.end())
        {
          deviceAddr = itS->second;
          deviceTopic = curtainStr;
        }
        itS = allMeters.find(aName);
        if (itS != allMeters.end())
        {
          deviceAddr = itS->second;
          deviceTopic = tempStr;
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
        client.publish(esp32Str.c_str(), aBuffer);
      }
    }

    delay(500);
    client.publish(esp32Str.c_str(), "{\"status\":\"idle\"}");
    processing = false;
  });

  client.subscribe(ESPMQTTTopic + "/rescan", [] (const String & payload)  {
    Serial.println("Rescan MQTT Received...");
    if (pScan->isScanning()) {
      Serial.println("Already scanning. Exiting");
      return;
    }
    int count = 0;
    while ((processing) && (count < 600) ) {
      delay(100);
      count++;
    }
    if (pScan->isScanning()) {
      Serial.println("Already scanning. Exiting");
      return;
    }
    processing = true;
    Serial.println("Processing Rescan MQTT...");
    StaticJsonDocument<100> docIn;
    deserializeJson(docIn, payload);

    if (docIn == NULL) { //Check for errors in parsing
      Serial.println("Parsing failed");
      char aBuffer[100];
      StaticJsonDocument<100> docOut;
      docOut["status"] = "errorParsingJSON";
      serializeJson(docOut, aBuffer);
      client.publish(esp32Str.c_str(), aBuffer);
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
          client.publish(esp32Str.c_str(), aBuffer);
        }
      }
    }

    delay(500);
    client.publish(esp32Str.c_str(), "{\"status\":\"idle\"}");
    processing = false;
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
  if (pChr == NULL) {
    return false;
  }
  return pChr->writeValue(bArray, aSize, false);
}

bool sendCommand(NimBLEAdvertisedDevice* advDeviceToUse, const char * type, int attempts) {
  if (advDeviceToUse == NULL) {
    return false;
  }
  Serial.println("Sending command...");
  bool returnValue = true;
  NimBLEClient* pClient = NimBLEDevice::getClientByPeerAddress(advDeviceToUse->getAddress());
  NimBLERemoteService* pSvc = nullptr;
  NimBLERemoteCharacteristic* pChr = nullptr;
  bool tryConnect = !(pClient->isConnected());
  int count = 1;
  if (tryConnect) {
    if (count > 20) {
      Serial.println("Failed to connect for sending command");
      return false;
    }
    count++;
    Serial.println("Attempt to send command. Not connecting. Try connecting...");
    tryConnect = !(connectToServer(advDeviceToUse));
  }

  getGeneric(advDeviceToUse);

  pSvc = pClient->getService("cba20d00-224d-11e6-9fb8-0002a5d5c51b");
  if (pSvc) {
    pChr = pSvc->getCharacteristic("cba20002-224d-11e6-9fb8-0002a5d5c51b");
  }
  if (pChr) {
    if (pChr->canWrite()) {
      bool wasSuccess = false;
      bool isNum = is_number(type);
      if (isNum) {
        int aVal;
        sscanf(type, "%d", &aVal);
        byte bArrayPos[] =  {0x57, 0x0F, 0x45, 0x01, 0x05, 0xFF, aVal};
        wasSuccess = sendCommandBytes(pChr, bArrayPos, 7);
      }
      else {
        if (strcmp(type, "press") == 0) {
          wasSuccess = sendCommandBytes(pChr, bArrayPress, 2);
        }
        else if (strcmp(type, "on") == 0) {
          wasSuccess = sendCommandBytes(pChr, bArrayOn, 3);
        }
        else if (strcmp(type, "off") == 0) {
          wasSuccess = sendCommandBytes(pChr, bArrayOff, 3);
        }
        else if (strcmp(type, "open") == 0) {
          wasSuccess = sendCommandBytes(pChr, bArrayOpen, 7);
        }
        else if (strcmp(type, "close") == 0) {
          wasSuccess = sendCommandBytes(pChr, bArrayClose, 7);
        }
        else if (strcmp(type, "pause") == 0) {
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

bool requestInfo(NimBLEAdvertisedDevice* advDeviceToUse, const char * type, int attempts) {
  return false;
}
