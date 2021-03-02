# SwitchBot-MQTT-BLE-ESP32

	https://github.com/devWaves/SwitchBot-MQTT-BLE-ESP32
	
  Code can be installed using Arduino IDE for ESP32
	Allows for 2 switchbots buttons to be controlled via MQTT sent to ESP32. ESP32 will send BLE commands to switchbots and return MQTT responses to the broker
	
	v0.1

    Created: on March 1 2021
        Author: devWaves

	based off of the work from https://github.com/combatistor/ESP32_BLE_Gateway
	
	Notes: 
		- Good for placing one ESP32 in a zone with 1 or 2 devices that has a bad bluetooth signal from your smart hub. MQTT will use Wifi to "boost" the bluetooth signal
	
		- This is currently only setup for button PUSH. Code can easily be adapted for on/off. You can use the switchbot app to configure your switchbots, the code can be setup for this but is not currently 
	
		- ESP32 bluetooth is pretty strong and one ESP32 can work for entire house. The code will try around 60 times to connect/push button. It should not need this many but it depends on ESP32 bluetooth signal to switchbots. If one alone doesn't work, get another esp32 and place it in the problem area

  - Code can be adapted to support more than 2 switchbots (or only 1), 2 just worked for what I needed. Code can also be updated to support battery MQTT update, which I will     probably do later

Steps

1. Install Arduino IDE
2. Setup IDE for proper ESP32 type
3. Modify code for your Wifi and MQTT configurations and SwitchBot MAC address
4. Compile and upload to ESP32 (I am using Wemos D1 Mini ESP32)
5. Reboot ESP32
6. To push the button, send any MQTT message payload switchbotMQTT/switchbotone/push    or  switchbotMQTT/switchbottwo/push
7. Watch for MQTT traffic on switchbotMQTT/#
