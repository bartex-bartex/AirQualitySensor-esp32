#ifndef HTTP_CLIENT_H
#define HTTP_CLIENT_H

#include <esp_http_client.h>

#define MAX_DATA_LENGTH 2048

esp_err_t get_handler(esp_http_client_event_t* event);
void get_request(char* url);

#endif // HTTP_CLIENT_H