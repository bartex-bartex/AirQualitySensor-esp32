#include <string.h>
#include "config_manager.h"
#include "led_manager.h"
#include "esp_spiffs.h"
#include "esp_log.h"
#include "cJSON.h"

#define WIFI_CONFIG_FILE_PATH "/spiffs/wifi_config.json"
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
        .format_if_mount_failed = true // Format if mount fails
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
        config.ssid = NULL;
    }
    if (config.pass) {
        free(config.pass);
        config.pass = NULL;
    }
}

bool config_wifi_load() {
    ESP_LOGI(TAG, "Loading Wi-Fi configuration");

    FILE* f = fopen(WIFI_CONFIG_FILE_PATH, "r");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for reading");
        return false;
    }

    fseek(f, 0, SEEK_END);
    size_t size = ftell(f);
    fseek(f, 0, SEEK_SET);

    char* buffer = (char*)malloc(size + 1); // +1 for null termination
    if (!buffer) {
        ESP_LOGE(TAG, "Failed to allocate memory for buffer");
        fclose(f);
        return false;
    }

    ESP_LOGI(TAG, "File size: %d", size);

    fread(buffer, 1, size, f);
    buffer[size] = '\0'; // Null terminate the buffer
    fclose(f);

    ESP_LOGI(TAG, "File content: %s", buffer);

    // Parse JSON
    cJSON* json = cJSON_Parse(buffer);
    free(buffer);
    if (json == NULL) {
        ESP_LOGE(TAG, "Failed to parse JSON");
        return false;
    }

    // Extract SSID and password from the JSON object
    cJSON* ssid_item = cJSON_GetObjectItem(json, "ssid");
    cJSON* pass_item = cJSON_GetObjectItem(json, "pass");

    if (cJSON_IsString(ssid_item) && (ssid_item->valuestring != NULL)) {
        ESP_LOGI(TAG, "SSID: %s", ssid_item->valuestring);
        strlcpy(config.ssid, ssid_item->valuestring, WIFI_SSID_MAX_LEN);
    }

    if (cJSON_IsString(pass_item) && (pass_item->valuestring != NULL)) {
        ESP_LOGI(TAG, "Password: %s", pass_item->valuestring);
        strlcpy(config.pass, pass_item->valuestring, WIFI_PASS_MAX_LEN);
    }

    cJSON_Delete(json); // Clean up the cJSON object

    if (config.ssid[0] == '\0' || config.pass[0] == '\0') {
        ESP_LOGE(TAG, "Failed to load Wi-Fi configuration");
        return false;
    }

    ESP_LOGI("CONFIG", "Config loaded");
    return true;
}

bool config_wifi_save(const char* ssid, const char* pass) {
    ESP_LOGI(TAG, "Saving Wi-Fi configuration");

    cJSON* json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "ssid", ssid);
    cJSON_AddStringToObject(json, "pass", pass);

    char* json_str = cJSON_Print(json);
    cJSON_Delete(json);

    FILE* f = fopen(WIFI_CONFIG_FILE_PATH, "w");
    if (f == NULL) {
        ESP_LOGE(TAG, "Failed to open file for writing");
        return false;
    }

    fwrite(json_str, 1, strlen(json_str), f);
    fclose(f);
    free(json_str);

    strlcpy(config.ssid, ssid, sizeof(config.ssid));
    strlcpy(config.pass, pass, sizeof(config.pass));

    ESP_LOGI(TAG, "Config saved");
    return true;
}

const char* config_wifi_get_ssid(void) {
    return config.ssid;
}

const char* config_wifi_get_pass(void) {
    return config.pass;
}
