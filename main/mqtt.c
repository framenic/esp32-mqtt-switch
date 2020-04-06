#include <esp_log.h>

#include "mqtt_client.h"
#include "mqtt.h"
#include "config.h"
#include "user_config.h"
#include <esp_ota_ops.h>
#include "esp_partition.h"
#include "rtc_sntp.h"
#include "comm_queue.h"
#include "telnet.h"
#include "board_gpio.h"
#include "ota_mqtt.h"
#include "ota_http.h"
#include "sled.h"
#include "apconfig.h"

#include "freertos/FreeRTOS.h"
#include "freertos/timers.h"

static const char *TAG = "mqtt.c";

#define STATES        10
uint8_t state = 0;
uint8_t otaflag = 0;
uint16_t mqttotaflag = 0;
uint8_t restartflag = 0;
uint8_t smartconfigflag = 0;
uint8_t factoryrestoreflag = 0;
uint16_t heartbeat = 0;

uint8_t mqttretry = 0;

uint8_t lastbutton = NOT_PRESSED;
uint8_t holdcount = 0;
uint8_t multiwindow = 0;
uint8_t multipress = 0;
//uint16_t blinks = 0;
//uint8_t blinkstate = 0;

uint32_t pulse = 0;
uint32_t timer = 0;

bool mqtt_connected = false;

uint8_t device_state = STATE_RESET;


void update_outputs_state()
{
	switch (device_state) {
			case STATE_RESET:
				set_sled_state(SLED_ON);
				break;
			case STATE_WIFI_CONNECTING:
				set_sled_state(SLED_SLOW_BLINK);
				break;
			case STATE_WIFI_CONNECTED:
			case STATE_MQTT_CONNECTING:
				set_sled_state(SLED_SLOW_FLASH);
				break;
			case STATE_MQTT_CONNECTED:
				set_sled_state(SLED_OFF);
				break;
			default: break;
		}
}

void mqtt_setlower_state(uint8_t state)
{
	if (state<device_state) {
		device_state = state;
		update_outputs_state();
	}
}

void mqtt_sethigher_state(uint8_t state)
{
	if (state>device_state) {
		device_state = state;
		update_outputs_state();		
	}
}

uint8_t mqtt_get_state()
{
	return device_state;
}

void send_mqttota_off(esp_mqtt_client_handle_t client)
{
	char stopic[80], svalue[20];

	sprintf(stopic, "%s/%s/%s", PUB_PREFIX, sysCfg.mqtt_topic, "mqttotastart");
	strcpy(svalue, "off");
	//MQTT_Publish_Tx(&mqttClient, stopic, svalue, strlen(svalue), 1, 0); 
	 if (mqtt_connected) esp_mqtt_client_publish(client, stopic, svalue, strlen(svalue), 1, 0);
	
	printf("%s = %s\n", strrchr(stopic,'/')+1, svalue);
	telnetSend(strrchr(stopic,'/')+1, strlen(strrchr(stopic,'/')+1));
	telnetSend(" = ", 3);
	telnetSend(svalue, strlen(svalue));
	telnetSend("\n", 1);
}


void send_power(esp_mqtt_client_handle_t client)
{
	char stopic[80], svalue[20];

	sprintf(stopic, "%s/%s/%s", PUB_PREFIX, sysCfg.mqtt_topic, sysCfg.mqtt_subtopic);
	strcpy(svalue, (sysCfg.power == 0) ? "off" : "on");
	//MQTT_Publish_Tx(&mqttClient, stopic, svalue, strlen(svalue), 1, 1); //retain
	 if (mqtt_connected) esp_mqtt_client_publish(client, stopic, svalue, strlen(svalue), 1, 1);
	
	printf("%s = %s\n", strrchr(stopic,'/')+1, svalue);
	telnetSend(strrchr(stopic,'/')+1, strlen(strrchr(stopic,'/')+1));
	telnetSend(" = ", 3);
	telnetSend(svalue, strlen(svalue));
	telnetSend("\n", 1);
}

