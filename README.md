# ESP32 mqtt client for controlling switches

This is an implementation of a mqqt control switch on ESP32 SoC. Development toolchain is esp-idf v4.1.

- support TLS encryption to remote MQTT server
- support configuration and commands via telnet
- support configuration and commands via uart
- Real time clock synchronization via ntp
- Configuration saved via esp-idf NVS Storage
Wifi parameters, Network parameters, MQTT options are permanently saved in NVS partition, under namespace Syscfg
- suport OTA update via mqtt messages 
Device enters OTA update by sending message 'cmnd/esp32-cam/mqttotastart 1'
Within 20 seconds a message to OTA/DVES_XXXXXX/data must be sent containing the new frmware
After reboot, new firmware must be confirmed sending 'cmnd/esp32-cam/mqttotaconfirm 1', othewise the device performs a rollback on next reboot 
