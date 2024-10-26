#include <stdio.h>
#include "esp_err.h"
#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_log.h"
#include "driver/gpio.h"

#define WIFI_FAIL_BIT BIT0
#define WIFI_CONNECTED_BIT BIT1

#define SSID "Galaxy M21CABD"
#define PASS "poreba12"
#define MAX_RETRY 255

#define BLINK_GPIO 2

static bool isConnected = false;
static bool wasConnected = false;
static EventGroupHandle_t s_wifi_event_group; // static = accessible only in this file
// static uint8_t ucParametersToPass;
static TaskHandle_t xBlinkHandle = NULL;


void blink_led(void* pvParameters){
    gpio_reset_pin(BLINK_GPIO);
    gpio_set_direction(BLINK_GPIO, GPIO_MODE_OUTPUT);

    while (true){
        gpio_set_level(BLINK_GPIO, 1);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
        gpio_set_level(BLINK_GPIO, 0);
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

static void event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI("EVENT_HANDLER", "WIFI_EVENT_STA_START");
        esp_wifi_connect();  // returns ESP_OK if no error (doesn't have to connect successfully)
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        ESP_LOGI("EVENT_HANDLER", "WIFI_EVENT_STA_CONNECTED");

        isConnected = true;
        wasConnected = true;

        if (xBlinkHandle != NULL){
            vTaskDelete(xBlinkHandle);
            gpio_reset_pin(BLINK_GPIO);
            xBlinkHandle = NULL;
        }
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        char* message = wasConnected ? "reconnecting..." : "connecting...";
        ESP_LOGI("EVENT_HANDLER", "%s", message);
    
        isConnected = false;

        if (xBlinkHandle == NULL){
            xTaskCreate(blink_led, "blink_led", 2048, NULL, tskIDLE_PRIORITY, &xBlinkHandle);
        }

        ESP_ERROR_CHECK(esp_wifi_connect());
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI("EVENT_HANDLER", "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}

void wifi_init_sta(){
    s_wifi_event_group = xEventGroupCreate();

    ESP_ERROR_CHECK(esp_netif_init()); // initialize TCP/IP stack

    ESP_ERROR_CHECK(esp_event_loop_create_default()); // initialize default event loop, but nothing yet is registered
    esp_netif_create_default_wifi_sta();              // register default wifi event handlers (WIFI_START, WIFI_STOP, ...)

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); // prepare wifi config
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));                // initialize wifi driver with config

    // registering events
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

    // create wifi config 
    // for some reason it is tough to pass |const char*|, best way is to use memcpy / strcpy
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = SSID,
            .password = PASS
        }
    };

    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); // set wifi mode to station
    ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config)); // set wifi config
    ESP_ERROR_CHECK(esp_wifi_start()); // start wifi

    // best way to wait for event is to use event group
    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group, WIFI_CONNECTED_BIT, pdFALSE, pdTRUE, portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT){
        ESP_LOGI("WIFI_INIT_STA", "connected to ap SSID:%s password:%s", SSID, PASS);
    } else {
        ESP_LOGI("WIFI_INIT_STA", "Failed to connect to SSID:%s, password:%s", SSID, PASS);
    }
}

void app_main() {
    esp_err_t ret = nvs_flash_init();  // key-value pair memory
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ret = nvs_flash_init();
    }

    ESP_ERROR_CHECK(ret); // call abort when not ESP_OK

    ESP_LOGI("APP_MAIN", "ESP_WIFI_INIT");

    wifi_init_sta();
}