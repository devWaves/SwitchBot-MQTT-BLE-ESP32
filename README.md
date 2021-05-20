**SwitchBot-MQTT-BLE-ESP32**

Switchbot local control using ESP32. no switchbot hub used/required. works with any smarthub that supports MQTT

https://github.com/devWaves/SwitchBot-MQTT-BLE-ESP32

Code can be installed using Arduino IDE for ESP32
Allows for "unlimited" switchbots devices to be controlled via MQTT sent to ESP32. ESP32 will send BLE commands to switchbots and return MQTT responses to the broker
  ** I do not know where performance will be affected by number of devices
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
 - Good for placing one ESP32 in a zone with 1 or 2 devices that has a bad bluetooth signal from your smart hub. MQTT will use Wifi to "boost" the bluetooth signal
 - ESP32 bluetooth is pretty strong and one ESP32 can work for entire house. The code will try around 60 times to connect/push button. It should not need this many but it depends on ESP32 bluetooth signal to switchbots. If one alone doesn't work, get another esp32 and place it in the problem area
 - OTA update added. Go to ESP32 IP address in browser. In Arduino IDE menu - Sketch / Export compile Binary . Upload the .bin file
 - Support for bot passwords
 - Automatically rescan every X seconds
 - Automatically requestInfo X seconds after successful control command

**ESP32 will Subscribe to MQTT topic for control...**
 - [ESPMQTTTopic]/control

**ESP32 will subscribe to MQTT 'set' topic for every configure device.**
 - [ESPMQTTTopic]/bot/[name]/set
 - [ESPMQTTTopic]/curtain/[name]/set
 - [ESPMQTTTopic]/meter/[name]/set

Send a payload to the 'set' topic of the device you want to control
 - "PRESS"
 - "ON"
 - "OFF"
 - "OPEN"
 - "CLOSE"
 - "PAUSE"
 - 0-100 (for curtain position) Example: 50
  
**ESP32 will respond with MQTT on 'status' topic for every configured device**
 - [ESPMQTTTopic]/bot/[name]/status
 - [ESPMQTTTopic]/curtain/[name]/status
 - [ESPMQTTTopic]/meter/[name]/status

Example reponses:
 - [ESPMQTTTopic]/bot/[name]/status
 - [ESPMQTTTopic]/curtain/[name]/status
 - [ESPMQTTTopic]/meter/[name]/status

Example payload:
 - {"status":"connected"}
 - {"status":"press"}
 - {"status":"errorConnect"}
 - {"status":"errorCommand"}
 - {"status":"commandSent"}

**ESP32 will respond with MQTT on esp32Topic with ESP32 status**
 - [ESPMQTTTopic]/ESP32

Example payload:
 - {status":"idle"}

**ESP32 will Subscribe to MQTT topic to rescan for all device information...**
 - [ESPMQTTTopic]/rescan

  send a JSON payload of how many seconds you want to rescan for
   example payloads =
   - {"sec":"30"}

**ESP32 will Subscribe to MQTT topic for single device status update...**
 - [ESPMQTTTopic]/requestInfo

  send a JSON payload of which device you want status from
   example payloads =
   - {"id":"switchbotone"}

**ESP32 will respond with MQTT on...**
 - [ESPMQTTTopic]/#

Example reponses as device are detected:
 - [ESPMQTTTopic]/bot/[name]/attributes
 - [ESPMQTTTopic]/curtain/[name]/attributes
 - [ESPMQTTTopic]/meter/[name]/attributes
	Example payloads: 
	 - {"rssi":-78,"mode":"Press","state":"OFF","batt":94}
	 - {"rssi":-66,"calib":true,"batt":55,"pos":50,"state":"open","light":1}
	 - {"rssi":-66,"scale":"c","batt":55,"C":"21.5","F":"70.7","hum":"65"}
	 
 - [ESPMQTTTopic]/bot/[name]/state
 - [ESPMQTTTopic]/curtain/[name]/state
 - [ESPMQTTTopic]/meter/[name]/state
	Example payloads: 
	 - ON
	 - OFF
	 - OPEN
	 - CLOSE
	 - 50

Errors that cannot be linked to a specific device will be published to
 - [ESPMQTTTopic]/ESP32


<strong>Steps to Install on ESP32</strong>
1. Install Arduino IDE
2. Setup IDE for proper ESP32 type
     https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/
3. Install NimBLEDevice library
4. Install EspMQTTClient library
5. Install ArduinoJson library
6. Install CRC32 library (by Christopher Baker)
7. Install ArduinoQueue library
8. Modify code for your Wifi and MQTT configurations and SwitchBot MAC address
9. Compile and upload to ESP32 (I am using Wemos D1 Mini ESP32)
10. Reboot ESP32 plug it in with 5v usb (no data needed)
