/* NimBLE stack APIs */
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "cJSON.h"
#include "esp_bt.h"
#include "esp_random.h"

#include "gatt_svc.h"
#include "config_manager.h"

#define WIFI_SSID_MAX_LEN 64
#define WIFI_PASS_MAX_LEN 64

static const char* TAG = "GATT_SVR";

/* Private function declarations */
static void wifi_cred_svc_access(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg);
// static void wifi_ssid_chr_access(uint16_t conn_handle, uint16_t attr_handle,
//                           struct ble_gatt_access_ctxt *ctxt, void *arg);

// static void wifi_pass_chr_access(uint16_t conn_handle, uint16_t attr_handle,
//                           struct ble_gatt_access_ctxt *ctxt, void *arg);

/* Private variables */

/* Wifi service */
static const ble_uuid16_t wifi_cred_svc_uuid = 
    BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
                     0x12, 0x12, 0x25, 0x15, 0x00, 0x01);

static uint16_t wifi_ssid_chr_val_handle;
static const ble_uuid128_t wifi_ssid_chr_uuid =
    BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
                     0x12, 0x12, 0x25, 0x15, 0x00, 0x02);

static uint16_t wifi_pass_chr_val_handle;
static const ble_uuid128_t wifi_pass_chr_uuid =
    BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
                     0x12, 0x12, 0x25, 0x15, 0x00, 0x03);

/* GATT services table */
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    /* Wifi cred service */
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &wifi_cred_svc_uuid.u,
        .characteristics =
            (struct ble_gatt_chr_def[])
            {
                {/* Wifi ssid characteristic */
                    .uuid = &wifi_ssid_chr_uuid.u,
                    .access_cb = wifi_cred_svc_access,
                    .flags = BLE_GATT_CHR_F_WRITE,
                    .val_handle = &wifi_ssid_chr_val_handle},
                {/* Wifi pass characteristic */
                    .uuid = &wifi_pass_chr_uuid.u,
                    .access_cb = wifi_cred_svc_access,
                    .flags = BLE_GATT_CHR_F_WRITE,
                    .val_handle = &wifi_pass_chr_val_handle},
                {
                    0 /* No more characteristics. */
                }
            },
    },

    {
        0, /* No more services. */
    },
};

static void wifi_cred_svc_access(uint16_t conn_handle, uint16_t attr_handle,
                            struct ble_gatt_access_ctxt *ctxt, void *arg) {

    /* Handle access events */
    /* Note: Wifi cred characteristic is write only */
    switch (ctxt->op) {

    /* Write characteristic event */
    case BLE_GATT_ACCESS_OP_WRITE_CHR:
        /* Verify connection handle */
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(TAG, "Wi-Fi credentials write; conn_handle=%d, attr_handle=%d", conn_handle, attr_handle);
        } else {
            ESP_LOGI(TAG, "Characteristic write by nimble stack with no connection; attr_handle=%d", attr_handle);
        }

        if (attr_handle == wifi_ssid_chr_val_handle) {
            // Allocate a buffer to hold the incoming data
            ESP_LOGI(TAG, "Received data length: %d", ctxt->om->om_len);

            char buffer[ctxt->om->om_len + 1]; // +1 for null-termination
            memcpy(buffer, ctxt->om->om_data, ctxt->om->om_len);
            buffer[ctxt->om->om_len] = '\0'; // Null-terminate the buffer

            ESP_LOGI(TAG, "Received (ssid) data: %s", buffer);

            config_wifi_ssid_save(buffer);

        } else if (attr_handle == wifi_pass_chr_val_handle) {
            // Allocate a buffer to hold the incoming data
            ESP_LOGI(TAG, "Received data length: %d", ctxt->om->om_len);

            char buffer[ctxt->om->om_len + 1]; // +1 for null-termination
            memcpy(buffer, ctxt->om->om_data, ctxt->om->om_len);
            buffer[ctxt->om->om_len] = '\0'; // Null-terminate the buffer

            config_wifi_pass_save(buffer);

            ESP_LOGI(TAG, "Received (pass) data: %s", buffer);
        }
    }
}

/*
 *  GATT server initialization
 *      1. Initialize GATT service
 *      2. Update NimBLE host GATT services counter
 *      3. Add GATT services to server
 *      4. Create a task to send heart rate indication
 */
int gatt_svc_init(void) {
    /* Local variables */
    int rc;

    /* 1. GATT service initialization */
    ble_svc_gatt_init();

    /* 2. Update GATT services counter */
    rc = ble_gatts_count_cfg(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    /* 3. Add GATT services */
    rc = ble_gatts_add_svcs(gatt_svr_svcs);
    if (rc != 0) {
        return rc;
    }

    return 0;
}