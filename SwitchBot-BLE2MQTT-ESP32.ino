/** SwitchBot-MQTT-BLE-ESP32:

  https://github.com/devWaves/SwitchBot-MQTT-BLE-ESP32

    Code can be installed using Arduino IDE for ESP32
  Allows for 2 switchbots buttons to be controlled via MQTT sent to ESP32. ESP32 will send BLE commands to switchbots and return MQTT responses to the broker

  v0.2

    Created: on March 3 2021
        Author: devWaves

  based off of the work from https://github.com/combatistor/ESP32_BLE_Gateway

  Notes:
    - Good for placing one ESP32 in a zone with 1 or 2 devices that has a bad bluetooth signal from your smart hub. MQTT will use Wifi to "boost" the bluetooth signal

    - This is currently only setup for button PUSH. Code can easily be adapted for on/off. You can use the switchbot app to configure your switchbots, the code can be setup for this but is not currently

    - ESP32 bluetooth is pretty strong and one ESP32 can work for entire house. The code will try around 60 times to connect/push button. It should not need this many but it depends on ESP32 bluetooth signal to switchbots. If one alone doesn't work, get another esp32 and place it in the problem area

*/

#include <NimBLEDevice.h>
#include "EspMQTTClient.h"

/****************** CONFIGURATIONS TO CHANGE *******************/

EspMQTTClient client(
  "SSID",
  "Password",
  "192.168.1.XXX",  // MQTT Broker server ip
  "MQTTUsername",   // Can be omitted if not needed
  "MQTTPassword",   // Can be omitted if not needed
  "ESPMQTT"      // Client name that uniquely identify your device
);
static NimBLEAddress switchBotOneAddress = NimBLEAddress ("XX:XX:XX:XX:XX:XX");
static NimBLEAddress switchBotTwoAddress = NimBLEAddress ("YY:YY:YY:YY:YY:YY");
static String switbotOneTopic = "switchbotone";
static String switbotTwoTopic = "switchbottwo";

/*************************************************************/

void scanEndedCB(NimBLEScanResults results);
static NimBLEAdvertisedDevice* advDeviceSwitchBotOne;
static NimBLEAdvertisedDevice* advDeviceSwitchBotTwo;
static NimBLEScan* pScan;
static bool isScanning;
static uint32_t scanTime = 10; /** 0 = scan forever */

/**  None of these are required as they will be handled by the library with defaults. **
 **                       Remove as you see fit for your needs                        */
class ClientCallbacks : public NimBLEClientCallbacks {
    void onConnect(NimBLEClient* pClient) {
      Serial.println("Connected");
      /** After connection we should change the parameters if we don't need fast response times.
          These settings are 150ms interval, 0 latency, 450ms timout.
          Timeout should be a multiple of the interval, minimum is 100ms.
          I find a multiple of 3-5 * the interval works best for quick response/reconnect.
          Min interval: 120 * 1.25ms = 150, Max interval: 120 * 1.25ms = 150, 0 latency, 60 * 10ms = 600ms timeout
      */
      pClient->updateConnParams(120, 120, 0, 60);
    };

    void onDisconnect(NimBLEClient* pClient) {
    };

    /** Called when the peripheral requests a change to the connection parameters.
        Return true to accept and apply them or false to reject and keep
        the currently used parameters. Default will return true.
    */
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

    /********************* Security handled here **********************
    ****** Note: these are the same return values as defaults ********/
    uint32_t onPassKeyRequest() {
      Serial.println("Client Passkey Request");
      /** return the passkey to send to the server */
      return 123456;
    };

    bool onConfirmPIN(uint32_t pass_key) {
      Serial.print("The passkey YES/NO number: ");
      Serial.println(pass_key);
      /** Return false if passkeys don't match. */
      return true;
    };

