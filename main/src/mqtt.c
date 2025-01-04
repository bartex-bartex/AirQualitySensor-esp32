#include <string.h>
#include "mqtt_client.h"
#include "esp_log.h"
#include "esp_random.h"
#include <stdio.h>
#include <freertos/FreeRTOS.h>
#include <freertos/task.h>
#include <esp_system.h>
#include <bmp280.h>
#include <dht.h>
#include "driver/adc.h"
#include "math.h"

#include "mqtt.h"

// mosquitto_pub.exe -h localhost -t 60/30:AE:A4:E9:EE:E0/config/time_interval -m 2000
// mosquitto_sub.exe -h localhost -t "test/qos1" -v

volatile int time_interval = 5000;
esp_mqtt_client_handle_t client;

static EventGroupHandle_t mqtt_event_group;
const int MQTT_CONNECTED_BIT = BIT0;

static const char* TAG = "MQTT";
static const char* base = "mqtt://";
static const char* suffix = "/";

static char* user_id;
static char* mac;

#ifndef APP_CPU_NUM
#define APP_CPU_NUM PRO_CPU_NUM
#endif

#define CONFIG_EXAMPLE_I2C_MASTER_SDA 21 
#define CONFIG_EXAMPLE_I2C_MASTER_SCL 22
#define CONFIG_EXAMPLE_DATA_GPIO 23
#define SENSOR_TYPE DHT_TYPE_AM2301

#define MQ135_ADC_CHANNEL ADC1_CHANNEL_6  // GPIO34 is ADC1 Channel 6

char* generate_mqtt_topic(const char *suffix) {
    static char topic[64]; // Static buffer to hold the topic string

    memset(topic, 0, sizeof(topic)); 

    // Format the topic string
    snprintf(topic, sizeof(topic), "%s/%s/%s", user_id, mac, suffix);

    // Return the formatted topic string
    return topic;
}

// SENSORS part

void dht_test(void *pvParameters)
{
    float temperature, humidity;

#ifdef CONFIG_EXAMPLE_INTERNAL_PULLUP
    gpio_set_pull_mode(dht_gpio, GPIO_PULLUP_ONLY);
#endif

    while (1)
    {
        if (dht_read_float_data(SENSOR_TYPE, CONFIG_EXAMPLE_DATA_GPIO, &humidity, &temperature) == ESP_OK) {
            ESP_LOGI(TAG, "Humidity: %.1f%% Temp: %.1fC", humidity, temperature);
            
            char temp_str[16];
            snprintf(temp_str, sizeof(temp_str), "%.1f", temperature);
            esp_mqtt_client_publish(client, generate_mqtt_topic("temp"), temp_str, 0, 1, 0);

            char humidity_str[16];
            snprintf(humidity_str, sizeof(humidity_str), "%.1f", humidity);
            esp_mqtt_client_publish(client, generate_mqtt_topic("humidity"), humidity_str, 0, 1, 0);

        } else {
            ESP_LOGE(TAG, "Could not read data from sensor\n");
        }

        // If you read the sensor data too often, it will heat up
        // http://www.kandrsmith.org/RJS/Misc/Hygrometers/dht_sht_how_fast.html


        vTaskDelay(pdMS_TO_TICKS(time_interval));
    }
}

void bmp280_test(void *pvParameters)
{
    bmp280_params_t params;
    bmp280_init_default_params(&params);
    bmp280_t dev;
    memset(&dev, 0, sizeof(bmp280_t));

    ESP_ERROR_CHECK(bmp280_init_desc(&dev, BMP280_I2C_ADDRESS_0, 0, CONFIG_EXAMPLE_I2C_MASTER_SDA, CONFIG_EXAMPLE_I2C_MASTER_SCL));
    ESP_ERROR_CHECK(bmp280_init(&dev, &params));

    bool bme280p = dev.id == BME280_CHIP_ID;

    float pressure, temperature, humidity;
    int altitude = 281;

    while (1)
    {
        if (bmp280_read_float(&dev, &temperature, &pressure, &humidity) != ESP_OK)
        {
            ESP_LOGE(TAG, "Temperature/pressure reading failed");
            continue;
        }

        /* float is used in printf(). you need non-default configuration in
         * sdkconfig for ESP8266, which is enabled by default for this
         * example. see sdkconfig.defaults.esp8266
         */
        pressure = pressure / 100; // hPa
        pressure = pressure / pow(1.0 - (altitude / 44330.0), 5.255);

        ESP_LOGI(TAG, "Pressure: %.2f hPa", pressure);

        char pressure_str[16];
        snprintf(pressure_str, sizeof(pressure_str), "%.2f", pressure);
        esp_mqtt_client_publish(client, generate_mqtt_topic("pressure"), pressure_str, 0, 1, 0);


        vTaskDelay(pdMS_TO_TICKS(time_interval));
    }
}

