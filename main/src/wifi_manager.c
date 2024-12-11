#include <stdio.h>
#include <string.h>
#include "esp_wifi.h"
#include "esp_log.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "wifi_manager.h"
#include "config.h"
#include "led_manager.h"
#include "config_manager.h"

static const char* TAG = "WIFI_MANAGER";

volatile bool isConnected = false;
static bool wasConnected = false;
// static EventGroupHandle_t s_wifi_event_group;
static TaskHandle_t xBlinkHandle = NULL;

void wifi_event_handler(void* arg, esp_event_base_t event_base, int32_t event_id, void* event_data){

    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
    {
        ESP_LOGI(TAG, "WIFI_EVENT_STA_START");

        if (xBlinkHandle == NULL){
            xTaskCreate(blink_led, "blink_led", 2048, NULL, tskIDLE_PRIORITY, &xBlinkHandle);
        }
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_CONNECTED)
    {
        ESP_LOGI(TAG, "WIFI_EVENT_STA_CONNECTED");

        isConnected = true;
        wasConnected = true;

        if (xBlinkHandle != NULL){
            vTaskDelete(xBlinkHandle);
            gpio_reset_pin(BLINK_GPIO);
            xBlinkHandle = NULL;
        }
    } 
    else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
    {
        ESP_LOGI(TAG, "WIFI_EVENT_STA_DISCONNECTED");
    
        isConnected = false;

        if (xBlinkHandle == NULL){
            xTaskCreate(blink_led, "blink_led", 2048, NULL, tskIDLE_PRIORITY, &xBlinkHandle);
        }
    } 
    else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) 
    {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        ESP_LOGI(TAG, "got ip:" IPSTR, IP2STR(&event->ip_info.ip));
    }
}

void wifi_connect(){

    while (!wasConnected){
        // create wifi config 
        // for some reason it is tough to pass |const char*|, best way is to use memcpy / strcpy
        wifi_config_t wifi_config = {
            .sta = {
                // Use memcpy to copy the ssid and password into the struct
                .ssid = {0},  // Initialize ssid array
                .password = {0}  // Initialize password array
            }
        };

        const char* ssid = config_wifi_get_ssid();
        const char* pass = config_wifi_get_pass();

        // Copy the SSID and password into the arrays
        memcpy(wifi_config.sta.ssid, ssid, strlen(ssid));
        memcpy(wifi_config.sta.password, pass, strlen(pass));

        ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA)); // set wifi mode to station
        ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config)); // set wifi config
        ESP_ERROR_CHECK(esp_wifi_start()); // start wifi

        int times = 4;
        while (times && !isConnected){
            ESP_LOGI(TAG, "Connecting to ssid: %s, pass: %s , %d try" , ssid, pass, 5 - times);
            esp_wifi_connect();

            vTaskDelay(3000 / portTICK_PERIOD_MS);

            if (times == 0){
                ESP_LOGE(TAG, "Failed to connect to ssid: %s, pass: %s", ssid, pass);
                esp_wifi_disconnect();
                esp_wifi_stop();
            }

            times--;
        }
    }

    vTaskDelete(NULL);
}

void wifi_init_sta(){
    ESP_ERROR_CHECK(esp_netif_init()); // initialize TCP/IP stack

    ESP_ERROR_CHECK(esp_event_loop_create_default()); // initialize default event loop, but nothing yet is registered
    esp_netif_create_default_wifi_sta();              // register default wifi event handlers (WIFI_START, WIFI_STOP, ...)

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT(); // prepare wifi config
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));                // initialize wifi driver with config

    // registering events
    ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &wifi_event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &wifi_event_handler, NULL));
}