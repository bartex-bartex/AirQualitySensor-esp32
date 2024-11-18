#include <stdio.h>
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "wifi_manager.h"
#include "config.h"
#include "http_client.h"
#include "config_manager.h"
#include "ble_manager.h"
#include "mqtt.h"

// only http -> port 80
char* url = "http://example.com/";
char* server_url = "mqtt://192.168.33.9/";

static const char *TAG = "APP_MAIN";

static TaskHandle_t xBleHandle = NULL;

void app_main() {
    esp_err_t ret = nvs_flash_init();  // key-value pair memory
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret); // call abort when not ESP_OK

    config_init();
    config_wifi_load();
    
    wifi_init_sta();

    bool connected = false;
    const char* ssid;
    const char* pass;

    while (1) {
        ESP_LOGI(TAG, "Update credentials");
        ssid = config_wifi_get_ssid();
        pass = config_wifi_get_pass();

        ESP_LOGI(TAG, "SSID: %s, PASS: %s", ssid, pass);
        connected = wifi_connect(ssid, pass);
        if (connected) {
            ESP_LOGI(TAG, "Connected to Wi-Fi");
            break;
        }

        ble_init();

        xTaskCreate(nimble_host_task, "NimBLE", 4096, NULL, 5, &xBleHandle);

        vTaskDelay(45000 / portTICK_PERIOD_MS);

        nimble_host_stop_task();
    }
    // TODO: Handle disconnection error
    // TODO: Handle too fast request before getting IP assigned

    mqtt_init(server_url);

    while(1) {

    }

    config_cleanup();
}