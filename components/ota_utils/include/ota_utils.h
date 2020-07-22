#ifndef _OTA_UTILS_H
#define _OTA_UTILS_H

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "freertos/event_groups.h"

#include "esp_system.h"
#include "esp_event.h"
#include "esp_err.h"

#include "nvs_flash.h"
#include "esp_wifi.h"
#include "esp_netif.h"
#include "esp_ota_ops.h"
#include "esp_http_server.h"

#include "lwip/err.h"
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"

#include "logger.h"

//Macro for error checking
#define IS_ESP_OK(x, label) \
  if ((x) != ESP_OK) \
    goto label;

// You can either use the menuconfig utility or pass values directly here

#define WIFI_SSID CONFIG_WIFI_SSID //"myssid"
#define WIFI_PWD  CONFIG_WIFI_PWD   //"mypassword"
#define WIFI_AP_RECONN_ATTEMPTS CONFIG_WIFI_AP_RECONN_ATTEMPTS // 5

#define WIFI_CONNECTED_BIT BIT0
#define WIFI_FAIL_BIT BIT1

#define OTA_TASK_STACK_SIZE CONFIG_OTA_TASK_STACK_SIZE // 16384
#define OTA_TASK_PRIORITY CONFIG_OTA_TASK_PRIORITY // 5
#define OTA_LISTEN_PORT CONFIG_OTA_LISTEN_PORT // 8032
#define OTA_BUFF_SIZE (OTA_TASK_STACK_SIZE / 2) // To prevent buffer overflows

//WiFi Function
void wifi_init_sta(void);

// OTA Task
void ota_server_task(void *arg);

#endif


