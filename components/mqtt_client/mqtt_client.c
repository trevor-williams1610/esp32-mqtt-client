#include "mongoose.h"
#include "esp_log.h"

static const char *TAG = "MQTT_CLIENT";
static const char *s_mqtt_broker = "mqtt://192.168.4.1:1883";
static const char *specific_topic = "lightshow";
static bool mqtt_connected = false;


static void mqtt_event_handler(struct mg_connection *c, int ev, void *ev_data) {
    if (ev == MG_EV_MQTT_OPEN) {
        ESP_LOGI(TAG, "MQTT connected");
        mqtt_connected = true;

        uint16_t topic_len = htons(strlen(specific_topic));
        size_t packet_len = 7 + strlen(specific_topic); // Fixed header (2 bytes) + Message ID (2 bytes) + Topic length (2 bytes) + Topic + QoS (1 byte)
        uint8_t *packet = malloc(packet_len);
        if (packet == NULL) {
            ESP_LOGE(TAG, "Failed to allocate memory for MQTT packet");
            return;
        }

        packet[0] = 0x82; // SUBSCRIBE fixed header (type = 8, QoS = 1)
        packet[1] = packet_len - 2; // Remaining length
        packet[2] = 0x00; // Message ID MSB
        packet[3] = 0x01; // Message ID LSB
        packet[4] = (uint8_t)((topic_len >> 8) & 0xFF); // Topic filter length MSB
        packet[5] = (uint8_t)(topic_len & 0xFF); // Topic filter length LSB
        memcpy(packet + 6, specific_topic, strlen(specific_topic)); // Topic filter
        packet[packet_len - 1] = 0x01; // Requested QoS

        // Send the packet
        mg_send(c, packet, packet_len);

        // Free the allocated memory
        free(packet);
    } else if (ev == MG_EV_MQTT_MSG) {
        struct mg_mqtt_message *msg = (struct mg_mqtt_message *)ev_data;
        ESP_LOGI(TAG, "Received MQTT message on topic %.*s: %.*s", (int)msg->topic.len, msg->topic.ptr, (int)msg->data.len, msg->data.ptr);
    } else if (ev == MG_EV_CLOSE) {
        ESP_LOGI(TAG, "MQTT connection closed");
        mqtt_connected = false;
    }
}

void mqtt_client_init(void) {
    struct mg_mgr mgr;
    mg_mgr_init(&mgr);

    struct mg_connection *c = NULL;
    int retry_count = 0;
    const int max_retries = 10;
    int wait_time_ms = 1000;  // Initial wait time of 1 second
    const int max_wait_time_ms = 30000;  // Maximum wait time of 30 seconds

    while (retry_count < max_retries && !mqtt_connected) {
        ESP_LOGI(TAG, "Connecting to MQTT broker at %s (attempt %d)", s_mqtt_broker, retry_count + 1);
        c = mg_mqtt_connect(&mgr, s_mqtt_broker, NULL, mqtt_event_handler, NULL);

        if (c != NULL) {
            // Wait for the connection to be established or for the timeout
            int wait_time_remaining = wait_time_ms;
            while (wait_time_remaining > 0 && !mqtt_connected) {
                mg_mgr_poll(&mgr, 100);  // Poll the event manager
                wait_time_remaining -= 100;
            }

            if (mqtt_connected) {
                break;  // Successfully connected
            }
        }

        ESP_LOGW(TAG, "Failed to connect to MQTT broker, retrying in %d ms...", wait_time_ms);
        vTaskDelay(pdMS_TO_TICKS(wait_time_ms));
        wait_time_ms = (wait_time_ms * 2 < max_wait_time_ms) ? wait_time_ms * 2 : max_wait_time_ms;  // Exponential backoff with max limit
        retry_count++;
    }

    if (retry_count == max_retries && !mqtt_connected) {
        ESP_LOGE(TAG, "Failed to connect to MQTT broker after %d attempts", max_retries);
        mg_mgr_free(&mgr);
        return;
    }

    ESP_LOGI(TAG, "Connected to MQTT broker");

    while (true) {
        mg_mgr_poll(&mgr, 1000);  // Poll the event manager continuously
    }

    mg_mgr_free(&mgr);
}
