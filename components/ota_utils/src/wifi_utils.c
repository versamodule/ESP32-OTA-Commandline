#include "ota_utils.h"

static const char *TAG_WIFI_UTILS = "wifi_utils";

static EventGroupHandle_t s_wifi_event_group;
static int s_retry_num = 0;

void event_handler(void *arg, esp_event_base_t event_base, int32_t event_id, void *event_data)
{
	if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_START)
	{
		esp_wifi_connect();
	}
	else if (event_base == WIFI_EVENT && event_id == WIFI_EVENT_STA_DISCONNECTED)
	{
		if (s_retry_num < WIFI_AP_RECONN_ATTEMPTS)
		{
			esp_wifi_connect();
			s_retry_num++;
			ESP_LOGI(TAG_WIFI_UTILS, "Trying to connect to AP");
		}
		else
		{
			xEventGroupSetBits(s_wifi_event_group, WIFI_FAIL_BIT);
		}
		ESP_LOGE(TAG_WIFI_UTILS, "Connection to AP failed!");
	}
	else if (event_base == IP_EVENT && event_id == IP_EVENT_STA_GOT_IP)
	{
		ip_event_got_ip_t *event = (ip_event_got_ip_t *)event_data;
		ESP_LOGI(TAG_WIFI_UTILS, "IP Address:" IPSTR, IP2STR(&event->ip_info.ip));
		s_retry_num = 0;
		xEventGroupSetBits(s_wifi_event_group, WIFI_CONNECTED_BIT);
	}
}

void init_nvs(void)
{
	//Initialize NVS
	esp_err_t ret = nvs_flash_init();
	if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
	{
		ESP_ERROR_CHECK(nvs_flash_erase());
		ret = nvs_flash_init();
	}
	ESP_ERROR_CHECK(ret);
}

void wifi_init_sta(void)
{
	init_nvs();
	
	s_wifi_event_group = xEventGroupCreate();

	ESP_ERROR_CHECK(esp_netif_init());

	ESP_ERROR_CHECK(esp_event_loop_create_default());
	esp_netif_create_default_wifi_sta();

	wifi_init_config_t cfg = WIFI_INIT_CONFIG_DEFAULT();
	ESP_ERROR_CHECK(esp_wifi_init(&cfg));

	ESP_ERROR_CHECK(esp_event_handler_register(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler, NULL));
    ESP_ERROR_CHECK(esp_event_handler_register(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler, NULL));

	wifi_config_t wifi_config = 
	{
		.sta = {
			.ssid = WIFI_SSID,
			.password = WIFI_PWD,
		},
	};
	ESP_ERROR_CHECK(esp_wifi_set_mode(WIFI_MODE_STA));
	ESP_ERROR_CHECK(esp_wifi_set_config(ESP_IF_WIFI_STA, &wifi_config));
	ESP_ERROR_CHECK(esp_wifi_start());

	ESP_LOGI(TAG_WIFI_UTILS, "Initialized WiFi in Station Mode");

	EventBits_t bits = xEventGroupWaitBits(s_wifi_event_group,
										   WIFI_CONNECTED_BIT | WIFI_FAIL_BIT,
										   pdFALSE,
										   pdFALSE,
										   portMAX_DELAY);

	if (bits & WIFI_CONNECTED_BIT)
	{
		ESP_LOGI(TAG_WIFI_UTILS, "Connected to AP: SSID - %s Password - %s",
				 WIFI_SSID, WIFI_PWD);
	}
	else if (bits & WIFI_FAIL_BIT)
	{
		ESP_LOGE(TAG_WIFI_UTILS, "Failed to connect: SSID- %s, Password - %s",
				 WIFI_SSID, WIFI_PWD);
		
		ESP_LOGW(TAG_WIFI_UTILS, "Restarting in 5 s");
		vTaskDelay(5000 / portTICK_PERIOD_MS);
		esp_restart();
	}
	else
	{
		ESP_LOGE(TAG_WIFI_UTILS, "Unexpected Event!");

		ESP_LOGW(TAG_WIFI_UTILS, "Restarting in 5 s");
		vTaskDelay(5000 / portTICK_PERIOD_MS);
		esp_restart();
	}

	ESP_ERROR_CHECK(esp_event_handler_unregister(WIFI_EVENT, ESP_EVENT_ANY_ID, &event_handler));
    ESP_ERROR_CHECK(esp_event_handler_unregister(IP_EVENT, IP_EVENT_STA_GOT_IP, &event_handler));
	vEventGroupDelete(s_wifi_event_group);
}
