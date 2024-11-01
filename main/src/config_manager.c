#include "config_manager.h"
#include "esp_spiffs.h"
#include "esp_log.h"

bool config_init(wifi_config* config) {
    esp_err_t ret;

    // Configuration for SPIFFS
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",    // Mount point
        .partition_label = NULL,   // Use the default partition
        .max_files = 5,            // Maximum number of open files
        .format_if_mount_failed = false // Format if mount fails
    };

    // Initialize SPIFFS
    ret = esp_vfs_spiffs_mount(&conf);
    if (ret != ESP_OK) {
        ESP_LOGI("CONFIG_INIT", "Failed to mount file system, error: %s", esp_err_to_name(ret));
        return false; // Mount failed
    }

    config->ssid = (char*)malloc(64);
    config->pass = (char*)malloc(64);

    return true; // Mount successful
}

void config_cleanup(wifi_config* config) {
    free(config->ssid);
    free(config->pass);
}
