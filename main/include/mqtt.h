#ifndef MQTT_H
#define MQTT_H

void mqtt_init(char* uri, char* user_id_param, char* mac_param);
void mqtt_start();

#endif // MQTT_H