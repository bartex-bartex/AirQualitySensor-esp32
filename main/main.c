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

// only http -> port 80
char* url = "http://example.com/";

static const char *TAG = "APP_MAIN";

void app_main() {
    esp_err_t ret = nvs_flash_init();  // key-value pair memory
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret); // call abort when not ESP_OK

    ble_init();


    // ESP_LOGI("APP_MAIN", "ESP_WIFI_INIT");
    
    // blocking call - until IP is obtained via DHCP
    // wifi_init_sta();

    // get_request(url);

    // config_init();

    // config_wifi_save("My ssid", "My pass");

    // if (!config_wifi_load()) {
    //     ESP_LOGI("APP_MAIN", "Failed to load Wi-Fi configuration");
    // }

    // ESP_LOGI("APP_MAIN", "SSID: %s", config_wifi_get_ssid());
    // ESP_LOGI("APP_MAIN", "PASS: %s", config_wifi_get_pass());

    // config_cleanup();


}