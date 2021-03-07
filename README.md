**SwitchBot-MQTT-BLE-ESP32**

Switchbot local control using ESP32. no switchbot hub used/required. works with any smarthub that supports MQTT

https://github.com/devWaves/SwitchBot-MQTT-BLE-ESP32

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

**ESP32 will Suscribe to MQTT topics...**
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
- {"device":"switchbotone","value":"press"}
- {"device":"switchbotone","value":"open"}
- {"device":"switchbotone","value":"50"}
  
**ESP32 will respond with MQTT on...**
- switchbotMQTT/#

Example reponses:
switchbotMQTT/switchbotone/status
- {"device":"switchbotone","type":"status","description":"connected"}
- {"device":"switchbotone","type":"status","description":"press"}
- {"device":"switchbotone","type":"status","description":"idle"}
- {"device":"switchbotone","type":"error","description":"errorConnect"}
- {"device":"switchbotone","type":"error","description":"errorCommand"}
          
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
