

//void ota_mqtt_cont(char *ota_write_data,int data_read);
int ota_mqtt_start(esp_mqtt_client_handle_t client,char* topic);
void ota_mqtt_stop(esp_mqtt_client_handle_t client);
int ota_mqtt_check(char* topic, int topic_length,char *ota_write_data,int ota_write_data_length, int total_ota_write_data_length);
int ota_mqtt_confirm();