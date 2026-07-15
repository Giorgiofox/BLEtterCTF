#include <assert.h>
#include <string.h>

#include "esp_log.h"
#include "nvs_flash.h"

#include "nimble/nimble_port.h"
#include "nimble/nimble_port_freertos.h"
#include "host/ble_hs.h"
#include "host/util/util.h"
#include "services/gap/ble_svc_gap.h"
#include "services/gatt/ble_svc_gatt.h"

#include "blectf.h"

static const char *TAG = "blectf";
static uint8_t     g_own_addr_type;

/* provided by NimBLE store/config */
void ble_store_config_init(void);

static int blectf_gap_event(struct ble_gap_event *event, void *arg);

static void blectf_advertise(void)
{
    struct ble_hs_adv_fields  fields;
    struct ble_gap_adv_params adv_params;
    int rc;

    memset(&fields, 0, sizeof fields);
    fields.flags = BLE_HS_ADV_F_DISC_GEN | BLE_HS_ADV_F_BREDR_UNSUP;
    fields.name = (uint8_t *)BLECTF_DEVICE_NAME;
    fields.name_len = strlen(BLECTF_DEVICE_NAME);
    fields.name_is_complete = 1;

    /* T2: a flag rides in the manufacturer-specific data */
    fields.mfg_data = blectf_mfg_data();
    fields.mfg_data_len = blectf_mfg_data_len();

    rc = ble_gap_adv_set_fields(&fields);
    if (rc != 0) {
        ESP_LOGE(TAG, "adv_set_fields rc=%d", rc);
        return;
    }

    memset(&adv_params, 0, sizeof adv_params);
    adv_params.conn_mode = BLE_GAP_CONN_MODE_UND;
    adv_params.disc_mode = BLE_GAP_DISC_MODE_GEN;

    rc = ble_gap_adv_start(g_own_addr_type, NULL, BLE_HS_FOREVER,
                           &adv_params, blectf_gap_event, NULL);
    if (rc != 0) {
        ESP_LOGE(TAG, "adv_start rc=%d", rc);
    }
}

static int blectf_gap_event(struct ble_gap_event *event, void *arg)
{
    switch (event->type) {
    case BLE_GAP_EVENT_CONNECT:
        ESP_LOGI(TAG, "connect %s",
                 event->connect.status == 0 ? "established" : "failed");
        if (event->connect.status != 0) {
            blectf_advertise();
        }
        break;

    case BLE_GAP_EVENT_DISCONNECT:
        ESP_LOGI(TAG, "disconnect; reason=%d", event->disconnect.reason);
        blectf_advertise();
        break;

    case BLE_GAP_EVENT_ADV_COMPLETE:
        blectf_advertise();
        break;

    case BLE_GAP_EVENT_SUBSCRIBE:
        blectf_on_subscribe(event->subscribe.conn_handle,
                            event->subscribe.attr_handle,
                            event->subscribe.cur_notify);
        break;

    default:
        break;
    }
    return 0;
}

static void blectf_on_sync(void)
{
    int rc = ble_hs_util_ensure_addr(0);
    assert(rc == 0);

    rc = ble_hs_id_infer_auto(0, &g_own_addr_type);
    if (rc != 0) {
        ESP_LOGE(TAG, "infer addr rc=%d", rc);
        return;
    }
    blectf_advertise();
}

static void blectf_on_reset(int reason)
{
    ESP_LOGE(TAG, "nimble reset; reason=%d", reason);
}

static void blectf_host_task(void *param)
{
    nimble_port_run();
    nimble_port_freertos_deinit();
}

void app_main(void)
{
    int rc = nvs_flash_init();
    if (rc == ESP_ERR_NVS_NO_FREE_PAGES || rc == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        ESP_ERROR_CHECK(nvs_flash_init());
    }

    ESP_ERROR_CHECK(nimble_port_init());

    ble_hs_cfg.sync_cb  = blectf_on_sync;
    ble_hs_cfg.reset_cb = blectf_on_reset;
    ble_hs_cfg.store_status_cb = ble_store_util_status_rr;

    /* SMP: enable bonding + LE Secure Connections for the T3 security flags */
    ble_hs_cfg.sm_bonding = 1;
    ble_hs_cfg.sm_sc = 1;
    ble_hs_cfg.sm_our_key_dist   = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;
    ble_hs_cfg.sm_their_key_dist = BLE_SM_PAIR_KEY_DIST_ENC | BLE_SM_PAIR_KEY_DIST_ID;

    ble_svc_gap_init();
    ble_svc_gatt_init();

    rc = blectf_gatt_init();
    assert(rc == 0);

    rc = ble_svc_gap_device_name_set(BLECTF_DEVICE_NAME);
    assert(rc == 0);

    ble_store_config_init();

    nimble_port_freertos_init(blectf_host_task);

    ESP_LOGI(TAG, "BLEtterCTF up: %d flags implemented", blectf_flag_count());
}
