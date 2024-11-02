#ifndef CONFIG_MANAGER_H
#define CONFIG_MANAGER_H

#include <stdbool.h>

// Initialization and Cleanup
bool config_init();
void config_cleanup();

// Loading, Saving, and Resetting Configuration
bool config_wifi_load();
bool config_wifi_save(const char* ssid, const char* pass);

// Getters
const char* config_wifi_get_ssid(void);
const char* config_wifi_get_pass(void);

#endif // CONFIG_MANAGER_H

