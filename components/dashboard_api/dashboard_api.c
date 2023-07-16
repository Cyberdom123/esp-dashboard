#include <sys/param.h>

#include "freertos/FreeRTOS.h"
#include "esp_system.h"
#include "esp_log.h"
#include "esp_netif.h"
#include "esp_event.h"
#include "protocol_examples_common.h"
#include "nvs.h"
#include "nvs_flash.h"
#include "json_logger.h"
#include "frozen.h"

#include <esp_http_server.h>
#include "dashboard_api.h"

static const char *TAG="HTTP_API";

/* An HTTP GET handler */
esp_err_t hello_get_handler(httpd_req_t *req)
{
    ESP_LOGI(TAG,"GET REGUEST FROM CLIENT");

    const char* resp_str = (const char*) req->user_ctx;
    httpd_resp_send(req, resp_str, strlen(resp_str));
    
    return ESP_OK;
}

esp_err_t spiffs_get_handler(httpd_req_t *req) //mutex here
{
    
    ESP_LOGI(TAG,"GET REGUEST FROM CLIENT");

    httpd_resp_set_type(req, "application/json");
    httpd_resp_set_hdr(req, "Access-Control-Allow-Origin", "*");
    char *json_str = json_fread("/spiffs/weather_log.json");
    if(json_str == NULL){
        httpd_resp_send_404(req);
        ESP_LOGE(TAG, "File cant be accesed");
    }else{
        print_logged_data("/spiffs/weather_log.json");
        httpd_resp_send(req, json_str, strlen(json_str));
        free(json_str);
    }
    return ESP_OK;
}


httpd_uri_t hello = {
    .uri       = "/hello",
    .method    = HTTP_GET,
    .handler   = spiffs_get_handler,
    /* Let's pass response string in user
     * context to demonstrate it's usage */
    .user_ctx  = "HELLO"
};

httpd_handle_t start_webserver(void)
{
    httpd_handle_t server = NULL;
    httpd_config_t config = HTTPD_DEFAULT_CONFIG();

    // Start the httpd server
    ESP_LOGI(TAG, "Starting server on port: '%d'", config.server_port);
    if (httpd_start(&server, &config) == ESP_OK) {
        // Set URI handlers
        ESP_LOGI(TAG, "Registering URI handlers");
        httpd_register_uri_handler(server, &hello);
        return server;
    }

    ESP_LOGI(TAG, "Error starting server!");
    return NULL;
}

void stop_webserver(httpd_handle_t server)
{
    // Stop the httpd server
    httpd_stop(server);
}

void disconnect_handler(void* arg, esp_event_base_t event_base, 
                               int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server) {
        ESP_LOGI(TAG, "Stopping webserver");
        stop_webserver(*server);
        *server = NULL;
    }
}

void connect_handler(void* arg, esp_event_base_t event_base, 
                            int32_t event_id, void* event_data)
{
    httpd_handle_t* server = (httpd_handle_t*) arg;
    if (*server == NULL) {
        ESP_LOGI(TAG, "Starting webserver");
        *server = start_webserver();
    }
}
