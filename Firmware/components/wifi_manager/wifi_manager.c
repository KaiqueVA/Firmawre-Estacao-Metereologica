// ============================ Includes ============================
#include <string.h>
#include <stdio.h>
#include "wifi_manager.h"
#include "sdkconfig.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_err.h"
#include "esp_check.h"

// ============================ Defines =============================
#define TAG "wifi_manager"

#ifndef WIFI_AP_DEFAULT_CHANNEL
#define WIFI_AP_DEFAULT_CHANNEL   1
#endif

#ifndef WIFI_AP_DEFAULT_MAX_CONN
#define WIFI_AP_DEFAULT_MAX_CONN  4
#endif

#define WIFI_SSID_MAXLEN 32
#define WIFI_PASS_MAXLEN 64

// NVS namespace/chaves (para salvar credenciais, se habilitado)
#define NVS_NS_WIFI   "wifi_cfg"
#define NVS_KEY_SSID  "ssid"
#define NVS_KEY_PASS  "pass"

// ========================= Variáveis Globais =======================
static wifi_cfg_t g_cfg;
static bool g_wifi_inited  = false;
static bool g_wifi_started = false;
static bool g_sta_connected = false;

static esp_netif_t *s_netif_sta = NULL;
static esp_netif_t *s_netif_ap  = NULL;

static char s_sta_ssid[WIFI_SSID_MAXLEN + 1] = {0};
static char s_sta_pass[WIFI_PASS_MAXLEN + 1] = {0};
static char s_ap_ssid [WIFI_SSID_MAXLEN + 1] = {0};
static char s_ap_pass [WIFI_PASS_MAXLEN + 1] = {0};
static char s_last_ip  [16] = {0}; // "xxx.xxx.xxx.xxx"

// ====================== Protótipos Internos ========================
static esp_err_t prv_nvs_init(void);
static void      prv_register_handlers(void);
static esp_err_t prv_create_netifs(wifi_app_mode_t mode);
static esp_err_t prv_destroy_unused_netifs(wifi_app_mode_t mode);
static esp_err_t prv_apply_mode_initial(wifi_app_mode_t mode);
static esp_err_t prv_apply_mode_runtime(wifi_app_mode_t mode);
static void      prv_wifi_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data);
static void      prv_ip_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data);
static esp_err_t prv_save_credentials_nvs(const char *ssid, const char *pass);
static esp_err_t prv_load_credentials_nvs(char *ssid, size_t ssid_sz, char *pass, size_t pass_sz);
void task_wifi_manager(void *pvParameters);


