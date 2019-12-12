#include "esp_log.h"
#include "esp_ota_ops.h"
#include "esp_flash_partitions.h"
#include "esp_partition.h"

//#include "mqtt_config.h"
#include "mqtt_client.h"

#include <string.h>

#include "ota_mqtt.h"

static const char *TAG = "ota_mqtt.c";

void ota_mqtt_end(const esp_partition_t *p_update_partition,esp_ota_handle_t update_handle);
const esp_partition_t * ota_mqtt_begin();

const esp_partition_t * ota_mqtt_begin()
{
	ESP_LOGI(TAG, "Starting OTA update from MQTT");

    const esp_partition_t *configured = esp_ota_get_boot_partition();
    const esp_partition_t *running = esp_ota_get_running_partition();
	
	if (configured != running) {
        ESP_LOGW(TAG, "Configured OTA boot partition at offset 0x%08x, but running from offset 0x%08x",
                 configured->address, running->address);
        ESP_LOGW(TAG, "(This can happen if either the OTA boot data or preferred boot image become corrupted somehow.)");
    }
    ESP_LOGI(TAG, "Running partition type %d subtype %d (offset 0x%08x)",
             running->type, running->subtype, running->address);

	return running;
}

void ota_mqtt_do(char *ota_write_data,int data_read, int total_ota_write_data_length)
{
	esp_err_t err;
	/* update handle : set by esp_ota_begin(), must be freed via esp_ota_end() */
    static esp_ota_handle_t update_handle = 0 ;
	static const esp_partition_t *update_partition = NULL;
	static const esp_partition_t *running;
	
	static int binary_file_length = 0;
    /*deal with all receive packet*/
    static bool image_header_was_checked = false;
	
	if (update_partition==NULL) { //first time being called
		running = ota_mqtt_begin();
			
		update_partition = esp_ota_get_next_update_partition(NULL);
		ESP_LOGI(TAG, "Writing to partition subtype %d at offset 0x%x",update_partition->subtype, update_partition->address);
		assert(update_partition != NULL);
		
		binary_file_length = 0;
		image_header_was_checked = false;
	}
	
	
	if (data_read < 0) {
            ESP_LOGE(TAG, "Error: data read error");
			update_partition = NULL;
			return;
    } else if (data_read > 0) {
            if (image_header_was_checked == false) {
                esp_app_desc_t new_app_info;
                if (data_read > sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t) + sizeof(esp_app_desc_t)) {
                    // check current version with downloading
                    memcpy(&new_app_info, &ota_write_data[sizeof(esp_image_header_t) + sizeof(esp_image_segment_header_t)], sizeof(esp_app_desc_t));
                    ESP_LOGI(TAG, "New firmware version: %s", new_app_info.version);

                    esp_app_desc_t running_app_info;
                    if (esp_ota_get_partition_description(running, &running_app_info) == ESP_OK) {
                        ESP_LOGI(TAG, "Running firmware version: %s", running_app_info.version);
                    }

                    const esp_partition_t* last_invalid_app = esp_ota_get_last_invalid_partition();
                    esp_app_desc_t invalid_app_info;
                    if (esp_ota_get_partition_description(last_invalid_app, &invalid_app_info) == ESP_OK) {
                        ESP_LOGI(TAG, "Last invalid firmware version: %s", invalid_app_info.version);
                    }

                    // check current version with last invalid partition
                    if (last_invalid_app != NULL) {
                        if (memcmp(invalid_app_info.version, new_app_info.version, sizeof(new_app_info.version)) == 0) {
                            ESP_LOGW(TAG, "New version is the same as invalid version.");
                            ESP_LOGW(TAG, "Previously, there was an attempt to launch the firmware with %s version, but it failed.", invalid_app_info.version);
                            ESP_LOGW(TAG, "The firmware has been rolled back to the previous version.");
                            
							return;
                        }
                    }
/*
                    if (memcmp(new_app_info.version, running_app_info.version, sizeof(new_app_info.version)) == 0) {
                        ESP_LOGW(TAG, "Current running version is the same as a new. We will not continue the update.");
                        
						return;
                    }
*/
                    image_header_was_checked = true;

                    err = esp_ota_begin(update_partition, OTA_SIZE_UNKNOWN, &update_handle);
                    if (err != ESP_OK) {
                        ESP_LOGE(TAG, "esp_ota_begin failed (%s)", esp_err_to_name(err));
                        update_partition = NULL;
						return;
                    }
                    ESP_LOGI(TAG, "esp_ota_begin succeeded");
                } else {
                    ESP_LOGE(TAG, "received package does not fit len");
					
                    update_partition = NULL;
					return;
                }
            }
            err = esp_ota_write( update_handle, (const void *)ota_write_data, data_read);
            if (err != ESP_OK) {
				ESP_LOGE(TAG, "esp_ota_write failed (%s)", esp_err_to_name(err));
                update_partition = NULL;
				return;
            }
            binary_file_length += data_read;
            ESP_LOGI(TAG, "Written image length %d", binary_file_length);
			if (binary_file_length == total_ota_write_data_length) {
				ESP_LOGI(TAG, "Message end, all data received");
				ESP_LOGI(TAG, "Total Write binary data length : %d", binary_file_length);
				ota_mqtt_end(update_partition,update_handle);
				update_partition = NULL;
				return;
			}
        }
}

