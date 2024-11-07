/* NimBLE stack APIs */
#include "host/ble_hs.h"
#include "host/ble_uuid.h"
#include "host/util/util.h"
#include "nimble/ble.h"
#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "cJSON.h"
#include "esp_bt.h"

#include "gatt_svc.h"
#include "config_manager.h"

#define WIFI_SSID_MAX_LEN 64
#define WIFI_PASS_MAX_LEN 64

static const char* TAG = "GATT_SVR";

/* Private function declarations */
static void wifi_cred_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg);

/* Private variables */

/* Automation IO service */
static const ble_uuid16_t wifi_cred_svc_uuid = 
    BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
                     0x12, 0x12, 0x25, 0x15, 0x00, 0x01);;
static uint16_t wifi_cred_chr_val_handle;
static const ble_uuid128_t wifi_cred_chr_uuid =
    BLE_UUID128_INIT(0x23, 0xd1, 0xbc, 0xea, 0x5f, 0x78, 0x23, 0x15, 0xde, 0xef,
                     0x12, 0x12, 0x25, 0x15, 0x00, 0x02);

/* GATT services table */
static const struct ble_gatt_svc_def gatt_svr_svcs[] = {
    /* Wifi cred service */
    {
        .type = BLE_GATT_SVC_TYPE_PRIMARY,
        .uuid = &wifi_cred_svc_uuid.u,
        .characteristics =
            (struct ble_gatt_chr_def[]){/* Wifi cred characteristic */
                                        {.uuid = &wifi_cred_chr_uuid.u,
                                         .access_cb = wifi_cred_chr_access,
                                         .flags = BLE_GATT_CHR_F_WRITE,
                                         .val_handle = &wifi_cred_chr_val_handle},
                                        {0}},
    },

    {
        0, /* No more services. */
    },
};

static void wifi_cred_chr_access(uint16_t conn_handle, uint16_t attr_handle,
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

        /* Verify attribute handle */
        if (attr_handle == wifi_cred_chr_val_handle) {
            // if (ctxt->om->om_len > WIFI_SSID_MAX_LEN + WIFI_PASS_MAX_LEN + 2) {
            //     ESP_LOGE(TAG, "Write data exceeds max length for SSID and password");
            //     // return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            // }

            // /* Extract the SSID and password from the write buffer */
            // int ssid_len = ctxt->om->om_data[0];  // Length of SSID
            // int pass_len = ctxt->om->om_data[1];  // Length of password

            // /* Validate lengths */
            // if (ssid_len > WIFI_SSID_MAX_LEN || pass_len > WIFI_PASS_MAX_LEN || 
            //     ssid_len + pass_len + 2 > ctxt->om->om_len) {
            //     ESP_LOGE(TAG, "Invalid lengths for SSID or password");
            //     // return BLE_ATT_ERR_INVALID_ATTR_VALUE_LEN;
            // }


            // /* Copy SSID */
            // char ssid[WIFI_SSID_MAX_LEN + 1];
            // char password[WIFI_PASS_MAX_LEN + 1];

            // memcpy(ssid, &ctxt->om->om_data[2], ssid_len);
            // ssid[ssid_len] = '\0'; // Null-terminate
            
            // /* Copy password */
            // memcpy(password, &ctxt->om->om_data[2 + ssid_len], pass_len);
            // password[pass_len] = '\0'; // Null-terminate

            // /* Save Wi-Fi credentials */
            // config_wifi_save(ssid, password);

            // ESP_LOGI(TAG, "Received SSID: %s, Password: %s", ssid, password);

            // Allocate a buffer to hold the incoming data
            ESP_LOGI(TAG, "Received data length: %d", ctxt->om->om_len);

            char buffer[ctxt->om->om_len + 1]; // +1 for null-termination
            memcpy(buffer, ctxt->om->om_data, ctxt->om->om_len);
            buffer[ctxt->om->om_len] = '\0'; // Null-terminate the buffer

            ESP_LOGI(TAG, "Received data: %s", buffer);

            // Parse the JSON string
            cJSON *json = cJSON_Parse(buffer);
            if (json == NULL) {
                ESP_LOGE(TAG, "Failed to parse JSON: %s", cJSON_GetErrorPtr());
                return; // Handle the error appropriately
            }

            // Extract SSID and password items
            cJSON *ssid_item = cJSON_GetObjectItem(json, "ssid");
            cJSON *pass_item = cJSON_GetObjectItem(json, "pass");

            // Check if the extracted items are valid
            if (ssid_item != NULL && cJSON_IsString(ssid_item) && ssid_item->valuestring != NULL) {
                // Process SSID
                ESP_LOGI(TAG, "SSID: %s", ssid_item->valuestring);
            } else {
                ESP_LOGE(TAG, "SSID not found or not a string");
            }

            if (pass_item != NULL && cJSON_IsString(pass_item) && pass_item->valuestring != NULL) {
                // Process Password
                ESP_LOGI(TAG, "Password: %s", pass_item->valuestring);
            } else {
                ESP_LOGE(TAG, "Password not found or not a string");
            }

            config_wifi_save(ssid_item->valuestring, pass_item->valuestring);

            // Clean up the cJSON object
            cJSON_Delete(json);

            // return 0;
        }
    }
}

/*
 *  GATT server initialization
 *      1. Initialize GATT service
 *      2. Update NimBLE host GATT services counter
 *      3. Add GATT services to server
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