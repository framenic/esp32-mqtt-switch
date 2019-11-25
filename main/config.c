/* config.c
*
* Copyright (c) 2014-2015, Tuan PM <tuanpm at live dot com>
* All rights reserved.
*
* Redistribution and use in source and binary forms, with or without
* modification, are permitted provided that the following conditions are met:
*
* * Redistributions of source code must retain the above copyright notice,
* this list of conditions and the following disclaimer.
* * Redistributions in binary form must reproduce the above copyright
* notice, this list of conditions and the following disclaimer in the
* documentation and/or other materials provided with the distribution.
* * Neither the name of Redis nor the names of its contributors may be used
* to endorse or promote products derived from this software without
* specific prior written permission.
*
* THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
* AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE
* IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE
* ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE
* LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
* CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
* SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
* INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
* CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
* ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF THE
* POSSIBILITY OF SUCH DAMAGE.
*/
//#include "ets_sys.h"
//#include "os_type.h"
//#include "mem.h"
//#include "osapi.h"
//#include "user_interface.h"

#include "stdint.h"
#include "string.h"

#include "nvs_flash.h"
#include "esp_system.h"
#include <esp_log.h>

//#include "mqtt.h"
#include "config.h"
#include "user_config.h"
//#include "debug.h"

SYSCFG sysCfg;

nvs_handle_t nvs_syscfg_h;

SYSCFG myCfg;

static const char *TAG = "config.c";

void CFG_Default()
{
  ESP_LOGI(TAG,"CONFIG: Use default configuration\n");
  
  printf("NVS partition erasing\n");
  // Retry nvs_flash_init
  nvs_erase_all(nvs_syscfg_h);
    
  CFG_Load();
}

void CFG_Defaultkey(char *key)
{
  ESP_LOGI(TAG,"CONFIG: Use default configuration for %s\n",key);
  
  printf("NVS key erasing\n");
  // Retry nvs_flash_init
   nvs_erase_key(nvs_syscfg_h,key);
    
  CFG_Load();
}

void CFG_Save()
{
  if (memcmp(&myCfg, &sysCfg, sizeof(sysCfg))) {
    myCfg = sysCfg;
	
	ESP_LOGI(TAG,"CONFIG: Save configuration\n");
	nvs_set_str(nvs_syscfg_h, "MQTT_CLIENT_ID", sysCfg.device_id);
	nvs_set_str(nvs_syscfg_h, "STA_SSID", sysCfg.sta_ssid);
	nvs_set_str(nvs_syscfg_h, "STA_PASS", sysCfg.sta_pwd);
	
	nvs_set_str(nvs_syscfg_h, "MQTT_HOST", sysCfg.mqtt_host);
	nvs_set_u32(nvs_syscfg_h, "MQTT_PORT", sysCfg.mqtt_port);
	nvs_set_str(nvs_syscfg_h, "MQTT_USER", sysCfg.mqtt_user);
	nvs_set_str(nvs_syscfg_h, "MQTT_PASS", sysCfg.mqtt_pass);
	
	nvs_set_u32(nvs_syscfg_h, "MQTT_KEEPALIVE", sysCfg.mqtt_keepalive);
	nvs_set_u8(nvs_syscfg_h, "MQTT_CLEANSESS", sysCfg.mqtt_cleansession);
	nvs_set_u8(nvs_syscfg_h, "MQTT_SECURITY", sysCfg.security);

	nvs_set_str(nvs_syscfg_h, "MQTT_TOPIC", sysCfg.mqtt_topic);
	nvs_set_str(nvs_syscfg_h, "MQTT_GRPTOPIC", sysCfg.mqtt_grptopic);
	nvs_set_str(nvs_syscfg_h, "MQTT_SUBTOPIC", sysCfg.mqtt_subtopic);
	
	nvs_set_str(nvs_syscfg_h, "OTA_URL", sysCfg.otaUrl);
	
	nvs_set_str(nvs_syscfg_h, "APP_TIMEZONE", sysCfg.timezone);
	nvs_set_u32(nvs_syscfg_h, "APP_PULSE", sysCfg.pulse);
	nvs_set_u8(nvs_syscfg_h, "APP_POWER", sysCfg.power); 
	nvs_set_u8(nvs_syscfg_h, "APP_PWR_AT_RST", sysCfg.power_at_reset);
    nvs_commit(nvs_syscfg_h);
  }	
}