// ============================ Implementação pública =================
esp_err_t wifi_manager_init(const wifi_cfg_t *cfg)
{
    esp_err_t err;

    // --------- 1) NVS ----------
    err = prv_nvs_init();
    if (err != ESP_OK) {
        ESP_LOGE(TAG, "NVS init falhou: %s", esp_err_to_name(err));
        return err;
    }

    // --------- 2) NETIF + EVENT LOOP ----------
    ESP_RETURN_ON_ERROR(esp_netif_init(), TAG, "esp_netif_init");
    ESP_RETURN_ON_ERROR(esp_event_loop_create_default(), TAG, "event_loop_create_default");

    // --------- 3) Copiar cfg / fallback menuconfig ----------
    memset(&g_cfg, 0, sizeof(g_cfg));

    if (cfg) {
        g_cfg = *cfg;
        if (cfg->sta_ssid) strncpy(s_sta_ssid, cfg->sta_ssid, WIFI_SSID_MAXLEN);
        if (cfg->sta_pass) strncpy(s_sta_pass, cfg->sta_pass, WIFI_PASS_MAXLEN);
        if (cfg->ap_ssid)  strncpy(s_ap_ssid,  cfg->ap_ssid,  WIFI_SSID_MAXLEN);
        if (cfg->ap_pass)  strncpy(s_ap_pass,  cfg->ap_pass,  WIFI_PASS_MAXLEN);
        s_sta_ssid[WIFI_SSID_MAXLEN] = 0;
        s_sta_pass[WIFI_PASS_MAXLEN] = 0;
        s_ap_ssid [WIFI_SSID_MAXLEN] = 0;
        s_ap_pass [WIFI_PASS_MAXLEN] = 0;

        g_cfg.sta_ssid = s_sta_ssid;
        g_cfg.sta_pass = s_sta_pass;
        g_cfg.ap_ssid  = s_ap_ssid;
        g_cfg.ap_pass  = s_ap_pass;

        if (g_cfg.ap_channel == 0)  g_cfg.ap_channel = WIFI_AP_DEFAULT_CHANNEL;
        if (g_cfg.ap_max_conn == 0) g_cfg.ap_max_conn = WIFI_AP_DEFAULT_MAX_CONN;
    } else {
        ESP_LOGI(TAG, "Usando configurações padrão do menuconfig");
        ESP_LOGI(TAG, "Modo Wi-Fi: %d, | SSID: %s | pass: %s", CONFIG_WIFI_DEFAULT_MODE, CONFIG_WIFI_SSID, CONFIG_WIFI_PASSWORD);
        #if   (CONFIG_WIFI_DEFAULT_MODE == 0)
            #define WIFI_DEFAULT_MODE_ENUM  WIFI_MODE_APP_AP
        #elif (CONFIG_WIFI_DEFAULT_MODE == 1)
            #define WIFI_DEFAULT_MODE_ENUM  WIFI_MODE_APP_STA
        #elif (CONFIG_WIFI_DEFAULT_MODE == 2)
            #define WIFI_DEFAULT_MODE_ENUM  WIFI_MODE_APP_APSTA
            //configurar o ponto de acesso futuramente
        #else
            #error "CONFIG_WIFI_DEFAULT_MODE inválido (use 0=AP, 1=STA, 2=AP+STA)"
        #endif

        g_cfg.mode = WIFI_DEFAULT_MODE_ENUM;
        
        #ifdef CONFIG_WIFI_SSID
            strncpy(s_sta_ssid, CONFIG_WIFI_SSID, WIFI_SSID_MAXLEN);
        #endif
        #ifdef CONFIG_WIFI_PASSWORD
            strncpy(s_sta_pass, CONFIG_WIFI_PASSWORD, WIFI_PASS_MAXLEN);
        #endif
        #ifdef CONFIG_WIFI_AP_SSID
            strncpy(s_ap_ssid, CONFIG_WIFI_AP_SSID, WIFI_SSID_MAXLEN);
        #endif
        #ifdef CONFIG_WIFI_AP_PASSWORD
            strncpy(s_ap_pass, CONFIG_WIFI_AP_PASSWORD, WIFI_PASS_MAXLEN);
        #endif

        s_sta_ssid[WIFI_SSID_MAXLEN] = 0;
        s_sta_pass[WIFI_PASS_MAXLEN] = 0;
        s_ap_ssid [WIFI_SSID_MAXLEN] = 0;
        s_ap_pass [WIFI_PASS_MAXLEN] = 0;

        g_cfg.sta_ssid    = s_sta_ssid;
        g_cfg.sta_pass    = s_sta_pass;
        g_cfg.ap_ssid     = s_ap_ssid;
        g_cfg.ap_pass     = s_ap_pass;
        g_cfg.ap_channel  = WIFI_AP_DEFAULT_CHANNEL;
        g_cfg.ap_max_conn = WIFI_AP_DEFAULT_MAX_CONN;
        g_cfg.save_to_nvs = false;

        // opcional: tentar carregar credenciais do NVS (se existirem)
        (void)prv_load_credentials_nvs(s_sta_ssid, sizeof(s_sta_ssid), s_sta_pass, sizeof(s_sta_pass));
    }

    // --------- 4) Handlers ----------
    prv_register_handlers();

    // --------- 5) Netifs ----------
    ESP_RETURN_ON_ERROR(prv_create_netifs(g_cfg.mode), TAG, "create_netifs");

    // --------- 6) Wi-Fi Driver ----------
    wifi_init_config_t wicfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_RETURN_ON_ERROR(esp_wifi_init(&wicfg), TAG, "esp_wifi_init");
    g_wifi_inited = true;

    ESP_RETURN_ON_ERROR(esp_wifi_set_storage(WIFI_STORAGE_RAM), TAG, "wifi_set_storage");

    // --------- 7) Aplicar modo inicial ----------
    ESP_RETURN_ON_ERROR(prv_apply_mode_initial(g_cfg.mode), TAG, "apply_mode_initial");

    ESP_LOGI(TAG, "init ok (modo=%d, STA:'%s', AP:'%s')",
             (int)g_cfg.mode, g_cfg.sta_ssid ? g_cfg.sta_ssid : "",
             g_cfg.ap_ssid ? g_cfg.ap_ssid : "");

    return ESP_OK;
}

// ============================ Funções restantes (públicas) =========

