# ESP32 mqtt client for controlling switches

This is an implementation of a mqqt control switch on ESP32 SoC. Development toolchain is esp-idf v4.1.

- support TLS encryption to remote MQTT server
- support configuration and commands via telnet
- support configuration and commands via uart
- Real time clock synchronization via ntp
- Configuration saved via esp-idf NVS Storage
Wifi parameters, Network parameters, MQTT options are permanently saved in NVS partition, under namespace Syscfg
- support OTA updates via http protocol 
- support OTA update via mqtt messages
- support wifi AP mode for initial configuration via http (default address 192.168.1.1)

Button functions:
- 1 short press
	toggles power on/off
- 4 short press
	start ap mode, led fast flash
- long press (>4s)
	factory reset, restore flash and data

Device enters OTA update by sending message 'cmnd/esp32-cam/mqttotastart 1'

Within 20 seconds a message to OTA/DVES_XXXXXX/data must be sent containing the new frmware

After reboot, new firmware must be confirmed sending 'cmnd/esp32-cam/mqttotaconfirm 1', othewise the device performs a rollback on next reboot

Example:


mosquitto_pub -h mqtt_server_ip -u mqtt_user -P mqtt_password -t cmnd/esp32-cam/mqttotastart -m 1

mosquitto_pub -h mqtt_server_ip -u mqtt_user -P mqtt_password -t OTA/DVES_XXXXXX/data -f build/esp32cam-mqtt.bin

After reboot:

mosquitto_pub -h mqtt_server_ip -u mqtt_user -P mqtt_password -t cmnd/esp32-cam/mqttotaconfirm -m 1
