/** SwitchBot-MQTT-BLE-ESP32:

  https://github.com/devWaves/SwitchBot-MQTT-BLE-ESP32

  does not use/require switchbot hub

  Code can be installed using Arduino IDE for ESP32
  Allows for "unlimited" switchbots devices to be controlled via MQTT sent to ESP32. ESP32 will send BLE commands to switchbots and return MQTT responses to the broker
     *** I do not know where performance will be affected by number of devices

  v0.4

    Created: on March 7 2021
        Author: devWaves

  based off of the work from https://github.com/combatistor/ESP32_BLE_Gateway

  Notes:
    - It works for button press/on/off

    - It works for curtain open/close/pause/position(%)

    - Good for placing one ESP32 in a zone with 1 or 2 devices that has a bad bluetooth signal from your smart hub. MQTT will use Wifi to "boost" the bluetooth signal

    - ESP32 bluetooth is pretty strong and one ESP32 can work for entire house. The code will try around 60 times to connect/push button. It should not need this many but it depends on ESP32 bluetooth signal to switchbots. If one alone doesn't work, get another esp32 and place it in the problem area

    ESP32 will Suscribe to MQTT topics
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
        {"device":"switchbotone","value":"press"}
        {"device":"switchbotone","value":"open"}
        {"device":"switchbotone","value":"50"}

    ESP32 will respond with MQTT on
      -switchbotMQTT/#

     Example reponses:
       -switchbotMQTT/switchbotone/status
          {"device":"switchbotone","type":"status","description":"connected"}"
          {"device":"switchbotone","type":"status","description":"press"}
          {"device":"switchbotone","type":"status","description":"idle"}"

          {"device":"switchbotone","type":"error","description":"errorConnect"}"
          {"device":"switchbotone","type":"error","description":"errorCommand"}"

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

static std::map<std::string, std::string> allSwitchbots = {
  { "switchbotone", "xx:xx:xx:xx:xx:xx" },
  { "switchbottwo", "yy:yy:yy:yy:yy:yy" }
};

/*************************************************************/

void scanEndedCB(NimBLEScanResults results);
static std::map<std::string, NimBLEAdvertisedDevice*> allSwitchbotsDev = {};
static std::map<std::string, std::string> allSwitchbotsOpp;
static NimBLEScan* pScan;
static bool isScanning;

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
          Serial.println("Found Our Service ... ");
          Serial.println(itS->second.c_str());
          allSwitchbotsDev.insert ( std::pair<std::string, NimBLEAdvertisedDevice*>(advStr, advertisedDevice) );
          Serial.println("Assigned advDevService");
        }
      }
      if (allSwitchbotsDev.size() == allSwitchbots.size()) {
        Serial.println("Stopping Scan found devices ... ");
        NimBLEDevice::getScan()->stop();
      }
    };
};

void scanEndedCB(NimBLEScanResults results) {
  isScanning = false;
  Serial.println("Scan Ended");
}

static ClientCallbacks clientCB;

void setup () {
  client.setMqttReconnectionAttemptDelay(100);
  std::map<std::string, std::string>::iterator it = allSwitchbots.begin();
  while (it != allSwitchbots.end())
  {
    allSwitchbotsOpp.insert ( std::pair<std::string, std::string>(it->second, it->first) );
    it++;
  }
  Serial.begin(115200);
  Serial.println("Starting NimBLE Client");
  NimBLEDevice::init("");
  NimBLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM |*/ BLE_SM_PAIR_AUTHREQ_SC);
  NimBLEDevice::setPower(ESP_PWR_LVL_P9);
  pScan = NimBLEDevice::getScan();
  pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  pScan->setInterval(45);
  pScan->setWindow(15);
  pScan->setActiveScan(true);
  isScanning = true;
  pScan->start(20, scanEndedCB);
}

void loop () {
  client.loop();
}

void processRequest(std::string macAdd, std::string topic, const char * type) {
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
      isScanning = true;
      pScan->start(5 * count, scanEndedCB);
      while (isScanning) {
        Serial.println("Scanning#" + count);
        delay(1000);
      }
      itS = allSwitchbotsDev.find(macAdd);
      advDevice =  itS->second;
      shouldContinue = (advDevice == NULL);
    }
  }
  if (advDevice == NULL)
  {
    StaticJsonDocument<200> doc;
    char aBuffer[256];
    doc["device"] = topic.c_str();
    doc["type"] = "error";
    doc["description"] = "errorLocatingDevice";
    serializeJson(doc, aBuffer);
    String publishStr = ESPMQTTTopic + "/status";
    client.publish(publishStr.c_str(), aBuffer);
    Serial.println();
    delay(500);
    doc["type"] = "status";
    doc["description"] = "idle";
    serializeJson(doc, aBuffer);
    client.publish(publishStr.c_str(),  aBuffer);
    Serial.println();
  }
  else {
    sendToDevice(advDevice, topic, type);
  }
}