// --------- start ----------
esp_err_t wifi_manager_start(void)
{
    if (!g_wifi_inited) return ESP_ERR_INVALID_STATE;
    if (g_wifi_started) return ESP_OK;

    ESP_RETURN_ON_ERROR(esp_wifi_start(), TAG, "esp_wifi_start");
    g_wifi_started = true;

    // Se STA ativo, iniciar conexão
    wifi_mode_t mode;
    esp_wifi_get_mode(&mode);
    if (mode == WIFI_MODE_STA || mode == WIFI_MODE_APSTA) {
        esp_wifi_connect();
    }

    xTaskCreate(task_wifi_manager, "wifi_manager_task", 4096, NULL, 5, NULL);
    return ESP_OK;
}

void task_wifi_manager(void *pvParameters)
{
    while (1) {
        if (wifi_manager_is_connected()) {
        char ip[16];
        if (wifi_manager_get_ip(ip, sizeof ip) == ESP_OK) ESP_LOGI("APP", "IP=%s", ip);
    }
        vTaskDelay(pdMS_TO_TICKS(100000)); // Exemplo: aguarda 100 segundos
    }
}

// --------- stop ----------
esp_err_t wifi_manager_stop(void)
{
    if (!g_wifi_inited) return ESP_ERR_INVALID_STATE;
    if (!g_wifi_started) return ESP_OK;

    ESP_RETURN_ON_ERROR(esp_wifi_stop(), TAG, "esp_wifi_stop");
    g_wifi_started = false;
    g_sta_connected = false;
    s_last_ip[0] = '\0';
    return ESP_OK;
}

// --------- set_mode (AP/STA/APSTA) ----------
esp_err_t wifi_manager_set_mode(wifi_app_mode_t mode)
{
    if (!g_wifi_inited) return ESP_ERR_INVALID_STATE;

    // Se nada mudou, ignore
    if (mode == g_cfg.mode) return ESP_OK;

    // Parar Wi-Fi se estiver rodando
    bool was_started = g_wifi_started;
    if (was_started) {
        ESP_RETURN_ON_ERROR(wifi_manager_stop(), TAG, "stop_before_set_mode");
    }

    // Criar/destruir netifs conforme necessário
    ESP_RETURN_ON_ERROR(prv_create_netifs(mode), TAG, "create_netifs");
    ESP_RETURN_ON_ERROR(prv_destroy_unused_netifs(mode), TAG, "destroy_unused_netifs");

    // Aplicar no driver
    ESP_RETURN_ON_ERROR(prv_apply_mode_runtime(mode), TAG, "apply_mode_runtime");

    // Atualizar cfg
    g_cfg.mode = mode;

    // Subir novamente se estava rodando
    if (was_started) {
        ESP_RETURN_ON_ERROR(wifi_manager_start(), TAG, "restart_after_set_mode");
    }
    return ESP_OK;
}

// --------- set_sta_credentials ----------
esp_err_t wifi_manager_set_sta_credentials(const char *ssid, const char *pass)
{
    if (!ssid) return ESP_ERR_INVALID_ARG;
    size_t ssid_len = strnlen(ssid, WIFI_SSID_MAXLEN + 1);
    size_t pass_len = pass ? strnlen(pass, WIFI_PASS_MAXLEN + 1) : 0;
    if (ssid_len == 0 || ssid_len > WIFI_SSID_MAXLEN) return ESP_ERR_INVALID_ARG;
    if (pass_len > WIFI_PASS_MAXLEN) return ESP_ERR_INVALID_ARG;

    // Atualiza buffers internos
    memset(s_sta_ssid, 0, sizeof(s_sta_ssid));
    memset(s_sta_pass, 0, sizeof(s_sta_pass));
    strncpy(s_sta_ssid, ssid, WIFI_SSID_MAXLEN);
    if (pass) strncpy(s_sta_pass, pass, WIFI_PASS_MAXLEN);

    // Apontar na cfg
    g_cfg.sta_ssid = s_sta_ssid;
    g_cfg.sta_pass = s_sta_pass;

    // Aplicar no driver (em RAM)
    wifi_config_t wc = {0};
    strncpy((char*)wc.sta.ssid,     s_sta_ssid, sizeof(wc.sta.ssid));
    strncpy((char*)wc.sta.password, s_sta_pass, sizeof(wc.sta.password));
    wc.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
    wc.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;

    ESP_RETURN_ON_ERROR(esp_wifi_set_config(WIFI_IF_STA, &wc), TAG, "set_config_sta");

    // Persistir no NVS se habilitado
    if (g_cfg.save_to_nvs) {
        (void)prv_save_credentials_nvs(s_sta_ssid, s_sta_pass);
    }

    // Se Wi-Fi ativo em STA/APSTA, tentar reconectar
    if (g_wifi_started) {
        wifi_mode_t mode;
        esp_wifi_get_mode(&mode);
        if (mode == WIFI_MODE_STA || mode == WIFI_MODE_APSTA) {
            g_sta_connected = false;
            s_last_ip[0] = '\0';
            esp_wifi_disconnect();
            esp_wifi_connect();
        }
    }

    return ESP_OK;
}

