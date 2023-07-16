#ifndef DASHBOARD_API
#define DASHBOARD_API
#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "protocol_examples_common.h"
#include "nvs.h"
#include "nvs_flash.h"

#include <esp_http_server.h>

void connect_handler(void* arg, esp_event_base_t event_base, 
                            int32_t event_id, void* event_data);

void disconnect_handler(void* arg, esp_event_base_t event_base, 
                               int32_t event_id, void* event_data);

httpd_handle_t start_webserver(void);

void stop_webserver(httpd_handle_t server);

void tem_set_test();

#endif