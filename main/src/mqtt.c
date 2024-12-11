#include <string.h>
#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_random.h"

#include "mqtt.h"

static EventGroupHandle_t mqtt_event_group;
const int MQTT_CONNECTED_BIT = BIT0;

static const char* TAG = "MQTT";
static const char* base = "mqtt://";
static const char* suffix = "/";

static void mqtt_event_handler(void *handler_args, esp_event_base_t base, int32_t event_id, void *event_data)
{
    ESP_LOGD(TAG, "Event dispatched from event loop base=%s, event_id=%" PRIi32 "", base, event_id);
    esp_mqtt_event_handle_t event = event_data;
    esp_mqtt_client_handle_t client = event->client;
    int msg_id;
    switch ((esp_mqtt_event_id_t)event_id) {
    case MQTT_EVENT_CONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_CONNECTED");
        xEventGroupSetBits(mqtt_event_group, MQTT_CONNECTED_BIT);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        msg_id = esp_mqtt_client_publish(client, "/topic/qos0", "data", 0, 0, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        printf("TOPIC=%.*s\r\n", event->topic_len, event->topic);
        printf("DATA=%.*s\r\n", event->data_len, event->data);
        break;
    // case MQTT_EVENT_ERROR:
    //     ESP_LOGI(TAG, "MQTT_EVENT_ERROR");
    //     if (event->error_handle->error_type == MQTT_ERROR_TYPE_TCP_TRANSPORT) {
    //         log_error_if_nonzero("reported from esp-tls", event->error_handle->esp_tls_last_esp_err);
    //         log_error_if_nonzero("reported from tls stack", event->error_handle->esp_tls_stack_err);
    //         log_error_if_nonzero("captured as transport's socket errno",  event->error_handle->esp_transport_sock_errno);
    //         ESP_LOGI(TAG, "Last errno string (%s)", strerror(event->error_handle->esp_transport_sock_errno));

    //     }
    //     break;
    default:
        ESP_LOGI(TAG, "Other event id:%d", event->event_id);
        break;
    }
}

// mosquitto_sub.exe -h localhost -t "test/qos1" -v
void mqtt_init(char* uri){
    if (uri == NULL){
        ESP_LOGE(TAG, "MQTT URI is NULL");
        return;
    }

    size_t buffer_size = strlen(base) + strlen(uri) + strlen(suffix);
    char* result = (char*)malloc(buffer_size);

    sprintf(result, "%s%s%s", base, uri, suffix);

    ESP_LOGI(TAG, "MQTT URI: %s", result);

    mqtt_event_group = xEventGroupCreate();

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = result,
    };

    esp_mqtt_client_handle_t client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    xEventGroupWaitBits(mqtt_event_group, MQTT_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "MQTT client connected, proceeding...");

    int msg_id;
    while (1){
        char message[20];
        int bpm = 60 + (uint8_t)(esp_random() % 21);
        sprintf(message, "BPM: %d", bpm);
        
        msg_id = esp_mqtt_client_publish(client, "test/qos1", message, 0, 1, 0);
        ESP_LOGI(TAG, "sent publish successful, msg_id=%d", msg_id);

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    free(result);
}