// --------- get_ip (retorna IP da STA como string) ----------
esp_err_t wifi_manager_get_ip(char *buf, size_t len)
{
    if (!buf || len < 2) return ESP_ERR_INVALID_ARG;

    buf[0] = '\0';

    // Preferir cache local se já temos IP
    if (s_last_ip[0]) {
        snprintf(buf, len, "%s", s_last_ip);
        return ESP_OK;
    }

    if (!s_netif_sta) return ESP_ERR_INVALID_STATE;

    esp_netif_ip_info_t ip;
    if (esp_netif_get_ip_info(s_netif_sta, &ip) != ESP_OK) {
        return ESP_FAIL;
    }

    uint32_t a = ip.ip.addr;
    snprintf(buf, len, "%u.%u.%u.%u",
             (unsigned)((a >> 0) & 0xFF),
             (unsigned)((a >> 8) & 0xFF),
             (unsigned)((a >> 16) & 0xFF),
             (unsigned)((a >> 24) & 0xFF));

    return ESP_OK;
}

// --------- is_connected (STA) ----------
bool wifi_manager_is_connected(void)
{
    return g_sta_connected;
}

// ============================ Internas (apoio) =====================
static esp_err_t prv_nvs_init(void)
{
    esp_err_t err = nvs_flash_init();
    if (err == ESP_ERR_NVS_NO_FREE_PAGES || err == ESP_ERR_NVS_NEW_VERSION_FOUND) {
        ESP_ERROR_CHECK(nvs_flash_erase());
        err = nvs_flash_init();
    }
    return err;
}

static void prv_register_handlers(void)
{
    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        WIFI_EVENT, ESP_EVENT_ANY_ID, &prv_wifi_event_handler, NULL, NULL));

    ESP_ERROR_CHECK(esp_event_handler_instance_register(
        IP_EVENT, IP_EVENT_STA_GOT_IP, &prv_ip_event_handler, NULL, NULL));
}

static esp_err_t prv_create_netifs(wifi_app_mode_t mode)
{
    if ((mode == WIFI_MODE_APP_STA) || (mode == WIFI_MODE_APP_APSTA)) {
        if (!s_netif_sta) {
            s_netif_sta = esp_netif_create_default_wifi_sta();
            if (!s_netif_sta) return ESP_FAIL;
        }
    }
    if ((mode == WIFI_MODE_APP_AP) || (mode == WIFI_MODE_APP_APSTA)) {
        if (!s_netif_ap) {
            s_netif_ap = esp_netif_create_default_wifi_ap();
            if (!s_netif_ap) return ESP_FAIL;
        }
    }
    return ESP_OK;
}

static esp_err_t prv_destroy_unused_netifs(wifi_app_mode_t mode)
{
    // (Opcional) destruir netif não usado — geralmente não é necessário,
    // mas pode liberar RAM se alternar muito.
    if (mode == WIFI_MODE_APP_STA) {
        if (s_netif_ap) {
            esp_netif_destroy(s_netif_ap);
            s_netif_ap = NULL;
        }
    } else if (mode == WIFI_MODE_APP_AP) {
        if (s_netif_sta) {
            esp_netif_destroy(s_netif_sta);
            s_netif_sta = NULL;
        }
    }
    return ESP_OK;
}