void send_timer(esp_mqtt_client_handle_t client)
{
	char stopic[80], svalue[20];

	sprintf(stopic, "%s/%s/%s", PUB_PREFIX, sysCfg.mqtt_topic, "timer");
	sprintf(svalue, "%d", timer);
	//MQTT_Publish_Tx(&mqttClient, stopic, svalue, strlen(svalue), 0, 1); //retain
	 if (mqtt_connected) esp_mqtt_client_publish(client, stopic, svalue, strlen(svalue), 0, 1);
	  
	printf("%s = %s\n", strrchr(stopic,'/')+1, svalue);
	telnetSend(strrchr(stopic,'/')+1, strlen(strrchr(stopic,'/')+1));
	telnetSend(" = ", 3);
	telnetSend(svalue, strlen(svalue));
	telnetSend("\n", 1);
}


void mqttConnectedCb(esp_mqtt_client_handle_t client)
{
  char stopic[60], svalue[40];
  int msg_id;
  
  sprintf(stopic, "%s/%s/#", SUB_PREFIX, sysCfg.mqtt_topic);
  //MQTT_Subscribe(client, stopic, 1);
  msg_id = esp_mqtt_client_subscribe(client, stopic, 1);
  ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
  
  sprintf(stopic, "%s/%s/#", SUB_PREFIX, sysCfg.mqtt_grptopic);
  //MQTT_Subscribe(client, stopic, 1);
  msg_id = esp_mqtt_client_subscribe(client, stopic, 1);
  ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
  
  sprintf(stopic, "%s/%s/#", SUB_PREFIX, sysCfg.device_id);  // Fall back topic
  //MQTT_Subscribe(client, stopic, 0);
  msg_id = esp_mqtt_client_subscribe(client, stopic, 1);
  ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
  
  sprintf(stopic, "%s/%s/NAME", PUB_PREFIX, sysCfg.mqtt_topic);
  sprintf(svalue, PROJECT);
  //MQTT_Publish_Tx(client, stopic, svalue, strlen(svalue), 0, 0);
  msg_id = esp_mqtt_client_publish(client, stopic, svalue, 0, 0, 0);
  ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
  
  sprintf(stopic, "%s/%s/VERSION", PUB_PREFIX, sysCfg.mqtt_topic);
  sprintf(svalue, "%s", esp_ota_get_app_description()->version);
  //MQTT_Publish_Tx(client, stopic, svalue, strlen(svalue), 0, 0);
  msg_id = esp_mqtt_client_publish(client, stopic, svalue, 0, 0, 0);
  ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
    
  sprintf(stopic, "%s/%s/FALLBACKTOPIC", PUB_PREFIX, sysCfg.mqtt_topic);
  sprintf(svalue, "%s", sysCfg.device_id);
  //MQTT_Publish_Tx(client, stopic, svalue, strlen(svalue), 0, 0);
  msg_id = esp_mqtt_client_publish(client, stopic, svalue, 0, 0, 0);
  ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
  
  // send PUB_PREFIX/sysCfg.mqtt_topic/power and timer at startup
  send_power(client);
  send_timer(client);
}

