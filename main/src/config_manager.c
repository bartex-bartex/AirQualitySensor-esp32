#include <string.h>
#include "config_manager.h"
#include "led_manager.h"
#include "esp_log.h"
#include "esp_system.h"
#include "nvs_flash.h"
#include "nvs.h"

#define WIFI_SSID_MAX_LEN 64
#define WIFI_PASS_MAX_LEN 64

static char* ssid;
static char* pass;

static const char* TAG = "CONFIG_MANAGER";
static nvs_handle_t my_nvs_handle;

bool config_init() {
    esp_err_t err;

    err = nvs_open("storage", NVS_READWRITE, &my_nvs_handle);
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) opening NVS handle", esp_err_to_name(err));
        return false;
    }

    // Allocate memory for SSID and password
    ssid = (char*)malloc(WIFI_SSID_MAX_LEN);
    if (ssid == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for SSID");
        return false;
    }
    memset(ssid, 0, WIFI_SSID_MAX_LEN);

    pass = (char*)malloc(WIFI_PASS_MAX_LEN);
    if (pass == NULL) {
        ESP_LOGE(TAG, "Failed to allocate memory for Password");
        free(ssid);  // Clean up previously allocated memory
        return false;
    }
    memset(pass, 0, WIFI_PASS_MAX_LEN);

    ESP_LOGI(TAG, "Configuration initialized successfully");
    return true;
}

void config_cleanup() {
    // Free the dynamically allocated memory
    if (ssid != NULL) {
        free(ssid);
        ssid = NULL;
    }
    
    if (pass != NULL) {
        free(pass);
        pass = NULL;
    }

    // Close NVS storage handle
    nvs_close(my_nvs_handle);
}

bool config_wifi_ssid_load() {
    ESP_LOGI(TAG, "Loading Wi-Fi configuration");

    esp_err_t err;
    size_t required_size;

    err = nvs_get_str(my_nvs_handle, "ssid", NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND) {
        ESP_LOGI(TAG, "No SSID stored in NVS - using default");
        ssid[0] = '\0';
        return true;
    }
    err = nvs_get_str(my_nvs_handle, "ssid", NULL, &required_size);
    free(ssid);
    ssid = (char*)malloc(required_size);
    err = nvs_get_str(my_nvs_handle, "ssid", ssid, &required_size);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) reading ssid from NVS", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI("CONFIG", "SSID loaded: %s", ssid);
    return true;
}

bool config_wifi_pass_load() {
    ESP_LOGI(TAG, "Loading Wi-Fi configuration");

    esp_err_t err;
    size_t required_size;

    err = nvs_get_str(my_nvs_handle, "pass", NULL, &required_size);
    if (err == ESP_ERR_NVS_NOT_FOUND){
        ESP_LOGI(TAG, "No password stored in NVS - using default");
        pass[0] = '\0';
        return true;
    }
    free(pass);
    pass = (char*)malloc(required_size);
    err = nvs_get_str(my_nvs_handle, "pass", pass, &required_size);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) reading pass from NVS", esp_err_to_name(err));
        return false;
    }

    ESP_LOGI("CONFIG", "Password loaded: %s", pass);
    return true;
}

bool config_wifi_ssid_save(const char* ssid_param) {
    ESP_LOGI(TAG, "Saving Wi-Fi configuration");

    esp_err_t err;
    strcpy(ssid, ssid_param);
    err = nvs_set_str(my_nvs_handle, "ssid", ssid_param);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) saving ssid to NVS", esp_err_to_name(err));
        return false;
    }

    nvs_commit(my_nvs_handle);

    ESP_LOGI(TAG, "SSID saved");
    return true;
}

bool config_wifi_pass_save(const char* pass_param) {
    ESP_LOGI(TAG, "Saving Wi-Fi configuration");

    esp_err_t err;
    strcpy(pass, pass_param);
    err = nvs_set_str(my_nvs_handle, "pass", pass_param);

    if (err != ESP_OK) {
        ESP_LOGE(TAG, "Error (%s) saving password to NVS", esp_err_to_name(err));
        return false;
    }

    nvs_commit(my_nvs_handle);

    ESP_LOGI(TAG, "Password saved");
    return true;
}

const char* config_wifi_get_ssid(void) {
    return ssid;
}

const char* config_wifi_get_pass(void) {
    return pass;
}