static esp_err_t prv_apply_mode_initial(wifi_app_mode_t mode)
{
    wifi_mode_t idf_mode = WIFI_MODE_NULL;
    switch (mode) {
        case WIFI_MODE_APP_AP:    idf_mode = WIFI_MODE_AP;    break;
        case WIFI_MODE_APP_STA:   idf_mode = WIFI_MODE_STA;   break;
        case WIFI_MODE_APP_APSTA: idf_mode = WIFI_MODE_APSTA; break;
        default:                  idf_mode = WIFI_MODE_STA;   break;
    }
    ESP_ERROR_CHECK(esp_wifi_set_mode(idf_mode));

    // STA
    if (idf_mode == WIFI_MODE_STA || idf_mode == WIFI_MODE_APSTA) {
        wifi_config_t sta = {0};
        strncpy((char*)sta.sta.ssid,     g_cfg.sta_ssid ? g_cfg.sta_ssid : "", sizeof(sta.sta.ssid));
        strncpy((char*)sta.sta.password, g_cfg.sta_pass ? g_cfg.sta_pass : "", sizeof(sta.sta.password));
        sta.sta.threshold.authmode = WIFI_AUTH_WPA2_PSK;
        sta.sta.sae_pwe_h2e = WPA3_SAE_PWE_BOTH;
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &sta));
    }

    // AP
    if (idf_mode == WIFI_MODE_AP || idf_mode == WIFI_MODE_APSTA) {
        wifi_config_t ap = {0};
        strncpy((char*)ap.ap.ssid,     g_cfg.ap_ssid ? g_cfg.ap_ssid : "ESP32-AP", sizeof(ap.ap.ssid));
        strncpy((char*)ap.ap.password, g_cfg.ap_pass ? g_cfg.ap_pass : "",         sizeof(ap.ap.password));
        ap.ap.ssid_len       = strlen((char*)ap.ap.ssid);
        ap.ap.channel        = g_cfg.ap_channel ? g_cfg.ap_channel : WIFI_AP_DEFAULT_CHANNEL;
        ap.ap.max_connection = g_cfg.ap_max_conn ? g_cfg.ap_max_conn : WIFI_AP_DEFAULT_MAX_CONN;
        ap.ap.authmode       = (strlen((char*)ap.ap.password) == 0) ? WIFI_AUTH_OPEN : WIFI_AUTH_WPA_WPA2_PSK;
        ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_AP, &ap));
    }

    return ESP_OK;
}

static esp_err_t prv_apply_mode_runtime(wifi_app_mode_t mode)
{
    // semelhante ao initial, mas usado ao alternar em runtime
    return prv_apply_mode_initial(mode);
}

// ===================== Handlers de Evento ==========================
static void prv_wifi_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data)
{
    switch (id) {
    case WIFI_EVENT_STA_START:
        ESP_LOGI(TAG, "STA_START");
        break;

    case WIFI_EVENT_STA_CONNECTED:
        ESP_LOGI(TAG, "STA_CONNECTED");
        break;

    case WIFI_EVENT_STA_DISCONNECTED:
        ESP_LOGW(TAG, "STA_DISCONNECTED - tentando reconectar...");
        g_sta_connected = false;
        s_last_ip[0] = '\0';
        // política simples de reconexão
        esp_wifi_connect();
        break;

    case WIFI_EVENT_AP_START:
        ESP_LOGI(TAG, "AP_START");
        break;

    case WIFI_EVENT_AP_STACONNECTED:
        ESP_LOGI(TAG, "AP_STACONNECTED");
        break;

    case WIFI_EVENT_AP_STADISCONNECTED:
        ESP_LOGI(TAG, "AP_STADISCONNECTED");
        break;

    default:
        break;
    }
}

static void prv_ip_event_handler(void* arg, esp_event_base_t base, int32_t id, void* data)
{
    if (id == IP_EVENT_STA_GOT_IP) {
        const ip_event_got_ip_t *event = (const ip_event_got_ip_t *)data;
        uint32_t a = event->ip_info.ip.addr;
        snprintf(s_last_ip, sizeof(s_last_ip), "%u.%u.%u.%u",
                 (unsigned)((a >> 0) & 0xFF),
                 (unsigned)((a >> 8) & 0xFF),
                 (unsigned)((a >> 16) & 0xFF),
                 (unsigned)((a >> 24) & 0xFF));
        g_sta_connected = true;
        ESP_LOGI(TAG, "STA_GOT_IP: %s", s_last_ip);
    }
}

// ===================== Persistência (opcional) =====================
static esp_err_t prv_save_credentials_nvs(const char *ssid, const char *pass)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS_WIFI, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    err = nvs_set_str(h, NVS_KEY_SSID, ssid ? ssid : "");
    if (err == ESP_OK) err = nvs_set_str(h, NVS_KEY_PASS, pass ? pass : "");
    if (err == ESP_OK) err = nvs_commit(h);
    nvs_close(h);
    return err;
}

static esp_err_t prv_load_credentials_nvs(char *ssid, size_t ssid_sz, char *pass, size_t pass_sz)
{
    if (!ssid || !pass) return ESP_ERR_INVALID_ARG;

    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS_WIFI, NVS_READONLY, &h);
    if (err != ESP_OK) return err;

    size_t len = ssid_sz;
    err = nvs_get_str(h, NVS_KEY_SSID, ssid, &len);
    if (err != ESP_OK) { nvs_close(h); return err; }

    len = pass_sz;
    err = nvs_get_str(h, NVS_KEY_PASS, pass, &len);
    nvs_close(h);
    return err;
}
