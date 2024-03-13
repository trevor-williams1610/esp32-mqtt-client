#include "mqtt_client.h"
#include "mongoose.h"

static const char *s_mqtt_broker = "mqtt://192.168.4.1:1883";
static const char *s_mqtt_topic = "client/lightshow";

static void mqtt_event_handler(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_MQTT_OPEN) {
        printf("MQTT connected\n");
        struct mg_str topic = {s_mqtt_topic, strlen(s_mqtt_topic)};
        struct mg_mqtt_opts opts = {.qos = 1};
        mg_mqtt_sub(c, &opts);
    } else if (ev == MG_EV_MQTT_MSG) {
        struct mg_mqtt_message *msg = (struct mg_mqtt_message *)ev_data;
        printf("Received MQTT message: %.*s\n", (int)msg->data.len, msg->data.ptr);
    } else if (ev == MG_EV_CLOSE) {
        printf("MQTT connection closed\n");
    }
}

void mqtt_client_init(void) {
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);

    struct mg_connection *c = mg_mqtt_connect(&mgr, s_mqtt_broker, NULL, mqtt_event_handler, NULL);
    if (c == NULL) {
        printf("Failed to connect to MQTT broker\n");
        mg_mgr_free(&mgr);
        return;
    }

    while (true) {
        mg_mgr_poll(&mgr, 1000);  // Poll the event manager
    }

    mg_mgr_free(&mgr);
}
