**SwitchBot-MQTT-BLE-ESP32**

Switchbot local control using ESP32. no switchbot hub used/required. works with any smarthub that supports MQTT

https://github.com/devWaves/SwitchBot-MQTT-BLE-ESP32

Code can be installed using Arduino IDE for ESP32
Allows for "unlimited" switchbots devices to be controlled via MQTT sent to ESP32. ESP32 will send BLE commands to switchbots and return MQTT responses to the broker
  ** I do not know where performance will be affected by number of devices
  ** This is an unofficial SwitchBot integration. User takes full responsibility with the use of this code**

v4.1

Created: on June 24 2021
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
 - Retry set/control command on busy response from bot/curtain until success
 - Get settings from bot (firmware, holdSecs, inverted, number of timers)
 - Add a defined delay between each set/control commands or per device
 - ESP32 will collect hold time from bots and automatically wait holdSecs+defaultBotWaitTime until next command is sent to bot
 - Retry on no response curtain or bot
 - holdPress = set bot hold value, then call press (without disconnecting in between)
 - Set/Control is prioritized over scanning. While scanning, if a set/control command is received scanning is stopped and resumed later

\<ESPMQTTTopic\> = \<mqtt_main_topic\>/\<host\>
	
      - Default = switchbot/esp32

**ESP32 will subscribe to MQTT 'set' topic for every configure device**

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
      Integer 0-100 (for setting bot hold seconds) Example: 5           (for bot only) Does the same thing as <ESPMQTTTopic>/setHold

      Strings:
        - "REQUESTINFO" or "GETINFO"                               (for bot and curtain) Does the same thing as calling <ESPMQTTTopic>/requestInfo
        - "REQUESTSETTINGS" or "GETSETTINGS"                       (for bot only) Does the same thing as calling <ESPMQTTTopic>/requestSettings . requires getBotResponse = true
        - "MODEPRESS","MODESWITCH","MODEPRESSINV","MODESWITCHINV"  (for bot only) Does the same thing as <ESPMQTTTopic>/setMode

                      ESP32 will respond with MQTT on 'status' topic for every configured device
                        - <ESPMQTTTopic>/bot/<name>/status
                        - <ESPMQTTTopic>/curtain/<name>/status
                        - <ESPMQTTTopic>/meter/<name>/status
			
                        Example payload:
                          - {"status":"connected"}
                          - {"status":"press"}
                          - {"status":"errorConnect"}
                          - {"status":"errorCommand"}
                          - {"status":"commandSent"}
                          - {"status":"busy", "value":3}
                          - {"status":"failed", "value":9}
                          - {"status":"success", "value":1}
                          - {"status":"success", "value":5}
                          - {"status":"success"}
			  
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


  **ESP32 will Subscribe to MQTT topic to rescan for all device information**
  
      - <ESPMQTTTopic>/rescan

      send a JSON payload of how many seconds you want to rescan for
          example payloads =
            {"sec":30}
            {"sec":"30"}

  **ESP32 will Subscribe to MQTT topic for device information**
  
      - <ESPMQTTTopic>/requestInfo

      send a JSON payload of the device you want to control
          example payloads =
            {"id":"switchbotone"}

                      ESP32 will respond with MQTT on
                        - <ESPMQTTTopic>/#
			
                        Example attribute responses per device are detected:
                          - <ESPMQTTTopic>/bot/<name>/attributes
                          - <ESPMQTTTopic>/curtain/<name>/attributes
                          - <ESPMQTTTopic>/meter/<name>/attributes
			  
                        Example payloads:
                          - {"rssi":-78,"mode":"Press","state":"OFF","batt":94}
                          - {"rssi":-66,"calib":true,"batt":55,"pos":50,"state":"open","light":1}
                          - {"rssi":-66,"scale":"c","batt":55,"C":"21.5","F":"70.7","hum":"65"}
			  
                        Example attribute responses per device are detected:
                          - <ESPMQTTTopic>/bot/<name>/state
                          - <ESPMQTTTopic>/curtain/<name>/state
                          - <ESPMQTTTopic>/meter/<name>/state
			  
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

  **ESP32 will Subscribe to MQTT topic for device settings information (requires getBotResponse = true) - BOT ONLY**
  
      - <ESPMQTTTopic>/requestSettings

    send a JSON payload of the device you want to control
          example payloads =
            {"id":"switchbotone"}

                      ESP32 will respond with MQTT on
                        - <ESPMQTTTopic>/#

                      Example responses per device are detected:
                        - <ESPMQTTTopic>/bot/<name>/settings

                      Example payloads:
                         - {"firmware":4.9,"timers":0,"inverted":false,"hold":5}


  **ESP32 will Subscribe to MQTT topic setting hold time on bots**
  
      - <ESPMQTTTopic>/setHold

    send a JSON payload of the device you want to control
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
                          - {"status":"connected"}
                          - {"status":"errorConnect"}
                          - {"status":"errorCommand"}
                          - {"status":"commandSent"}
                          - {"status":"busy", "value":3}
                          - {"status":"failed", "value":9}
                          - {"status":"success", "value":1}
                          - {"status":"success", "value":5}
                          - {"status":"success"}

 **ESP32 will Subscribe to MQTT topic to holdPress on bots. holdPress = set bot hold value, then call press on bot (without disconnecting in between) **
  
      - <ESPMQTTTopic>/holdPress

    send a JSON payload of the device you want to control
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
                          - {"status":"connected"}
                          - {"status":"errorConnect"}
                          - {"status":"errorCommand"}
                          - {"status":"commandSent"}
                          - {"status":"busy", "value":3}
                          - {"status":"failed", "value":9}
                          - {"status":"success", "value":1}
                          - {"status":"success", "value":5}
                          - {"status":"success"}
	
  **ESP32 will Subscribe to MQTT topic setting mode for bots**
  
      - <ESPMQTTTopic>/setMode

    send a JSON payload of the device you want to control
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
                          - {"status":"connected"}
                          - {"status":"errorConnect"}
                          - {"status":"errorCommand"}
                          - {"status":"commandSent"}
                          - {"status":"busy", "value":3}
                          - {"status":"failed", "value":9}
                          - {"status":"success", "value":1}
                          - {"status":"success", "value":5}

  **ESP32 will respond with MQTT on ESPMQTTTopic with ESP32 status**
  
      - <ESPMQTTTopic>

      example payloads:
        {status":"idle"}
        {status":"scanning"}
        {status":"boot"}


  **Errors that cannot be linked to a specific device will be published to**
  
      - <ESPMQTTTopic>

	
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