void mqttDataCb(esp_mqtt_client_handle_t client, const char* topic, uint32_t topic_len, const char *data, uint32_t data_len)
{
  uint8_t i, grpflg = 0;
  char *str, *p, *mtopic = NULL, *type = NULL;
  char stopic[80], svalue[300];
 
  char *topicBuf = (char*)malloc(topic_len+1),
       *dataBuf = (char*)malloc(data_len+1),
       *dataBufLc = (char*)malloc(data_len+1);

  memcpy(topicBuf, topic, topic_len);
  topicBuf[topic_len] = 0;
  i = 0;
  for (str = strtok_r(topicBuf, "/", &p); str && i < 3; str = strtok_r(NULL, "/", &p))
  {
    switch (i++) {
    case 0:  // cmnd
      break;
    case 1:  // Topic / GroupTopic / DVES_123456
      mtopic = str;
      break;
    case 2:  // Text
      type = str;
    }
  }
  if (!strcmp(mtopic, sysCfg.mqtt_grptopic)) grpflg = 1;
  if (type != NULL) for(i = 0; i < strlen(type); i++) type[i] = tolower(type[i]);

  if (strcmp(mtopic, sysCfg.mqtt_grptopic) && strcmp(mtopic, sysCfg.mqtt_topic) ) {
	  char fallbacktopic[40];
          sprintf(fallbacktopic, "%s", sysCfg.device_id);  // Fall back topic
	  if (strcmp(mtopic,fallbacktopic))
		  type = NULL;
	  	  ESP_LOGI(TAG,"Error: topic %s should have not been received! Plese check cleansession flag.",mtopic);
  }
  
  memcpy(dataBuf, data, data_len);
  dataBuf[data_len] = 0;
  for(i = 0; i <= data_len; i++) dataBufLc[i] = tolower(dataBuf[i]);

  ESP_LOGI(TAG,"APP: MQTT DataCb Topic = %s, Group = %d, Type = %s, data = %s (%s)", mtopic, grpflg, type, dataBuf, dataBufLc);

  if (type != NULL) {
	uint8_t retain = 0;
	uint8_t qos = 0;
	
	  
    set_sled_blink(SLED_NORMAL_BLINK,2);
    sprintf(stopic, "%s/%s/%s", PUB_PREFIX, sysCfg.mqtt_topic, type);
    strcpy(svalue, "Error");

    uint32_t payload = atol(dataBuf);
    if (!strcmp(dataBufLc,"off")) payload = 0;
    if (!strcmp(dataBufLc,"on")) payload = 1;
    if (!strcmp(dataBufLc,"toggle")) payload = 2;

    if (!strcmp(type,"status")) {
      sprintf(svalue, "%s, %s, %s, %d, %d, %s, %d",
        esp_ota_get_app_description()->version, sysCfg.mqtt_topic, sysCfg.mqtt_subtopic, sysCfg.power, sysCfg.pulse, sysCfg.timezone, timer);
      if ((data_len > 0) && (payload == 1)) {
        /* os_sprintf(svalue, "%s, %s, "MQTT_CLIENT_ID", %s, %s, %s, %s, %d",
          svalue, sysCfg.mqtt_grptopic, system_get_chip_id(), sysCfg.otaUrl, sysCfg.sta_ssid, sysCfg.sta_pwd, sysCfg.mqtt_host, heartbeat); */
		sprintf(svalue, "%s, %s, %s, %s, %s, %s, %d",
          sysCfg.mqtt_grptopic, sysCfg.device_id, sysCfg.otaUrl, sysCfg.sta_ssid, "****", sysCfg.mqtt_host, heartbeat);  
      }
      if ((data_len > 0) && (payload == 2)) {
//#ifdef SERIAL_IO
//        system_print_meminfo();
//#endif
        sprintf(svalue, "Version %s, Compile date %s %s, idf %s", esp_ota_get_app_description()->version, esp_ota_get_app_description()->date, esp_ota_get_app_description()->time, esp_ota_get_app_description()->version);
      }
    }
    else if (!grpflg && !strcmp(type,"upgrade")) {
      if ((data_len > 0) && (payload == 1)) {
        set_sled_blink(SLED_NORMAL_BLINK,4);
        otaflag = 3;
        sprintf(svalue, "Upgrade %s", esp_ota_get_app_description()->version);
      }
      else
        sprintf(svalue, "1 to upgrade");
    }
    else if (!grpflg && !strcmp(type,"otaurl")) {
      if ((data_len > 0) && (data_len < 80))
        strcpy(sysCfg.otaUrl, (payload == 1) ? OTA_URL : dataBuf);
      sprintf(svalue, "%s", sysCfg.otaUrl);
    }
    else if (!grpflg && !strcmp(type,"ssid")) {
      if ((data_len > 0) && (data_len < 32)) {
        strcpy(sysCfg.sta_ssid, (payload == 1) ? STA_SSID : dataBuf);
        //restartflag = 2;
      }
      sprintf(svalue, "%s", sysCfg.sta_ssid);
    }
    else if (!grpflg && !strcmp(type,"password")) {
      if ((data_len > 0) && (data_len < 64)) {
        strcpy(sysCfg.sta_pwd, (payload == 1) ? STA_PASS : dataBuf);
        //restartflag = 2;
      }
      // os_sprintf(svalue, "%s", sysCfg.sta_pwd);
	  sprintf(svalue, "%s", "***");
    }
    else if (!grpflg && !strcmp(type,"mqtthost")) {
      if ((data_len > 0) && (data_len < 32)) {
        strcpy(sysCfg.mqtt_host, (payload == 1) ? MQTT_HOST : dataBuf);
        //restartflag = 2;
      }
      sprintf(svalue, "%s", sysCfg.mqtt_host);
    }
	else if (!grpflg && !strcmp(type,"mqttport")) {
      if ((data_len > 0) && (data_len < 32)) {
        sysCfg.mqtt_port = ((payload == 1) ? MQTT_PORT : payload);
        //restartflag = 2;
      }
      sprintf(svalue, "%d", sysCfg.mqtt_port);
    }else if (!grpflg && !strcmp(type,"mqttuser")) {
      if ((data_len > 0) && (data_len < 32)) {
        strcpy(sysCfg.mqtt_user, (payload == 1) ? MQTT_USER : dataBuf);
        //restartflag = 2;
      }
      sprintf(svalue, "%s", sysCfg.mqtt_user);
    }
	else if (!grpflg && !strcmp(type,"mqttpass")) {
      if ((data_len > 0) && (data_len < 32)) {
        strcpy(sysCfg.mqtt_pass, (payload == 1) ? MQTT_PASS : dataBuf);
        //restartflag = 2;
      }
      //os_sprintf(svalue, "%s", sysCfg.mqtt_pass);
	  sprintf(svalue, "%s", "***");
	  //sprintf(svalue, "%s", sysCfg.mqtt_pass);
    }
	else if (!grpflg && !strcmp(type,"mqttcleansession")) {
      if ((data_len > 0) && (data_len < 32)) {
		if ((payload == 1) || (payload == 0)) {
			sysCfg.mqtt_cleansession = payload;
            restartflag = 2;
		}
		else sprintf(svalue, "error MQTTCLEANSESSION parameter must be 0 or 1");
      }
      sprintf(svalue, "%d", sysCfg.mqtt_cleansession);
    }
    else if (!strcmp(type,"grouptopic")) {
      if ((data_len > 0) && (data_len < 32)) {
        for(i = 0; i <= data_len; i++)
          if ((dataBuf[i] == '/') || (dataBuf[i] == '+') || (dataBuf[i] == '#')) dataBuf[i] = '_';
        sprintf(svalue, "%s", sysCfg.device_id);
        if (!strcmp(dataBuf, svalue)) payload = 1;
        strcpy(sysCfg.mqtt_grptopic, (payload == 1) ? MQTT_GRPTOPIC : dataBuf);
		sysCfg.mqtt_cleansession = 1; //force cleansession after topic change
        restartflag = 2;
      }
      sprintf(svalue, "%s", sysCfg.mqtt_grptopic);
    }
    else if (!grpflg && !strcmp(type,"topic")) {
      if ((data_len > 0) && (data_len < 32)) {
        for(i = 0; i <= data_len; i++)
          if ((dataBuf[i] == '/') || (dataBuf[i] == '+') || (dataBuf[i] == '#')) dataBuf[i] = '_';
        sprintf(svalue, "%s", sysCfg.device_id);
        if (!strcmp(dataBuf, svalue)) payload = 1;
        strcpy(sysCfg.mqtt_topic, (payload == 1) ? MQTT_TOPIC : dataBuf);
		sysCfg.mqtt_cleansession = 1; //force cleansession after topic change
        restartflag = 2;
      }
      sprintf(svalue, "%s", sysCfg.mqtt_topic);
    }
    else if (!grpflg && !strcmp(type,"smartconfig")) {
      if ((data_len > 0) && (payload == 1)) {
        smartconfigflag = 1;
        sprintf(svalue, "Smartconfig started");
      } else
        sprintf(svalue, "1 to start smartconfig");
    }
    else if (!grpflg && !strcmp(type,"restart")) {
      if ((data_len > 0) && (payload == 1)) {
        restartflag = 2;
        sprintf(svalue, "Restarting");
      } else
        sprintf(svalue, "1 to restart");
    }
    else if (!grpflg && !strcmp(type,"reset")) {
      if ((data_len > 0) && (payload == 1)) {
        CFG_Default();
        restartflag = 2;
        sprintf(svalue, "Reset and Restarting");
      } else
        sprintf(svalue, "1 to reset");
    }
    else if (!strcmp(type,"timezone")) {
      if ((data_len > 0) && (data_len < 64)) {
        strcpy(sysCfg.timezone, (payload == 1) ? APP_TIMEZONE : dataBuf);
        restartflag = 2;
      }
      sprintf(svalue, "%s", sysCfg.timezone);
    }
	else if (!strcmp(type,"time")) {
      if ((data_len > 0) && (data_len <= 32)) {
      }
      sprintf(svalue, "%s", rtc_sntp_getasctime());
    }
    else if ((!strcmp(type,"light")) || (!strcmp(type,"power"))) {
      //os_sprintf(sysCfg.mqtt_subtopic, "%s", type);
	  if ((data_len > 0) /*&& (payload >= 0)*/ && (payload <= 2)) {
			switch (payload) {
			case 0: // Off
			case 1: // On
			  sysCfg.power = payload;
			  break;
			case 2: // Toggle
			  sysCfg.power ^= 1;
			  break;
			}
			if (sysCfg.power) pulse = sysCfg.pulse;
			timer = 0;
			send_timer(client);
			//   GPIO_OUTPUT_SET(REL_PIN, sysCfg.power);
			power_set(sysCfg.power);
      }
      sprintf(stopic, "%s/%s/%s", PUB_PREFIX, sysCfg.mqtt_topic, sysCfg.mqtt_subtopic);
	  strcpy(svalue, (sysCfg.power == 0) ? "off" : "on");
	  retain = 1; qos = 1;
    }
    else if (!strcmp(type,"timer")) {
        if ((data_len > 0) && (payload > 0) && (payload <= 1440)) {
			if (dataBuf[0]=='+') timer += payload;        
			else timer = payload;
			
			sysCfg.power = 1;
			pulse = sysCfg.pulse;
			//GPIO_OUTPUT_SET(REL_PIN, sysCfg.power);
			power_set(sysCfg.power);
			send_power(client);	
		}
        sprintf(svalue, "%d", timer);
	    retain = 1;
    }
	else if (!grpflg && !strcmp(type,"pulse")) {
      if ((data_len > 0) && (data_len < 32) ) {
        sysCfg.pulse = payload;
      }
      sprintf(svalue, "%d", sysCfg.pulse);
    }
	else if (!grpflg && !strcmp(type,"poweratreset")) {
      if ((data_len > 0) && (data_len < 32) && (payload <= 2)) {
		sysCfg.power_at_reset = payload;
      }
	  switch (sysCfg.power_at_reset) {
        case 0: 
			strcpy(svalue, "off");// Off
			break;
        case 1: 
			strcpy(svalue, "on");// On
			break;
        case 2: // last state
			strcpy(svalue, "last_state");// On
			break;
        }
    }
	else if (!grpflg && !strcmp(type,"mqttotastart")) {
      if ((data_len > 0) && (payload == 1)) {
        if (ota_mqtt_start(client,sysCfg.device_id)) {
			set_sled_state(SLED_NORMAL_BLINK);
			comm_queue_rxdisable();
			mqttotaflag = 20;
			strcpy(svalue, "on");
		} else strcpy(svalue, "Running image not confirmed");
      }
      else
        strcpy(svalue, "1 to start");
    }
	else if (!grpflg && !strcmp(type,"mqttotaconfirm")) {
      if ((data_len > 0) && (payload == 1)) {
        if (ota_mqtt_confirm()) 
			strcpy(svalue, "Confirmed OTA image");
		else strcpy(svalue, "OTA image already confirmed");
      }
      else
        strcpy(svalue, "1 to start");
    }
    else {
      type = NULL;
    }
    if (type == NULL) {
      set_sled_blink(SLED_FAST_BLINK,4);
      ESP_LOGI(TAG,"APP: Syntax error");
      sprintf(stopic, "%s/%s/SYNTAX", PUB_PREFIX, sysCfg.mqtt_topic);
      if (!grpflg)
        strcpy(svalue, "Status, Upgrade, Mqttotastart, Mqttotaconfirm, Otaurl, Restart, Reset, Smartconfig, SSId, Password, MqttHost, MqttPort, MqttUser, MqttPass, MqttCleansession, GroupTopic, Topic, Timezone, Time, Light, Power, Timer, PowerAtReset, Pulse");
      else
        strcpy(svalue, "Status, GroupTopic, Timezone, Light, Power");
    }
    //MQTT_Publish_Tx(client, stopic, svalue, strlen(svalue), qos, retain);
	 if (mqtt_connected) esp_mqtt_client_publish(client, stopic, svalue, strlen(svalue), qos, retain);
	
	printf("%s = %s\n", strrchr(stopic,'/')+1, svalue);
	telnetSend(strrchr(stopic,'/')+1, strlen(strrchr(stopic,'/')+1));
	telnetSend(" = ", 3);
	telnetSend(svalue, strlen(svalue));
	telnetSend("\n", 1);
  }
  free(topicBuf);
  free(dataBuf);
  free(dataBufLc);
}


