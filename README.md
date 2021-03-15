**SwitchBot-MQTT-BLE-ESP32**

Switchbot local control using ESP32. no switchbot hub used/required. works with any smarthub that supports MQTT

https://github.com/devWaves/SwitchBot-MQTT-BLE-ESP32

Code can be installed using Arduino IDE for ESP32
Allows for "unlimited" switchbots devices to be controlled via MQTT sent to ESP32. ESP32 will send BLE commands to switchbots and return MQTT responses to the broker
  *** I do not know where performance will be affected by number of devices

v0.11

Created: on March 14 2021
  Author: devWaves

based off of the work from https://github.com/combatistor/ESP32_BLE_Gateway

Notes:
 - It works for button press/on/off
 - It works for curtain open/close/pause/position(%)
 - It is setup to return values from curtain and temp sensosr but could still be issues with values returned. I don't own curtains or tempsensors to test
 - It can request button setting values (battery, mode, firmware version, Number of timers, Press mode, inverted (yes/no), Hold seconds)
 - Good for placing one ESP32 in a zone with 1 or 2 devices that has a bad bluetooth signal from your smart hub. MQTT will use Wifi to "boost" the bluetooth signal
 - ESP32 bluetooth is pretty strong and one ESP32 can work for entire house. The code will try around 60 times to connect/push button. It should not need this many but it depends on ESP32 bluetooth signal to switchbots. If one alone doesn't work, get another esp32 and place it in the problem area

**ESP32 will Suscribe to MQTT topic for control...**
- switchbotMQTT/control

send a JSON payload of the device you want to control (device = device to control) (value = string value)
Value can equal...
- "press"
- "on"
- "off"
- "open"
- "close"
- "pause"
- any number 0-100 (for curtain position) Example: "50"

example payloads
- {"id":"switchbotone","value":"press"}
- {"id":"switchbotone","value":"open"}
- {"id":"switchbotone","value":"50"}
  
**ESP32 will respond with MQTT on...**
- switchbotMQTT/#

Example reponses:
switchbotMQTT/status
- {"id":"switchbotone","status":"connected"}
- {"id":"switchbotone","status":"press"}
- {"id":"switchbotone","status":"idle"}
- {"id":"switchbotone","status":"errorConnect"}
- {"id":"switchbotone","status":"errorCommand"}


**ESP32 will Suscribe to MQTT topic for device information...**
- switchbotMQTT/requestInfo

  send a JSON payload of the device you want to control
   example payloads =
   - {"id":"switchbotone"}
      
**ESP32 will respond with MQTT on...**
- switchbotMQTT/#

  Example reponses:
  switchbotMQTT/status
  example payloads =
  - {"id":"switchbottwo","status":"info","rssi":-78,"mode":"Press","state":"OFF","batt":94}

Steps
1. Install Arduino IDE
2. Setup IDE for proper ESP32 type
     https://randomnerdtutorials.com/installing-the-esp32-board-in-arduino-ide-windows-instructions/
3. Install NimBLEDevice library
4. Install EspMQTTClient library
5. Install ArduinoJson library
6. Modify code for your Wifi and MQTT configurations and SwitchBot MAC address
7. Compile and upload to ESP32 (I am using Wemos D1 Mini ESP32)
8. Reboot ESP32 plug it in with 5v usb (no data needed)
