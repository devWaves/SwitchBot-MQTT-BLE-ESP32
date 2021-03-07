SwitchBot-MQTT-BLE-ESP32:

https://github.com/devWaves/SwitchBot-MQTT-BLE-ESP32

Code can be installed using Arduino IDE for ESP32
Allows for "unlimited" switchbots devices to be controlled via MQTT sent to ESP32. ESP32 will send BLE commands to switchbots and return MQTT responses to the broker
  *** I do not know where performance will be affected by number of devices

v0.3

Created: on March 6 2021
  Author: devWaves

based off of the work from https://github.com/combatistor/ESP32_BLE_Gateway

Notes:
- It works for button press/on/off and should also work for curtain open/close/pause  ( I do not have curtains, so I can't test)
- Good for placing one ESP32 in a zone with 1 or 2 devices that has a bad bluetooth signal from your smart hub. MQTT will use Wifi to "boost" the bluetooth signal
- ESP32 bluetooth is pretty strong and one ESP32 can work for entire house. The code will try around 60 times to connect/push button. It should not need this many but it depends on ESP32 bluetooth signal to switchbots. If one alone doesn't work, get another esp32 and place it in the problem area


ESP32 will Suscribe to MQTT topics
  - switchbotMQTT/press
  - switchbotMQTT/on
  - switchbotMQTT/off
  - switchbotMQTT/open
  - switchbotMQTT/close
  - switchbotMQTT/pause

send a payload of the device you want to control
- example payload = switchbotone

ESP32 will respond with MQTT on
- switchbotMQTT/#

Examples:
- switchbotMQTT/switchbotone/status/
     
Steps
1. Install Arduino IDE
2. Setup IDE for proper ESP32 type
     - Install NimBLEDevice library
     - Install EspMQTTClient library
3. Modify code for your Wifi and MQTT configurations and SwitchBot MAC address
4. Compile and upload to ESP32 (I am using Wemos D1 Mini ESP32)
5. Reboot ESP32 plug it in with 5v usb (no data needed)
