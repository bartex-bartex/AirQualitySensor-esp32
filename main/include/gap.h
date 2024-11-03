#ifndef GAP_SVC_H
#define GAP_SVC_H

/* Includes */
/* NimBLE GAP APIs */
#include "services/gap/ble_svc_gap.h"

/* Defines */
#define BLE_GAP_APPEARANCE_GENERIC_TAG 0x0200  // (512 -> Generic Tag)
#define BLE_GAP_URI_PREFIX_HTTPS 0x17          // (23 -> https://)
#define BLE_GAP_LE_ROLE_PERIPHERAL 0x00

/* Public function declarations */
void adv_init(void);
int gap_init(void);

#endif // GAP_SVC_H