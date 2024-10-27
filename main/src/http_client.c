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
    //char response_message[MAX_DATA_LENGTH + 1] = {0};

    esp_http_client_config_t config = {
        .url = url,
        .method = HTTP_METHOD_GET,
        // .user_data = response_message,
        .event_handler = get_handler,
    };

    ESP_LOGI("GET_REQUEST", "HTTP_CLIENT_INIT");
    esp_http_client_handle_t client = esp_http_client_init(&config);

    ESP_LOGI("GET_REQUEST", "HTTP_CLIENT_PERFORM");
    esp_err_t err = esp_http_client_perform(client);
    if (err == ESP_OK) {
        ESP_LOGI("GET_REQUEST", "HTTP GET Status = %d, content_length = %"PRId64,
                esp_http_client_get_status_code(client),
                esp_http_client_get_content_length(client));
    } else {
        ESP_LOGE("GET_REQUEST", "HTTP GET request failed: %s", esp_err_to_name(err));
    }
    
    ESP_LOGI("GET_REQUEST", "HTTP_CLIENT_CLEANUP");
    esp_http_client_cleanup(client);
}

