#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <stdbool.h>

// Initialization and Cleanup
bool config_init();
void config_cleanup();

// Loading, Saving, and Resetting Configuration
bool config_wifi_ssid_load(void);
bool config_wifi_pass_load(void);
bool config_wifi_ssid_save(const char* ssid_param);
bool config_wifi_pass_save(const char* pass_param);
const char* config_mqtt_get_uri(void);
bool config_mqtt_uri_save(const char* uri_param);

// Getters
const char* config_wifi_get_ssid(void);
const char* config_wifi_get_pass(void);

#endif // CONFIG_MANAGER_H