void ota_mqtt_end(const esp_partition_t *update_partition,esp_ota_handle_t update_handle)
{
	esp_err_t err;
	
	if (esp_ota_end(update_handle) != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_end failed!");
        
		return;
    }
	
	err = esp_ota_set_boot_partition(update_partition);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "esp_ota_set_boot_partition failed (%s)!", esp_err_to_name(err));
		return;
	}
    ESP_LOGI(TAG, "Prepare to restart system!");
    esp_restart();
}

char *ota_topic=NULL;

int ota_mqtt_start(esp_mqtt_client_handle_t client, char* topic)
{
	int msg_id;
	
	const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    
	if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
			ESP_LOGI(TAG, "Running image not confirmed");
			return 0;
		};
	}	
	
	if (ota_topic==NULL) {
		ota_topic = (char*)malloc(40);
		snprintf(ota_topic,40,"OTA/%s/data",topic);
		ESP_LOGI(TAG, "Subscribing for OTA data at topic %s",ota_topic);
		msg_id = esp_mqtt_client_subscribe(client, ota_topic, 0);
		ESP_LOGI(TAG, "sent subscribe successful, msg_id=%d", msg_id);
	}
	else ESP_LOGI(TAG, "Error: Already subscrbed for OTA data");
	return 1;
}

void ota_mqtt_stop(esp_mqtt_client_handle_t client)
{
	int msg_id;
	char *app_ota_topic = ota_topic;
	
	ESP_LOGI(TAG, "Unsubscribing topic %s",ota_topic);
	msg_id = esp_mqtt_client_unsubscribe(client,ota_topic);
	ESP_LOGI(TAG, "sent Unsubscribe successful, msg_id=%d", msg_id);
	ota_topic = NULL;
	free(app_ota_topic);
}

int ota_mqtt_confirm()
{
	const esp_partition_t *running = esp_ota_get_running_partition();
    esp_ota_img_states_t ota_state;
    
	if (esp_ota_get_state_partition(running, &ota_state) == ESP_OK) {
        if (ota_state == ESP_OTA_IMG_PENDING_VERIFY) {
            // run diagnostic function ...
            //bool diagnostic_is_ok = diagnostic();
            //if (diagnostic_is_ok) {
                ESP_LOGI(TAG, "Diagnostics completed successfully! Continuing execution of new OTA image");
                esp_ota_mark_app_valid_cancel_rollback();
				return 1;
            //} else {
            //    ESP_LOGE(TAG, "Diagnostics failed! Start rollback to the previous version ...");
            //    esp_ota_mark_app_invalid_rollback_and_reboot();
            }
        }
	return 0;
}

int ota_mqtt_check(char* topic, int topic_length,char *ota_write_data,int ota_write_data_length, int total_ota_write_data_length)
{
	if (ota_topic==NULL) return 0;
	else if (memcmp(ota_topic, topic, topic_length)==0)
		ota_mqtt_do(ota_write_data, ota_write_data_length, total_ota_write_data_length);
	return 1;
}