void sendToDevice(NimBLEAdvertisedDevice* advDeviceToUse, std::string deviceTopic, const char * type) {
  String publishStr = ESPMQTTTopic + "/status";
  if (advDeviceToUse != NULL)
  {
    bool isConnected = false;
    int count = 0;
    bool shouldContinue = true;
    char aBuffer[256];
    StaticJsonDocument<500> doc;
    doc["device"] = deviceTopic.c_str();
    while (shouldContinue) {
      if (count > 1) {
        delay(100);
      }
      isConnected = connectToServer(advDeviceToUse);
      count++;
      if (isConnected) {
        shouldContinue = false;
        doc["type"] = "status";
        doc["description"] = "connected";
        serializeJson(doc, aBuffer);
        client.publish(publishStr.c_str(),  aBuffer);
      }
      else {
        if (count > 60) {
          shouldContinue = false;
          doc["type"] = "error";
          doc["description"] = "errorConnect";
          serializeJson(doc, aBuffer);
          client.publish(publishStr.c_str(),  aBuffer);
        }
      }
    }
    count = 0;
    if (isConnected) {
      shouldContinue = true;
      bool isSuccess;
      while (shouldContinue) {
        if (count > 1) {
          delay(100);
        }
        isSuccess = sendCommand(advDeviceToUse, type);
        count++;
        if (isSuccess) {
          shouldContinue = false;
          doc["type"] = "status";
          doc["description"] = type;
          serializeJson(doc, aBuffer);
          client.publish(publishStr.c_str(),  aBuffer);
        }
        else {
          if (count > 60) {
            shouldContinue = false;
            doc["type"] = "error";
            doc["description"] = "errorCommand";
            serializeJson(doc, aBuffer);
            client.publish(publishStr.c_str(),  aBuffer);
          }
        }
      }
    }
    delay(500);
    doc["type"] = "status";
    doc["description"] = "idle";
    serializeJson(doc, aBuffer);
    client.publish(publishStr.c_str(),  aBuffer);
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
    while (isScanning) {
      delay(1000);
    }
    String publishStr = ESPMQTTTopic + "/error";
    StaticJsonDocument<200> doc;
    deserializeJson(doc, payload);

    if (doc == NULL) { //Check for errors in parsing
      Serial.println("Parsing failed");
      client.publish(publishStr.c_str(), "errorParsingJSON");
      return;
    }
    const char * deviceTopic = doc["device"]; //Get sensor type value
    const char * value = doc["value"];        //Get value of sensor measurement

    Serial.print("Device: ");
    Serial.println(deviceTopic);
    Serial.print("Device value: ");
    Serial.println(value);

    std::map<std::string, std::string>::iterator itS = allSwitchbots.find(deviceTopic);
    if (itS != allSwitchbots.end())
    {
      bool isNum = is_number(value);

      if (isNum) {
        int aVal;
        sscanf(value, "%d", &aVal);
        if (aVal < 0) {
          value = "0";
        }
        else if (aVal > 100) {
          value = "100";
        }
        processRequest(itS->second, deviceTopic, value);
      }
      else {
        if ((strcmp(value, "press") == 0) || (strcmp(value, "on") == 0) || (strcmp(value, "off") == 0) || (strcmp(value, "open") == 0) || (strcmp(value, "close") == 0) || (strcmp(value, "pause") == 0)) {
          processRequest(itS->second, deviceTopic, value);
        }
        else {
          Serial.println("Parsing failed = value too low/high");
          client.publish(publishStr.c_str(), "errorJSONValue");
        }
      }
    }
    else {
      Serial.println("Parsing failed = device not from list");
      client.publish(publishStr.c_str(), "errorJSONDevice");
    }
  });
}

bool connectToServer(NimBLEAdvertisedDevice* advDeviceToUse) {
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

bool sendCommand(NimBLEAdvertisedDevice* advDeviceToUse, const char * type) {
  if (advDeviceToUse == NULL) {
    return false;
  }
  NimBLEClient* pClient = NimBLEDevice::getClientByPeerAddress(advDeviceToUse->getAddress());
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
          /** Disconnect if write failed */
          pClient->disconnect();
          return false;
        }
      }
    }
    else {
      /** Disconnect if write failed */
      pClient->disconnect();
      return false;
    }
  }
  else {
    Serial.println("CUSTOM write service not found.");
    pClient->disconnect();
    return false;
  }
  Serial.println("Done with this device!");
  return true;
}
