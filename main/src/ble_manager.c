/* NimBLE stack APIs */
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
// #include "ble_store_config.h"

#include "ble_manager.h"
#include "gap.h"
#include "sdkconfig.h"

const char* TAG = "BLE_MANAGER";

// Function prototypes for static functions
static void on_stack_reset(int reason);
static void on_stack_sync(void);
static void nimble_host_task(void *param);

bool ble_init() {
    esp_err_t ret;
    int rc = 0;

    ESP_LOGI(TAG, "Initializing NimBLE host stack");

    ret = nimble_port_init(); // initialize both the NimBLE host and controller stacks
    if (ret != ESP_OK) {
        ESP_LOGE(TAG, "NimBLE stack initialization failed");
        return false;
    }

    /* Initialize the NimBLE GAP configuration */
    ESP_LOGI(TAG, "Initializing NimBLE GAP configuration");

    rc = gap_init();
    if (rc != 0) {
        ESP_LOGE(TAG, "failed to initialize GAP service, error code: %d", rc);
        return false;
    }

    /* Set host callbacks */
    ESP_LOGI(TAG, "Setting host callbacks");

    ble_hs_cfg.reset_cb = on_stack_reset;
    ble_hs_cfg.sync_cb = on_stack_sync;  // when controller and host are synchronized -> APP_START
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    /* Store host configuration */
    // ble_store_config_init(); // TODO: What about this
    xTaskCreate(nimble_host_task, "NimBLE", 4096, NULL, 5, NULL);

    return true;
}

static void on_stack_reset(int reason) {
    /* On reset, print reset reason to console */
    ESP_LOGI(TAG, "nimble stack reset, reset reason: %d", reason);
}

static void on_stack_sync(void) {
    /* On stack sync, do advertising initialization */
    adv_init();
}

static void nimble_host_task(void *param) {
    /* Task entry log */
    ESP_LOGI(TAG, "nimble host task has been started!");

    /* This function won't return until nimble_port_stop() is executed */
    nimble_port_run();

    /* Clean up at exit */
    vTaskDelete(NULL);
}
