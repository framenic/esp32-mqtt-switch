
#define STATE_RESET			   0
#define STATE_WIFI_CONNECTING  1
#define STATE_WIFI_CONNECTED   2
#define STATE_MQTT_CONNECTING  3 
#define STATE_MQTT_CONNECTED   4


void mqtt_init();

void mqtt_setlower_state(uint8_t state);
void mqtt_sethigher_state(uint8_t state);
uint8_t mqtt_get_state();