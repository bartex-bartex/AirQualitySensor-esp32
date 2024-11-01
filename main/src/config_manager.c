#include "config_manager.h"
#include "esp_spiffs.h"
#include "esp_log.h"

#define WIFI_SSID_MAX_LEN 64
#define WIFI_PASS_MAX_LEN 64

static const char* TAG = "CONFIG_MANAGER";

// Define the struct to hold Wi-Fi credential data
typedef struct {
    char* ssid;
    char* pass;
} wifi_config;

static wifi_config config = {NULL, NULL};

bool config_init() {
    esp_err_t ret;

    // Configuration for SPIFFS
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",    // Mount point
        .partition_label = NULL,   // Use the default partition
        .max_files = 5,            // Maximum number of open files
        .format_if_mount_failed = false // Format if mount fails
    };

    // Initialize SPIFFS
    ret = esp_vfs_spiffs_register(&conf);
    if (ret != ESP_OK) {
        ESP_LOGI(TAG, "Failed to mount file system, error: %s", esp_err_to_name(ret));
        return false; // Mount failed
    }

    // Allocate memory for ssid and pass fields
    config.ssid = (char*)malloc(WIFI_SSID_MAX_LEN);
    config.pass = (char*)malloc(WIFI_PASS_MAX_LEN);

    if (config.ssid == NULL || config.pass == NULL) {
        ESP_LOGE(TAG, "Memory allocation failed");
        // Free any allocated memory in case of failure
        config_cleanup();
        return false;
    }

    // Optionally initialize to empty strings
    config.ssid[0] = '\0';
    config.pass[0] = '\0';

    ESP_LOGI(TAG, "Configuration initialized successfully");
    return true; // Mount successful
}

void config_cleanup() {
    // Unmount SPIFFS if mounted
    esp_vfs_spiffs_unregister(NULL);

    // Free allocated memory
    if (config.ssid) {
        free(config.ssid);
        config.ssid = NULL;
    }
    if (config.pass) {
        free(config.pass);
        config.pass = NULL;
    }
}
