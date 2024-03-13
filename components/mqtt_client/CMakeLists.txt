idf_component_register(SRCS "mqtt_client.c"
                       INCLUDE_DIRS "include"
                       REQUIRES wifi nvs_flash mongoose esp_timer)
