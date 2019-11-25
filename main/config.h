/* config.h
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

#ifndef USER_CONFIG_H_
#define USER_CONFIG_H_
//#include "os_type.h"
#include "user_config.h"
typedef struct{
//	uint32_t cfg_holder;
	char device_id[32];

	char sta_ssid[64];
	char sta_pwd[64];
	uint32_t sta_type;

	char mqtt_host[64];
	uint32_t mqtt_port;
	char mqtt_user[32];
	char mqtt_pass[32];
	uint32_t mqtt_keepalive;
	uint8_t mqtt_cleansession;
	uint8_t security;
	
	char mqtt_grptopic[32];
    char mqtt_topic[32];
    char otaUrl[80];

    char mqtt_subtopic[32];
    char timezone[64];
    uint8_t power;
	uint8_t power_at_reset; // 0:off 1:on 2:last state
	uint32_t pulse;
	
} SYSCFG;

void CFG_Init();
void CFG_Save();
void CFG_Load();
void CFG_Default();
void CFG_Defaultkey(char *key);

extern SYSCFG sysCfg;

#endif /* USER_CONFIG_H_ */
