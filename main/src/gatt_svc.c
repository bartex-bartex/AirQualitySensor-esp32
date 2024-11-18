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
static TaskHandle_t xHeartIndicateTask = NULL;

/* Private function declarations */
static void wifi_cred_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg);

static int heart_rate_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                          struct ble_gatt_access_ctxt *ctxt, void *arg);

/* Private variables */
/* Heart Rate service */
static const ble_uuid16_t heart_rate_svc_uuid = BLE_UUID16_INIT(0x180D);

static uint8_t heart_rate_chr_val[2] = {0};
static uint16_t heart_rate_chr_val_handle;
static const ble_uuid16_t heart_rate_chr_uuid = BLE_UUID16_INIT(0x2A37);

static uint16_t heart_rate_chr_conn_handle = 0;
static bool heart_rate_chr_conn_handle_inited = false;
static bool heart_rate_ind_status = false;

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
    /* Heart rate service */
    {.type = BLE_GATT_SVC_TYPE_PRIMARY,
     .uuid = &heart_rate_svc_uuid.u,
     .characteristics =
         (struct ble_gatt_chr_def[]){
             {/* Heart rate characteristic */
              .uuid = &heart_rate_chr_uuid.u,
              .access_cb = heart_rate_chr_access,
              .flags = BLE_GATT_CHR_F_READ | BLE_GATT_CHR_F_INDICATE,
              .val_handle = &heart_rate_chr_val_handle},
             {
                 0, /* No more characteristics in this service. */
             }}},
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

static int heart_rate_chr_access(uint16_t conn_handle, uint16_t attr_handle,
                                 struct ble_gatt_access_ctxt *ctxt, void *arg) {
    /* Local variables */
    int rc;

    /* Handle access events */
    /* Note: Heart rate characteristic is read only */
    switch (ctxt->op) {

    /* Read characteristic event */
    case BLE_GATT_ACCESS_OP_READ_CHR:
        /* Verify connection handle */
        if (conn_handle != BLE_HS_CONN_HANDLE_NONE) {
            ESP_LOGI(TAG, "characteristic read; conn_handle=%d attr_handle=%d",
                     conn_handle, attr_handle);
        } else {
            ESP_LOGI(TAG, "characteristic read by nimble stack; attr_handle=%d",
                     attr_handle);
        }

        /* Verify attribute handle */
        if (attr_handle == heart_rate_chr_val_handle) {
            /* Update access buffer value */
            heart_rate_chr_val[1] = 60 + (uint8_t)(esp_random() % 21);
            rc = os_mbuf_append(ctxt->om, &heart_rate_chr_val,
                                sizeof(heart_rate_chr_val));
            return rc == 0 ? 0 : BLE_ATT_ERR_INSUFFICIENT_RES;
        }
        goto error;

    /* Unknown event */
    default:
        goto error;
    }

error:
    ESP_LOGE(
        TAG,
        "unexpected access operation to heart rate characteristic, opcode: %d",
        ctxt->op);
    return BLE_ATT_ERR_UNLIKELY;
}

/* Public functions */
void send_heart_rate_indication(void) {
    if (heart_rate_ind_status && heart_rate_chr_conn_handle_inited) {
        ble_gatts_indicate(heart_rate_chr_conn_handle,
                           heart_rate_chr_val_handle);
        ESP_LOGI(TAG, "heart rate indication sent!");
    }
}

void heart_rate_ind_task(void *pvParameter) {
    while (1) {
        send_heart_rate_indication();
        vTaskDelay(1000 / portTICK_PERIOD_MS);
    }
}

/*
 *  Action when someone subscribes to characteristics
 */ 
void gatt_svr_subscribe_cb(struct ble_gap_event *event) {
    /* Check connection handle */
    if (event->subscribe.conn_handle != BLE_HS_CONN_HANDLE_NONE) {
        ESP_LOGI(TAG, "subscribe event; conn_handle=%d attr_handle=%d",
                 event->subscribe.conn_handle, event->subscribe.attr_handle);
    } else {
        ESP_LOGI(TAG, "subscribe by nimble stack; attr_handle=%d",
                 event->subscribe.attr_handle);
    }

    /* Check attribute handle */
    if (event->subscribe.attr_handle == heart_rate_chr_val_handle) {
        /* Update heart rate subscription status */
        heart_rate_chr_conn_handle = event->subscribe.conn_handle;
        heart_rate_chr_conn_handle_inited = true;
        heart_rate_ind_status = event->subscribe.cur_indicate;

        /* Create a task to send heart rate indication */
        if (heart_rate_ind_status && xHeartIndicateTask == NULL) {
            ESP_LOGI(TAG, "Start heart rate indication task");
            xTaskCreate(heart_rate_ind_task, "HeartRateIndTask", 4096, NULL, 5, &xHeartIndicateTask);
        } else {
            ESP_LOGI(TAG, "Stop heart rate indication task");
            vTaskDelete(xHeartIndicateTask);
            xHeartIndicateTask = NULL;
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