static esp_err_t mqtt_event_handler_cb(esp_mqtt_event_handle_t event)
{
    esp_mqtt_client_handle_t client = event->client;
    
    // your_context_t *context = event->context;
    switch (event->event_id) {
        case MQTT_EVENT_CONNECTED:
			mqtt_sethigher_state(STATE_MQTT_CONNECTED);
            ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
			mqtt_connected = true;
            mqttConnectedCb(client);
			break;
        case MQTT_EVENT_DISCONNECTED:
			mqtt_setlower_state(STATE_WIFI_CONNECTED);
            ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
			mqtt_connected = false;
            break;

        case MQTT_EVENT_SUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_UNSUBSCRIBED:
            ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_PUBLISHED:
            ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
            break;
        case MQTT_EVENT_DATA:
            ESP_LOGI(TAG, "MQTT_EVENT_DATA");
			if (ota_mqtt_check(event->topic, event->topic_len,event->data, event->data_len,event->total_data_len)==0) {
				if ((event->total_data_len<=DATA_SIZE) && (event->topic_len<=TOPIC_SIZE)) {
					printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
					printf("DATA=%.*s\r\n", event->data_len, event->data);
					//mqttDataCb(client, event->topic, event->topic_len, event->data, event->data_len);
					comm_queue_rxsend(event->topic, event->topic_len, event->data, event->data_len);
				}
				else ESP_LOGE(TAG, "Error topic lenght=%d or data lenght=%d exceed limits", event->topic_len, event->data_len);
			} else mqttotaflag = 20;
            break;
        case MQTT_EVENT_ERROR:
            ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
            break;
        default:
            ESP_LOGI(TAG, "Other event id:%d", event->event_id);
            break;
    }
    return ESP_OK;
}

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data) 
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%d", base, event_id);
    mqtt_event_handler_cb(event_data);
}

