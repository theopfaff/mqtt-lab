/*
 * test_publisher.c
 * Connects to an MQTT broker and publishes 5 simulated sensor readings.
 * Usage: ./test_publisher
 */

#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <mosquitto.h>

#define DEFAULT_BROKER "localhost"
#define DEFAULT_PORT   1883
#define DEFAULT_TOPIC  "mqtt-lab/test/sensor"
#define MSG_COUNT      5
#define INTERVAL_S     2

struct mqtt_config {
    const char *host;
    int port;
    const char *topic;
};

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
        fprintf(stderr, "[publisher] Invalid MQTT_PORT '%s'; using %d\n",
                value, DEFAULT_PORT);
        return DEFAULT_PORT;
    }

    return (int)port;
}

/* Generate a simple JSON payload with dummy sensor data */
static void build_payload(char *buf, size_t len, int seq) {
    time_t now = time(NULL);
    struct tm *t = gmtime(&now);
    char ts[30];
    strftime(ts, sizeof(ts), "%Y-%m-%dT%H:%M:%SZ", t);

    /* Simple simulated values */
    float temp     = 18.0f + (float)(rand() % 100) / 20.0f;   /* 18.0 – 23.0 */
    float humidity = 50.0f + (float)(rand() % 300) / 10.0f;   /* 50.0 – 80.0 */

    snprintf(buf, len,
        "{\"seq\":%d,\"station_id\":\"S1\","
        "\"timestamp\":\"%s\","
        "\"temperature_c\":%.1f,"
        "\"humidity_pct\":%.1f}",
        seq, ts, temp, humidity);
}

/* Callback: called when connection is established */
static void on_connect(struct mosquitto *mosq, void *userdata, int rc) {
    const struct mqtt_config *config = userdata;

    if (rc == 0) {
        printf("[publisher] Connected to %s:%d\n", config->host, config->port);
    } else {
        fprintf(stderr, "[publisher] Connection failed: %s\n",
                mosquitto_connack_string(rc));
        mosquitto_disconnect(mosq);
    }
}

/* Callback: called after each message is published */
static void on_publish(struct mosquitto *mosq, void *userdata, int mid) {
    (void)mosq;
    (void)userdata;

    printf("[publisher] Message %d delivered to broker\n", mid);
}

int main(void) {
    srand((unsigned int)time(NULL));

    struct mqtt_config config = {
        .host = config_string("MQTT_HOST", DEFAULT_BROKER),
        .port = config_port(),
        .topic = config_string("MQTT_TOPIC", DEFAULT_TOPIC),
    };

    mosquitto_lib_init();

    struct mosquitto *mosq = mosquitto_new(NULL, true, &config);
    if (!mosq) {
        fprintf(stderr, "[publisher] Failed to create mosquitto instance\n");
        mosquitto_lib_cleanup();
        return 1;
    }

    mosquitto_connect_callback_set(mosq, on_connect);
    mosquitto_publish_callback_set(mosq, on_publish);

    int rc = mosquitto_connect(mosq, config.host, config.port, 60);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "[publisher] Could not connect: %s\n",
                mosquitto_strerror(rc));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }

    /* Start the network loop in background thread */
    rc = mosquitto_loop_start(mosq);
    if (rc != MOSQ_ERR_SUCCESS) {
        fprintf(stderr, "[publisher] Could not start network loop: %s\n",
                mosquitto_strerror(rc));
        mosquitto_destroy(mosq);
        mosquitto_lib_cleanup();
        return 1;
    }

    char payload[256];
    printf("[publisher] Sending %d messages to topic: %s\n\n", MSG_COUNT, config.topic);

    for (int i = 1; i <= MSG_COUNT; i++) {
        build_payload(payload, sizeof(payload), i);
        printf("[publisher] Publishing: %s\n", payload);

        rc = mosquitto_publish(mosq, NULL, config.topic,
                               (int)strlen(payload), payload, 1, false);
        if (rc != MOSQ_ERR_SUCCESS) {
            fprintf(stderr, "[publisher] Publish error: %s\n",
                    mosquitto_strerror(rc));
        }
        sleep(INTERVAL_S);
    }

    printf("\n[publisher] Done. Disconnecting.\n");
    mosquitto_disconnect(mosq);
    mosquitto_loop_stop(mosq, true);
    mosquitto_destroy(mosq);
    mosquitto_lib_cleanup();
    return 0;
}
