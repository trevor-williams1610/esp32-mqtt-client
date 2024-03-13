idf_component_register(SRCS "main.c"
                       INCLUDE_DIRS "."
                       REQUIRES wifi nvs_flash mqtt_client)
