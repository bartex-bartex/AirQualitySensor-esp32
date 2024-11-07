#ifndef BLE_MANAGER_H
#define BLE_MANAGER_H

bool ble_init();
void nimble_host_stop_task();
void nimble_host_task(void *param);

#endif // BLE_MANAGER_H