set(srcs
	"src/wifi_utils.c"
	"src/ota_server.c")
	
idf_component_register(SRCS "${srcs}"
                       INCLUDE_DIRS "include"
                       REQUIRES app_update esp32 esp_event logger
					   esp_http_server esp_netif esp_wifi freertos 
					   log lwip nvs_flash partition_table)                   

