/**
 * @file wifi_manager.c
 * @brief WiFi connection management implementation
 * 
 * Handles WiFi station mode initialization, connection with automatic retry,
 * connection monitoring, and event handling for WiFi and IP events.
 */

#include "wifi_manager.h"
#include "config.h"
#include "esp_wifi.h"
#include "esp_event.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/event_groups.h"
#include "nvs_flash.h"

static const char *TAG = "wifi_manager";

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT      BIT1
#define MAX_RETRY_COUNT    100  // Allow many reconnection attempts

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;
static bool s_is_connected = false;

/**
 * @brief WiFi and IP event handler callback
 * 
 * Handles:
 * - WIFI_EVENT_STA_START: Auto-connect on WiFi start
 * - WIFI_EVENT_STA_DISCONNECTED: Retry connection up to MAX_RETRY_COUNT
 * - IP_EVENT_STA_GOT_IP: Mark as connected and log IP address
 * 
 * @param arg User argument (unused)
 * @param event_base Event base (WIFI_EVENT or IP_EVENT)
 * @param event_id Specific event ID
 * @param event_data Event-specific data
 */
static void event_handler(void* arg, esp_event_base_t event_base,
                         int32_t event_id, void* event_data) {
    if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START) {
        esp_wifi_connect();
    } else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED) {
        s_is_connected = false;
        // Reset retry counter to allow continuous reconnection attempts
        s_retry_num = 0;
        if (s_retry_num < MAX_RETRY_COUNT) {
            esp_wifi_connect();
            s_retry_num++;
            ESP_LOGI(TAG, "Retry to connect to the AP (attempt %d/%d)", s_retry_num, MAX_RETRY_COUNT);
        } else {
            xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
            ESP_LOGE(TAG, "Failed to connect to AP");
        }
    } else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP) {
        ip_event_got_ip_t* event = (ip_event_got_ip_t*) event_data;
        //ESP_LOGI(TAG, "Got IP: " IPSTR, IP2STR(&event->ip_info.ip));
        s_retry_num = 0;
        s_is_connected = true;
        xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
    }
}

esp_err_t wifi_manager_init(void) {
    s_wifi_event_group = xEventGroupCreate();
    
    ESP_ERROR_CHECK(esp_netif_init());
    ESP_ERROR_CHECK(esp_event_loop_create_default());
    esp_netif_create_default_wifi_sta();

    wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
    ESP_ERROR_CHECK(esp_wifi_init(&cfg));

    esp_event_handler_instance_t instance_any_id;
    esp_event_handler_instance_t instance_got_ip;
    ESP_ERROR_CHECK(esp_event_handler_instance_register(WIFI_EVENT,
                                                        ESP_EVENT_ANY_ID,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_any_id));
    ESP_ERROR_CHECK(esp_event_handler_instance_register(IP_EVENT,
                                                        IP_EVENT_STA_GOT_IP,
                                                        &event_handler,
                                                        NULL,
                                                        &instance_got_ip));

    //ESP_LOGI(TAG, "WiFi manager initialized");
    return ESP_OK;
}

esp_err_t wifi_manager_connect(void) {
    wifi_config_t wifi_config = {
        .sta = {
            .ssid = WIFI_SSID,
            .password = WIFI_PASSWORD,
            .threshold.authmode = WIFI_AUTH_WPA2_PSK,
            .pmf_cfg = {
                .capable = true,
                .required = false
            },
        },
    };
    
    ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
    ESP_ERROR_CHECK(esp_wifi_set_config(WIFI_IF_STA, &wifi_config));
    ESP_ERROR_CHECK(esp_wifi_start());

    //ESP_LOGI(TAG, "Connecting to WiFi SSID: %s", WIFI_SSID);

    EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
            WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
            pdFALSE,
            pdFALSE,
            portMAX_DELAY);

    if (bits & WIFI_CONNECTED_BIT) {
        //ESP_LOGI(TAG, "Connected to AP successfully");
        
        // Enable minimum modem sleep for light sleep compatibility
        // This allows WiFi to work properly with light sleep
        esp_wifi_set_ps(WIFI_PS_MIN_MODEM);
        ESP_LOGI(TAG, "WiFi power save mode enabled for light sleep");
        
        return ESP_OK;
    } else if (bits & WIFI_FAIL_BIT) {
        ESP_LOGE(TAG, "Failed to connect to AP");
        return ESP_FAIL;
    }

    return ESP_ERR_TIMEOUT;
}

bool wifi_manager_is_connected(void) {
    // Check both our flag and the actual WiFi state
    if (!s_is_connected) {
        return false;
    }
    
    // Also verify the WiFi is actually in connected state
    wifi_ap_record_t ap_info;
    if (esp_wifi_sta_get_ap_info(&ap_info) == ESP_OK) {
        return true;  // Successfully got AP info, we're connected
    }
    
    // If we can't get AP info, we're not really connected
    s_is_connected = false;
    return false;
}

void wifi_manager_disconnect(void) {
    esp_wifi_disconnect();
    s_is_connected = false;
    //ESP_LOGI(TAG, "WiFi disconnected");
}
