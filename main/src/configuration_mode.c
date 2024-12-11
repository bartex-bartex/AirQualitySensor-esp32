#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"

#include "ble_manager.h"
#include "configuration_mode.h"
#include "wifi_manager.h"

// boot button
// 0 -> pressed
// 1 -> not pressed
#define CONFIGURE_GPIO 0

static const char* TAG = "CONFIGURATION_MODE";

static TaskHandle_t xBleHandle = NULL;

void configure(void* pvParameters){
    ESP_LOGI(TAG, "Configuration mode task started");
    gpio_reset_pin(CONFIGURE_GPIO);
    gpio_set_direction(CONFIGURE_GPIO, GPIO_MODE_INPUT);
    ESP_LOGI(TAG, "Port setup");

    while (true){
        // int val = gpio_get_level(CONFIGURE_GPIO);
        // ESP_LOGI(TAG, "Analyzing configuration mode: %d", val); // suddenly stopped crashing!?
        if (gpio_get_level(CONFIGURE_GPIO) == 0 && xBleHandle == NULL){
            ESP_LOGI(TAG, "Configuration mode enabled");
            ble_init();
            xTaskCreate(nimble_host_task, "NimBLE", 4096, NULL, 5, &xBleHandle);
        } else if (gpio_get_level(CONFIGURE_GPIO) == 0 && xBleHandle != NULL){
            ESP_LOGI(TAG, "Configuration mode disabled");
            nimble_host_stop_task();
            xBleHandle = NULL;

            // now I assume that user clicks "reboot" button
            // if (isConnected){
            //     // reboot
            //     esp_restart();
            // }
        }

        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}