    /** Pairing process complete, we can check the results in ble_gap_conn_desc */
    void onAuthenticationComplete(ble_gap_conn_desc* desc) {
      if (!desc->sec_state.encrypted) {
        Serial.println("Encrypt connection failed - disconnecting");
        /** Find the client with the connection handle provided in desc */
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

      if (advertisedDevice->isAdvertisingService(NimBLEUUID("cba20d00-224d-11e6-9fb8-0002a5d5c51b")))
      {
        if (advertisedDevice->getAddress().equals(switchBotOneAddress))
        {
          Serial.println("Found Our Service - switchBotOne");
          /** stop scan before connecting */
          if (advDeviceSwitchBotTwo != NULL) {
            NimBLEDevice::getScan()->stop();
          }
          /** Save the device reference in a global for the client to use*/
          advDeviceSwitchBotOne = advertisedDevice;
          /** Ready to connect now */
        }
        else  if (advertisedDevice->getAddress().equals(switchBotTwoAddress))
        {
          Serial.println("Found Our Service - switchBotTwo");
          /** stop scan before connecting */
          if (advDeviceSwitchBotOne != NULL) {
            NimBLEDevice::getScan()->stop();
          }
          /** Save the device reference in a global for the client to use*/
          advDeviceSwitchBotTwo = advertisedDevice;
          /** Ready to connect now */
        }
      }
    };
};

/** Callback to process the results of the last scan or restart it */
void scanEndedCB(NimBLEScanResults results) {
  isScanning = false;
  Serial.println("Scan Ended");
}

/** Create a single global instance of the callback class to be used by all clients */
static ClientCallbacks clientCB;

void setup () {
  Serial.begin(115200);
  Serial.println("Starting NimBLE Client");
  /** Initialize NimBLE, no device name spcified as we are not advertising */
  NimBLEDevice::init("");
  NimBLEDevice::setSecurityAuth(/*BLE_SM_PAIR_AUTHREQ_BOND | BLE_SM_PAIR_AUTHREQ_MITM |*/ BLE_SM_PAIR_AUTHREQ_SC);
  /** Optional: set the transmit power, default is 3db */
  NimBLEDevice::setPower(ESP_PWR_LVL_P9); /** +9db */
  /** Optional: set any devices you don't want to get advertisments from */
  // NimBLEDevice::addIgnored(NimBLEAddress ("aa:bb:cc:dd:ee:ff"));
  /** create new scan */
  pScan = NimBLEDevice::getScan();
  /** create a callback that gets called when advertisers are found */
  pScan->setAdvertisedDeviceCallbacks(new AdvertisedDeviceCallbacks());
  /** Set scan interval (how often) and window (how long) in milliseconds */
  pScan->setInterval(45);
  pScan->setWindow(15);
  /** Active scan will gather scan response data from advertisers
      but will use more energy from both devices
  */

  pScan->setActiveScan(true);
  /** Start scanning for advertisers for the scan time specified (in seconds) 0 = forever
      Optional callback for when scanning stops.
  */
  isScanning = true;
  pScan->start(20, scanEndedCB);
}

void loop () {
  /** Loop here until we find a device we want to connect to */
  client.loop();
}

void pushButtonOne() {
  int count = 1;
  bool shouldContinue = (advDeviceSwitchBotOne == NULL);
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
      shouldContinue = (advDeviceSwitchBotOne == NULL);
    }
  }
  if (advDeviceSwitchBotOne == NULL)
  {
    client.publish("switchbotMQTT/" + switbotOneTopic + "/status", "errorLocatingDevice");
    delay(500);
    client.publish("switchbotMQTT/" + switbotOneTopic + "/status", "idle");
  }
  else {
    pushButton(advDeviceSwitchBotOne, switbotOneTopic);
  }
}

void pushButtonTwo() {
  int count = 1;
  bool shouldContinue = (advDeviceSwitchBotTwo == NULL);
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
      shouldContinue = (advDeviceSwitchBotTwo == NULL);
    }
  }
  if (advDeviceSwitchBotTwo == NULL)
  {
    client.publish("switchbotMQTT/" + switbotTwoTopic + "/status", "errorLocatingDevice");
    delay(500);
    client.publish("switchbotMQTT/" + switbotTwoTopic + "/status", "idle");
  }
  else {
    pushButton(advDeviceSwitchBotTwo, switbotTwoTopic);
  }
}

void pushButton(NimBLEAdvertisedDevice* advDeviceToUse, String deviceTopic) {
  if (advDeviceToUse != NULL)
  {
    bool isConnected = false;
    int count = 0;
    bool shouldContinue = true;
    while (shouldContinue) {
      if (count > 1) {
        delay(100);
      }
      isConnected = connectToServer(advDeviceToUse);
      count++;
      if (isConnected) {
        shouldContinue = false;
        client.publish("switchbotMQTT/" + deviceTopic + "/status", "connected");
      }
      else {
        if (count > 60) {
          shouldContinue = false;
          client.publish("switchbotMQTT/" + deviceTopic + "/status", "errorConnect");
        }
      }
    }
    count = 0;
    if (isConnected) {
      shouldContinue = true;
      bool isPushed;
      while (shouldContinue) {
        if (count > 1) {
          delay(100);
        }
        isPushed = sendButtonPush(advDeviceToUse);
        count++;
        if (isPushed) {
          shouldContinue = false;
          client.publish("switchbotMQTT/" + deviceTopic + "/status", "pushed");
        }
        else {
          if (count > 60) {
            shouldContinue = false;
            client.publish("switchbotMQTT/" + deviceTopic + "/status", "errorPush");
          }
        }
      }
    }
    delay(500);
    client.publish("switchbotMQTT/" + deviceTopic + "/status", "idle");
  }
}