void mq135_test(void *pvParameters){

    adc1_config_width(ADC_WIDTH_BIT_12); // 12-bit resolution
    adc1_config_channel_atten(MQ135_ADC_CHANNEL, ADC_ATTEN_DB_11);

    while (1) {
        // Read the ADC value
        int raw_adc_value = adc1_get_raw(MQ135_ADC_CHANNEL);

        // Convert the raw value to voltage (if needed)
        float voltage = (raw_adc_value * 3.3) / 4095; // For a 3.3V reference

        ESP_LOGI(TAG, "Raw ADC Value: %d, Voltage: %.2f V", raw_adc_value, voltage);

        char raw_adc_value_str[16];
        snprintf(raw_adc_value_str, sizeof(raw_adc_value_str), "%d", raw_adc_value);
        esp_mqtt_client_publish(client, generate_mqtt_topic("gas"), raw_adc_value_str, 0, 1, 0);


        vTaskDelay(pdMS_TO_TICKS(time_interval)); // Delay 1 second
    }
}

// MQTT part

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

        int msg_id = esp_mqtt_client_subscribe(client, generate_mqtt_topic("config/time_interval"), 0);

        ESP_LOGI(TAG, "Subscribed to topic, msg_id=%d", msg_id);
        break;
    case MQTT_EVENT_DISCONNECTED:
        ESP_LOGI(TAG, "MQTT_EVENT_DISCONNECTED");
        break;
    case MQTT_EVENT_SUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_SUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_UNSUBSCRIBED:
        ESP_LOGI(TAG, "MQTT_EVENT_UNSUBSCRIBED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_PUBLISHED:
        // Terminal gets messy with this.
        // ESP_LOGI(TAG, "MQTT_EVENT_PUBLISHED, msg_id=%d", event->msg_id);
        break;
    case MQTT_EVENT_DATA:
        ESP_LOGI(TAG, "MQTT_EVENT_DATA");
        ESP_LOGI(TAG, "TOPIC=%.*s\r\n", event->topic_len, event->topic);
        ESP_LOGI(TAG, "DATA=%.*s\r\n", event->data_len, event->data);


        if (strncmp(event->topic, generate_mqtt_topic("config/time_interval"), event->topic_len) == 0) {
            ESP_LOGI(TAG, "Updating time intervale to: %.*s ms", event->data_len, event->data);
            
            char data[32] = {0}; // Adjust the size as needed
            memcpy(data, event->data, event->data_len);
            data[event->data_len] = '\0';

            // Convert string to integer
            time_interval = atoi(data);

            ESP_LOGI(TAG, "Time interval updated to: %d ms", time_interval);
        }

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

void mqtt_init(char* uri, char* user_id_param, char* mac_param){
    ESP_LOGI(TAG, "Initializing MQTT");

    if (user_id_param == NULL){
        ESP_LOGE(TAG, "User ID is NULL");
        return;
    }

    user_id = user_id_param;

    if (mac_param == NULL){
        ESP_LOGE(TAG, "MAC is NULL");
        return;
    }

    mac = mac_param;

    if (uri == NULL){
        ESP_LOGE(TAG, "MQTT URI is NULL");
        return;
    }

    ESP_LOGI(TAG, "user_id: %s", user_id);
    ESP_LOGI(TAG, "mac: %s", mac);

    size_t buffer_size = strlen(base) + strlen(uri) + strlen(suffix);
    char* result = (char*)malloc(buffer_size);
    sprintf(result, "%s%s%s", base, uri, suffix);
    ESP_LOGI(TAG, "MQTT URI: %s", result);

    mqtt_event_group = xEventGroupCreate();

    esp_mqtt_client_config_t mqtt_cfg = {
        .broker.address.uri = result,
    };

    client = esp_mqtt_client_init(&mqtt_cfg);
    esp_mqtt_client_register_event(client, ESP_EVENT_ANY_ID, mqtt_event_handler, NULL);
    esp_mqtt_client_start(client);

    xEventGroupWaitBits(mqtt_event_group, MQTT_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);
    ESP_LOGI(TAG, "MQTT client connected, proceeding...");

    // setup sensors
    ESP_ERROR_CHECK(i2cdev_init());
    xTaskCreatePinnedToCore(bmp280_test, "bmp280_test", configMINIMAL_STACK_SIZE * 8, NULL, 5, NULL, APP_CPU_NUM);

    // DHT
    xTaskCreate(dht_test, "dht_test", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);

    // MQ135
    xTaskCreate(mq135_test, "mq135_test", configMINIMAL_STACK_SIZE * 3, NULL, 5, NULL);

    while (1){
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    free(result);
}

