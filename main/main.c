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
#include "configuration_mode.h"
#include "esp_system.h"
#include "esp_mac.h"

//192.168.11.155

static TaskHandle_t xConfigure = NULL;

static const char *TAG = "APP_MAIN";

void app_main() {
    esp_err_t ret = nvs_flash_init();  // key-value pair memory
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret); // call abort when not ESP_OK

    config_init();
    config_wifi_ssid_load();
    config_wifi_pass_load();

    wifi_init_sta();
    xTaskCreate(wifi_connect, "wifi_connect", 4096, NULL, 5, NULL);

    xTaskCreate(configure, "configure", 4096, NULL, 5, &xConfigure);

    while (!isConnected) {

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    ESP_LOGI(TAG, "Connected to WiFi");
    vTaskDelay(1000 / portTICK_PERIOD_MS);

    // Get MAC address
    uint8_t mac[6];
    char mac_str[18]; // Buffer for MAC address as a string
    esp_err_t mac_ret = esp_efuse_mac_get_default(mac); // Retrieve the MAC address

    if (mac_ret == ESP_OK) {
        snprintf(mac_str, sizeof(mac_str), "%02X:%02X:%02X:%02X:%02X:%02X",
                mac[0], mac[1], mac[2], mac[3], mac[4], mac[5]);
        ESP_LOGI(TAG, "Device MAC Address: %s", mac_str);
    } else {
        ESP_LOGE(TAG, "Failed to retrieve MAC address");
        strncpy(mac_str, "UNKNOWN", sizeof(mac_str));
    }

    // Main program loop
    mqtt_init(config_mqtt_get_uri(), config_user_id_get(), mac_str);
    while (1){
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }

    config_cleanup();
}