static void mqtt_msg_handler_task(void *pvParameters)
{
	esp_mqtt_client_handle_t client = *(esp_mqtt_client_handle_t *)pvParameters;
	struct rx_msg_t rx_msg;
	
	while (1) {
		comm_queue_rxreceive(&rx_msg);
		mqttDataCb(client, rx_msg.topic, rx_msg.topic_len, rx_msg.data, rx_msg.data_len);
	}
}

void tick100msCallback(TimerHandle_t xTimer)
{
  uint8_t button;
  //char stopic[128], svalue[128];
  //char *token;
  static uint16_t dtimer=0;	

  state++;
  if (state == STATES) {             // Every second
    state = 0;
    /*
	time_t now;
	time(&now);
	struct tm timeinfo;
	localtime_r(&now, &timeinfo);
    if ((timeinfo.Minute == 0) && (timeinfo.Second == 30)) send_heartbeat();
    */
  }

  esp_mqtt_client_handle_t client = (esp_mqtt_client_handle_t)pvTimerGetTimerID(xTimer);
  
  if (pulse) {
	  pulse--;
	  if ((pulse==0)&&(sysCfg.power)) {
			timer = 0;
			sysCfg.power = 0;
			//GPIO_OUTPUT_SET(REL_PIN, sysCfg.power);
			send_power(client);
			send_timer(client);
	  }
  }
  if (timer) {
	  dtimer++;
	  if (dtimer==600) {	 
		dtimer=0;	  	
		timer--;
	  	send_timer(client);
		if ((timer==0)&&(sysCfg.power)) {
			sysCfg.power = 0;
			//GPIO_OUTPUT_SET(REL_PIN, sysCfg.power);
			send_power(client);
			send_timer(client);
		}
	  }
  }
  else dtimer = 0;	

  
  button = button_get();
  //if ((button == PRESSED) && (lastbutton == NOT_PRESSED)) led_on();
  //if ((button == NOT_PRESSED) && (lastbutton == PRESSED)) led_off();
  
  if ((button == PRESSED) && (lastbutton == NOT_PRESSED)) {
    if (multiwindow == 0) {
      multipress = 1;
      sysCfg.power ^= 1;
      //GPIO_OUTPUT_SET(REL_PIN, sysCfg.power);
	  power_set(sysCfg.power);
      if (sysCfg.power) pulse = sysCfg.pulse;
      timer = 0;	
      send_power(client);
      send_timer(client);
    } else {
      multipress++;
      ESP_LOGD(TAG, "APP: Multipress %d\n", multipress);
    }
    set_sled_blink(SLED_NORMAL_BLINK,1);
    multiwindow = STATES;            // 1 second multi press window
  }
  lastbutton = button;
  if (button == NOT_PRESSED) {
    holdcount = 0;
  } else {
    holdcount++;
    if (holdcount == (STATES *4)) {  // 4 seconds button hold
	  //restore firmware and settings
      factoryrestoreflag = 4;
      multipress = 0;
      //restartflag = 4;               // Allow 4 second delay to release button
      
	  //smartconfigflag = 1;
    }
  }
  if (multiwindow) {
    multiwindow--;
  } else {
	switch (multipress) {
    case 4:
      smartconfigflag = 1;
      //set_sled_state(SLED_FAST_FLASH);
      break;
    /*
	 case 6:
      otaflag = 1;
      //GPIO_OUTPUT_SET(LED_PIN, 0);
	  set_sled_state(SLED_ON);
      break;
	*/
    }
    multipress = 0;
  }

  /*
  if ((blinks) && (state & 1)) {
    blinkstate ^= 1;
    //GPIO_OUTPUT_SET(LED_PIN, blinkstate);
	led_set(blinkstate);
    if ((!blinkstate)&&(blinks!=USHRT_MAX)) blinks--;
	if (blinks==0) { blinkstate = 0; led_set(0);}
  }
  */
  update_sled_state();
  
  switch (state) {
  case (STATES/10)*2:
    if (otaflag) {
      otaflag--;
      if (otaflag <= 0) {
        //os_timer_disarm(&state_timer);
        //start_ota(sysCfg.otaUrl);
		ota_http(sysCfg.otaUrl);
      }
    }
	if (mqttotaflag) {
		mqttotaflag--;
		if (mqttotaflag <= 0) {
			ota_mqtt_stop(client);
			comm_queue_rxenable();
			set_sled_blink(SLED_FAST_BLINK,2);
			send_mqttota_off(client);
		}
	}
	if (factoryrestoreflag) {
		set_sled_state(SLED_ON);
		factoryrestoreflag--;
		if (factoryrestoreflag <= 0) {
			ESP_LOGI(TAG, "Restoring factory firmware...");
			esp_partition_iterator_t pi = esp_partition_find(ESP_PARTITION_TYPE_APP, ESP_PARTITION_SUBTYPE_APP_FACTORY, NULL);
			if(pi != NULL) {
				CFG_Default();
				const esp_partition_t* factory = esp_partition_get(pi);
				esp_partition_iterator_release(pi);
			    
				if(esp_ota_set_boot_partition(factory) == ESP_OK) {
					ESP_LOGI(TAG, "Factory firmware restored, rebooting");
					restartflag = 4;
				}
				
			}
		}
	}
    break;
  case (STATES/10)*3: //check every 10 second mqtt connection status and retry connecting
    if (mqtt_get_state()==STATE_WIFI_CONNECTED) {
		if (client!=NULL) {
			mqttretry++;
			if (mqttretry>=10) {
				esp_mqtt_client_reconnect(client);
				mqttretry=0;
			}
		}
		mqtt_sethigher_state(STATE_MQTT_CONNECTING);
	}
	break;	
  case (STATES/10)*4:
    CFG_Save();
    if (restartflag) {
      restartflag--;
      if (restartflag <= 0) esp_restart();
    }
    break;
  
    case (STATES/10)*6:
    if (smartconfigflag) {
      smartconfigflag = 0;
      apconfig_init();
	  set_sled_state(SLED_FAST_FLASH);
    }
    break;
  	
  }
}