void CFG_Load()
{
	size_t length;
	
	length = sizeof(sysCfg.device_id);
	if (nvs_get_str(nvs_syscfg_h, "MQTT_CLIENT_ID", sysCfg.device_id, &length)!=ESP_OK) {
		uint8_t chipid[6];
		char s_chipid[13];
        esp_efuse_mac_get_default(chipid);
		//sprintf("%02X%02X%02X%02X%02X%02X\n",chipid[0], chipid[1], chipid[2], chipid[3], chipid[4], chipid[5]);
		sprintf(s_chipid,"%02X%02X%02X", chipid[3], chipid[4], chipid[5]);
		sprintf(sysCfg.device_id, MQTT_CLIENT_ID, s_chipid);
	}
	length = sizeof(sysCfg.sta_ssid);
	if (nvs_get_str(nvs_syscfg_h, "STA_SSID", sysCfg.sta_ssid, &length)!=ESP_OK) strcpy(sysCfg.sta_ssid, STA_SSID);
	length = sizeof(sysCfg.sta_pwd);
	if (nvs_get_str(nvs_syscfg_h, "STA_PASS", sysCfg.sta_pwd, &length)!=ESP_OK) strcpy(sysCfg.sta_pwd, STA_PASS);
	length = sizeof(sysCfg.mqtt_host);
	if (nvs_get_str(nvs_syscfg_h, "MQTT_HOST", sysCfg.mqtt_host, &length)!=ESP_OK) strcpy(sysCfg.mqtt_host, MQTT_HOST);
	if (nvs_get_u32(nvs_syscfg_h, "MQTT_PORT", &sysCfg.mqtt_port)!=ESP_OK) sysCfg.mqtt_port=MQTT_PORT;
	length = sizeof(sysCfg.mqtt_user);
	if (nvs_get_str(nvs_syscfg_h, "MQTT_USER", sysCfg.mqtt_user, &length)!=ESP_OK) strcpy(sysCfg.mqtt_user, MQTT_USER);
	length = sizeof(sysCfg.mqtt_pass);
	if (nvs_get_str(nvs_syscfg_h, "MQTT_PASS", sysCfg.mqtt_pass, &length)!=ESP_OK) strcpy(sysCfg.mqtt_pass, MQTT_PASS);
	
	if (nvs_get_u32(nvs_syscfg_h, "MQTT_KEEPALIVE", &sysCfg.mqtt_keepalive)!=ESP_OK) sysCfg.mqtt_keepalive=MQTT_KEEPALIVE;
	if (nvs_get_u8(nvs_syscfg_h, "MQTT_CLEANSESS", &sysCfg.mqtt_cleansession)!=ESP_OK) sysCfg.mqtt_cleansession=MQTT_CLEANSESSION;
	if (nvs_get_u8(nvs_syscfg_h, "MQTT_SECURITY", &sysCfg.security)!=ESP_OK) sysCfg.security=MQTT_SECURITY;

	length = sizeof(sysCfg.mqtt_topic);
	if (nvs_get_str(nvs_syscfg_h, "MQTT_TOPIC", sysCfg.mqtt_topic, &length)!=ESP_OK) strcpy(sysCfg.mqtt_topic, MQTT_TOPIC);
	length = sizeof(sysCfg.mqtt_grptopic);
	if (nvs_get_str(nvs_syscfg_h, "MQTT_GRPTOPIC", sysCfg.mqtt_grptopic, &length)!=ESP_OK) strcpy(sysCfg.mqtt_grptopic, MQTT_GRPTOPIC);
	length = sizeof(sysCfg.mqtt_subtopic);
	if (nvs_get_str(nvs_syscfg_h, "MQTT_SUBTOPIC", sysCfg.mqtt_subtopic, &length)!=ESP_OK) strcpy(sysCfg.mqtt_subtopic, MQTT_SUBTOPIC);
	
	length = sizeof(sysCfg.otaUrl);
	if (nvs_get_str(nvs_syscfg_h, "OTA_URL", sysCfg.otaUrl, &length)!=ESP_OK) strcpy(sysCfg.otaUrl, OTA_URL);
	
	length = sizeof(sysCfg.timezone);
	if (nvs_get_str(nvs_syscfg_h, "APP_TIMEZONE", sysCfg.timezone, &length)!=ESP_OK) strcpy(sysCfg.timezone, APP_TIMEZONE);
	if (nvs_get_u32(nvs_syscfg_h, "APP_PULSE", &sysCfg.pulse)!=ESP_OK) sysCfg.pulse=APP_PULSE;
	if (nvs_get_u8(nvs_syscfg_h, "APP_POWER", &sysCfg.power)!=ESP_OK) sysCfg.power=0; // default off
	if (nvs_get_u8(nvs_syscfg_h, "APP_PWR_AT_RST", &sysCfg.power_at_reset)!=ESP_OK) sysCfg.power_at_reset=2; // default last_state
     //restore previous state or default off/on
    if ((sysCfg.power_at_reset==0)||(sysCfg.power_at_reset==1)) sysCfg.power=sysCfg.power_at_reset;
	
	myCfg = sysCfg;
}

void CFG_Init(){
	// Initialize NVS
    esp_err_t err =  nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        // NVS partition was truncated and needs to be erased
		printf("NVS partition was truncated and needs to be erased\n");
        // Retry nvs_flash_init
        //ESP_ERROR_CHECK(nvs_flash_erase_partition("syscfg"));
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    ESP_ERROR_CHECK( err );
	
	// Open
    printf("Opening Non-Volatile Storage (NVS) handle... ");

    err = nvs_open("syscfg", NVS_READWRITE, &nvs_syscfg_h);
    if (err != ESP_OK) {
        printf("Error (%s) opening NVS handle!\n", esp_err_to_name(err));
    } else {
        printf("Done\n");
    }
}
