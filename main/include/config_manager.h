#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <stdbool.h>

// Initialization and Cleanup
bool config_init();
void config_cleanup();

// Loading, Saving, and Resetting Configuration
bool wifi_config_load();
bool wifi_config_save(const char* ssid, const char* pass);

// Getters
const char* wifi_config_get_ssid(void);
const char* wifi_config_get_pass(void);

#endif // CONFIG_MANAGER_H