void mqtt_init()
{
	esp_mqtt_client_config_t mqtt_cfg = {
        .uri = sysCfg.mqtt_host,
		.port = sysCfg.mqtt_port,
		.client_id = sysCfg.device_id,
		.username = sysCfg.mqtt_user,
		.password = sysCfg.mqtt_pass,
		.disable_clean_session = !sysCfg.mqtt_cleansession,
		.keepalive = sysCfg.mqtt_keepalive,
		//.disable_auto_reconnect=true,
		//.lwt_topic = ,
		//.lwt_msg =,
		//.lwt_qos = ,
		//.lwt_retain = ,
    };
	
	ESP_LOGI(TAG, "Configuried MQTT broker: %s port: %d client id: %s username: %s",mqtt_cfg.uri,mqtt_cfg.port,mqtt_cfg.client_id,mqtt_cfg.username);
	esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
	
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, client);
    
	TimerHandle_t xtick100msTimer;
	//set_sled_state(SLED_OFF);
    xtick100msTimer = xTimerCreate("tick100ms", pdMS_TO_TICKS(100), pdTRUE, client, tick100msCallback);
    xTimerStart(xtick100msTimer, 0);
	
	xTaskCreate(mqtt_msg_handler_task, "mqtt_msg_handler", 4096, &client, 5, NULL);
	esp_mqtt_client_start(client);
	
    gpio_init();
}


