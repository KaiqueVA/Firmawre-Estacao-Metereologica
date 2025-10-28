// ============================ Includes ============================
#include <stdbool.h>
#include <stdint.h>
#include "esp_err.h"
// ============================ Defines =============================


typedef enum {
    WIFI_MODE_APP_AP = 0,
    WIFI_MODE_APP_STA,
    WIFI_MODE_APP_APSTA,
} wifi_app_mode_t;

// ============================ Structs =============================
typedef struct {
    wifi_app_mode_t mode;
    const char *sta_ssid;
    const char *sta_pass;
    const char *ap_ssid;
    const char *ap_pass;
    uint8_t     ap_channel;
    uint8_t     ap_max_conn;
    bool        save_to_nvs;   // salva credenciais ao conectar
} wifi_cfg_t;

// ====================== Protótipos de funções =====================
esp_err_t wifi_manager_init(const wifi_cfg_t *cfg);
esp_err_t wifi_manager_start(void);
esp_err_t wifi_manager_stop(void);
esp_err_t wifi_manager_set_mode(wifi_app_mode_t mode);
esp_err_t wifi_manager_set_sta_credentials(const char *ssid, const char *pass);
esp_err_t wifi_manager_get_ip(char *buf, size_t len);
bool      wifi_manager_is_connected(void);
