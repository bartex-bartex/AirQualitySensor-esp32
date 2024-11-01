#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <stdbool.h>

// Define the struct to hold Wi-Fi credential data
typedef struct {
    char* ssid;
    char* pass;
} wifi_config;

// Initialization and Cleanup
bool config_init(wifi_config* config);
void config_cleanup(wifi_config* config);

// Loading, Saving, and Resetting Configuration
bool wifi_config_load(wifi_config* config);
bool wifi_config_save(wifi_config* config);

#endif // CONFIG_MANAGER_H

