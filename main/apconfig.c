

#include <string.h>
#include "esp_system.h"
#include <esp_wifi.h>
#include "user_config.h"
#include "mqtt.h"


// stop all functions and switch to AP mode
void apconfig_init()
{
	mqtt_setlower_state(STATE_RESET);
	esp_wifi_stop();
	
	
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_stop(TCPIP_ADAPTER_IF_AP));
	tcpip_adapter_ip_info_t info;
	bzero(&info, sizeof(info));
	//memset(&info, 0, sizeof(info));
	IP4_ADDR(&info.ip, 192, 168, 1, 1);
	IP4_ADDR(&info.gw, 192, 168, 1, 1);
	IP4_ADDR(&info.netmask, 255, 255, 255, 0);
	ESP_ERROR_CHECK(tcpip_adapter_set_ip_info(TCPIP_ADAPTER_IF_AP, &info));
	ESP_ERROR_CHECK(tcpip_adapter_dhcps_start(TCPIP_ADAPTER_IF_AP));
	
	
	uint8_t chipid[6];
	char s_chipid[13];
	char ap_ssid[32];
	
    esp_efuse_mac_get_default(chipid);
	sprintf(s_chipid,"%02X%02X%02X", chipid[3], chipid[4], chipid[5]);
	sprintf(ap_ssid, MQTT_CLIENT_ID, s_chipid);
	
	wifi_config_t wifi_config = {
		.ap = {
			.max_connection = 1,
			.authmode = WIFI_AUTH_OPEN
		}
	};
	memcpy(wifi_config.ap.ssid, ap_ssid, strlen(ap_ssid));

	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_AP));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_AP, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

}