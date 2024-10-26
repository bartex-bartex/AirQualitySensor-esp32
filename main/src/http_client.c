#include <stdio.h>
#include "esp_http_client.h"
#include "http_client.h"
#include "esp_log.h"

esp_err_t get_handler(esp_http_client_event_t* event){
    switch(event->event_id)
    {
        case HTTP_EVENT_ON_DATA:
            printf("%.*s", event->data_len, (char*)event->data);
            break;
        default:
            break;
    }

    return ESP_OK;
}

void get_request(char* url) {
    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        .event_handler = get_handler
    };

    ESP_LOGI("GET_REQUEST", "HTTP_CLIENT_INIT");
    esp_http_client_handle_t client = esp_http_client_init(&config);

    ESP_LOGI("GET_REQUEST", "HTTP_CLIENT_PERFORM");
    esp_http_client_perform(client);

    ESP_LOGI("GET_REQUEST", "HTTP_CLIENT_CLEANUP");
    esp_http_client_cleanup(client);
}

