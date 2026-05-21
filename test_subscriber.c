/*
 * test_subscriber.c
 * Connects to an MQTT broker and prints every message on the lab topic.
 * Usage: ./test_subscriber   (keep running while publisher sends)
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>

#define DEFAULT_BROKER "localhost"
#define DEFAULT_PORT   1883
#define DEFAULT_TOPIC  "mqtt-lab/test/sensor"

struct mqtt_config {
    const char *host;
    int port;
    const char *topic;
};

static int msg_count = 0;

static const char *config_string(const char *name, const char *fallback) {
    const char *value = getenv(name);

    if (value == NULL || value[0] == '\0') {
        return fallback;
    }

    return value;
}

static int config_port(void) {
    const char *value = getenv("MQTT_PORT");

    if (value == NULL || value[0] == '\0') {
        return DEFAULT_PORT;
    }

    char *end = NULL;
    errno = 0;
    long port = strtol(value, &end, 10);

    if (errno != 0 || end == value || *end != '\0' || port < 1 || port > 65535) {
        fprintf(stderr, "[subscriber] Invalid MQTT_PORT '%s'; using %d\n",
                value, DEFAULT_PORT);
        return DEFAULT_PORT;
    }

    return (int)port;
}

/* Callback: called when connection is established */
static void on_connect(struct mosquitto *mosq, void *userdata, int rc) {
    const struct mqtt_config *config = userdata;

    if (rc == 0) {
        printf("[subscriber] Connected to %s:%d\n", config->host, config->port);
        printf("[subscriber] Subscribing to: %s\n\n", config->topic);

        int sub_rc = mosquitto_subscribe(mosq, NULL, config->topic, 1);
        if (sub_rc != MOSQ_ERR_SUCCESS) {
            fprintf(stderr, "[subscriber] Subscribe failed: %s\n",
                    mosquitto_strerror(sub_rc));
        }
    } else {
        fprintf(stderr, "[subscriber] Connection failed: %s\n",
                mosquitto_connack_string(rc));
    }
}

/* Callback: called when a message arrives */
static void on_message(struct mosquitto *mosq, void *userdata,
                       const struct mosquitto_message *msg) {
    (void)mosq;
    (void)userdata;

    msg_count++;
    printf("[subscriber] Message #%d received:\n", msg_count);
    printf("  Topic:   %s\n", msg->topic);
    printf("  Payload: %.*s\n\n", msg->payloadlen, (char *)msg->payload);
}

/* Callback: called on disconnect */
static void on_disconnect(struct mosquitto *mosq, void *userdata, int rc) {
    (void)mosq;
    (void)userdata;

    if (rc != 0) {
        printf("[subscriber] Unexpected disconnect (rc=%d) – will reconnect\n", rc);
    } else {
        printf("[subscriber] Disconnected cleanly. Received %d messages total.\n",
               msg_count);
    }
}

int main(void) {
    struct mqtt_config config = {
        .host = config_string("MQTT_HOST", DEFAULT_BROKER),
        .port = config_port(),
        .topic = config_string("MQTT_TOPIC", DEFAULT_TOPIC),
    };

    mosquitto_lib_init();

    struct mosquitto *mosq = mosquitto_new(NULL, true, &config);
    if (!mosq) {
        fprintf(stderr, "[subscriber] Failed to create mosquitto instance\n");
        mosquitto_lib_cleanup();
        return 1;
    }

    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_message_callback_set(mosq, on_message);
    mosquitto_disconnect_callback_set(mosq, on_disconnect);

    int rc = mosquitto_connect(mosq, config.host, config.port, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "[subscriber] Could not connect: %s\n",
                mosquitto_strerror(rc));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }

    printf("[subscriber] Waiting for messages. Press Ctrl+C to stop.\n\n");

    /* Loop forever – handles reconnects automatically */
    mosquitto_loop_forever(mosq, -1, 1);

    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}