void onConnectionEstablished() {
  client.subscribe("switchbotMQTT/" + switbotOneTopic + "/push", [] (const String & payload)  {
    while (isScanning) {
      delay(1000);
    }
    Serial.println("Got " + switbotOneTopic + " MQTT push");
    pushButtonOne();
  });
  client.subscribe("switchbotMQTT/" + switbotTwoTopic + "/push", [] (const String & payload)  {
    while (isScanning) {
      delay(1000);
    }
    Serial.println("Got " + switbotTwoTopic + " MQTT push");
    pushButtonTwo();
  });
}

bool connectToServer(NimBLEAdvertisedDevice* advDeviceToUse) {
  NimBLEClient* pClient = nullptr;

  /** Check if we have a client we should reuse first **/
  if (NimBLEDevice::getClientListSize()) {
    /** Special case when we already know this device, we send false as the
        second argument in connect() to prevent refreshing the service database.
        This saves considerable time and power.
    */
    pClient = NimBLEDevice::getClientByPeerAddress(advDeviceToUse->getAddress());
    if (pClient) {
      if (!pClient->connect(advDeviceToUse, false)) {
        Serial.println("Reconnect failed");
      }
      else {
        Serial.println("Reconnected client");
      }
    }
    /** We don't already have a client that knows this device,
        we will check for a client that is disconnected that we can use.
    */
    else {
      pClient = NimBLEDevice::getDisconnectedClient();
    }
  }

  /** No client to reuse? Create a new one. */
  if (!pClient) {
    if (NimBLEDevice::getClientListSize() >= NIMBLE_MAX_CONNECTIONS) {
      Serial.println("Max clients reached - no more connections available");
      return false;
    }

    pClient = NimBLEDevice::createClient();

    Serial.println("New client created");

    pClient->setClientCallbacks(&clientCB, false);
    /** Set initial connection parameters: These settings are 15ms interval, 0 latency, 120ms timout.
        These settings are safe for 3 clients to connect reliably, can go faster if you have less
        connections. Timeout should be a multiple of the interval, minimum is 100ms.
        Min interval: 12 * 1.25ms = 15, Max interval: 12 * 1.25ms = 15, 0 latency, 51 * 10ms = 510ms timeout
    */
    pClient->setConnectionParams(12, 12, 0, 51);
    /** Set how long we are willing to wait for the connection to complete (seconds), default is 30. */
    pClient->setConnectTimeout(10);

  }

  if (!pClient->isConnected()) {
    if (!pClient->connect(advDeviceToUse)) {
      /** Created a client but failed to connect, don't need to keep it as it has no data */
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

//Currently not used
bool registerNotify(NimBLEAdvertisedDevice* advDeviceToUse) {
  /** Assumes device is already connected*/
  NimBLEClient* pClient = NimBLEDevice::getClientByPeerAddress(advDeviceToUse->getAddress());
  /** Now we can read/write/subscribe the charateristics of the services we are interested in */
  NimBLERemoteService* pSvc = nullptr;
  NimBLERemoteCharacteristic* pChr = nullptr;

  pSvc = pClient->getService("cba20d00-224d-11e6-9fb8-0002a5d5c51b"); // custom device service

  if (pSvc) {    /** make sure it's not null */
    pChr = pSvc->getCharacteristic("cba20003-224d-11e6-9fb8-0002a5d5c51b"); // custom characteristic to notify
  }

  if (pChr) {    /** make sure it's not null */
    if (pChr->canNotify()) {
      //if(!pChr->registerForNotify(notifyCB)) {
      if (!pChr->subscribe(true, notifyCB)) {
        /** Disconnect if subscribe failed */
        pClient->disconnect();
        return false;
      }
    }
  }
  else {
    Serial.println("CUSTOM notify service not found.");
  }
}

//Currently not used
bool getSettings(NimBLEAdvertisedDevice* advDeviceToUse) {
  /** Assumes device is already connected*/
  NimBLEClient* pClient = NimBLEDevice::getClientByPeerAddress(advDeviceToUse->getAddress());
  /** Now we can read/write/subscribe the charateristics of the services we are interested in */
  NimBLERemoteService* pSvc = nullptr;
  NimBLERemoteCharacteristic* pChr = nullptr;

  pSvc = pClient->getService("cba20d00-224d-11e6-9fb8-0002a5d5c51b"); // custom device service
  if (pSvc) {    /** make sure it's not null */
    pChr = pSvc->getCharacteristic("cba20002-224d-11e6-9fb8-0002a5d5c51b"); // custom characteristic to write
  }
  if (pChr) {    /** make sure it's not null */
    if (pChr->canWrite()) {
      byte bArray[] = {0x57, 0x02}; // write to get settings of device
      if (pChr->writeValue(bArray, 2)) {
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
    Serial.println("CUSTOM write service not found.");
    pClient->disconnect();
    return false;
  }
}

//Currently not used
bool getDeviceInfo(NimBLEAdvertisedDevice* advDeviceToUse) {
  /** Assumes device is already connected*/
  if (advDeviceToUse == NULL) {
    return false;
  }
  NimBLEClient* pClient = NimBLEDevice::getClientByPeerAddress(advDeviceToUse->getAddress());
  /** Now we can read/write/subscribe the charateristics of the services we are interested in */
  NimBLERemoteService* pSvc = nullptr;
  NimBLERemoteCharacteristic* pChr = nullptr;

  pSvc = pClient->getService((uint16_t) 0x1800); // GENERIC ACCESS service
  if (pSvc) {    /** make sure it's not null */
    pChr = pSvc->getCharacteristic((uint16_t) 0x2a00); // DEVICE NAME characteristic
  }
  if (pChr) {    /** make sure it's not null */
    if (pChr->canRead()) {
      Serial.print(pChr->getUUID().toString().c_str());
      Serial.print(" Value: ");
      Serial.println(pChr->readValue().c_str()); // should return WoHand
    }
  }
  else {
    Serial.println("GENERIC ACCESS service not found.");
    pClient->disconnect();
    return false;
  }
}

bool sendButtonPush(NimBLEAdvertisedDevice* advDeviceToUse) {
  /** Assumes device is already connected*/
  if (advDeviceToUse == NULL) {
    return false;
  }
  NimBLEClient* pClient = NimBLEDevice::getClientByPeerAddress(advDeviceToUse->getAddress());
  /** Now we can read/write/subscribe the charateristics of the services we are interested in */
  NimBLERemoteService* pSvc = nullptr;
  NimBLERemoteCharacteristic* pChr = nullptr;

  pSvc = pClient->getService("cba20d00-224d-11e6-9fb8-0002a5d5c51b"); // custom device service

  if (pSvc) {    /** make sure it's not null */
    pChr = pSvc->getCharacteristic("cba20002-224d-11e6-9fb8-0002a5d5c51b"); // custom characteristic to write
  }

  if (pChr) {    /** make sure it's not null */
    if (pChr->canWrite()) {
      byte bArray[] = {0x57, 0x01}; // write to get trigger the device (press mode)
      //byte bArray[] = {0x57, 0x01, 0x01}; // write to get trigger the device (switch mode ON)
      //byte bArray[] = {0x57, 0x01, 0x02}; // write to get trigger the device (switch mode OFF)
      if (pChr->writeValue(bArray, 2)) {
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
    Serial.println("CUSTOM write service not found.");
    pClient->disconnect();
    return false;
  }
  Serial.println("Done with this device!");
  return true;
}

/** Notification / Indication receiving handler callback */
void notifyCB(NimBLERemoteCharacteristic* pRemoteCharacteristic, uint8_t* pData, size_t length, bool isNotify) {
  std::string str = (isNotify == true) ? "Notification" : "Indication";
  str += " from ";
  /** NimBLEAddress and NimBLEUUID have std::string operators */
  str += std::string(pRemoteCharacteristic->getRemoteService()->getClient()->getPeerAddress());
  str += ": Service = " + std::string(pRemoteCharacteristic->getRemoteService()->getUUID());
  str += ", Characteristic = " + std::string(pRemoteCharacteristic->getUUID());

  //    str += ", Size = " ;
  //    char strLength[sizeof(size_t)]; /* In fact not necessary as snprintf() adds the 0-terminator. */
  //    snprintf(strLength, sizeof strLength, "%zu", length);
  //    str += strLength;

  //str += ", Value = " + std::string((char*)pData, length);
  str += ", Value = ";
  char hexFormatted[length * 2] = "";
  for (int i = 0; i < length; i++) {
    sprintf( hexFormatted, "%s%02X", hexFormatted, pData[i]);
  }
  str += hexFormatted;
  Serial.println(str.c_str());
  if (pData[0] == 1) {
    int battLevel = pData[1];
    Serial.print("Battery level: ");
    Serial.println(battLevel);

    float fwVersion = pData[2] / 10.0;
    Serial.print("Fw version: ");
    Serial.println(fwVersion);

    int timersNumber = pData[8];
    Serial.print("Number of timers: ");
    Serial.println(timersNumber);

    bool dualStateMode = bitRead(pData[9], 4) ;
    str = (dualStateMode == true) ? "Switch mode" : "Press mode";
    Serial.println(str.c_str());

    bool inverted = bitRead(pData[9], 0) ;
    str = (inverted == true) ? "Inverted" : "Not inverted";
    Serial.println(str.c_str());

    int holdSecs = pData[10];
    Serial.print("Hold seconds: ");
    Serial.println(holdSecs);
  }
}
