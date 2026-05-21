CC         ?= gcc
PKG_CONFIG ?= pkg-config
CFLAGS     ?= -std=c99 -Wall -Wextra

MOSQUITTO_CFLAGS := $(shell $(PKG_CONFIG) --cflags libmosquitto 2>/dev/null)
MOSQUITTO_LIBS   := $(shell $(PKG_CONFIG) --libs libmosquitto 2>/dev/null || printf '%s\n' -lmosquitto)

CFLAGS  += $(MOSQUITTO_CFLAGS)
LDFLAGS += $(MOSQUITTO_LIBS)

EXEEXT ?=
ifeq ($(OS),Windows_NT)
EXEEXT := .exe
endif

BROKER ?= localhost
PORT   ?= 1883
TOPIC  ?= mqtt-lab/test/sensor

PUBLISHER  = test_publisher$(EXEEXT)
SUBSCRIBER = test_subscriber$(EXEEXT)
TARGETS    = $(PUBLISHER) $(SUBSCRIBER)

.PHONY: all clean broker publisher subscriber smoke test

all: $(TARGETS)

$(PUBLISHER): test_publisher.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

$(SUBSCRIBER): test_subscriber.c
	$(CC) $(CFLAGS) -o $@ $< $(LDFLAGS)

clean:
	rm -f $(TARGETS) broker.log publisher.log subscriber.log

broker:
	mosquitto -p $(PORT) -v

publisher: $(PUBLISHER)
	MQTT_HOST=$(BROKER) MQTT_PORT=$(PORT) MQTT_TOPIC="$(TOPIC)" ./$(PUBLISHER)

subscriber: $(SUBSCRIBER)
	MQTT_HOST=$(BROKER) MQTT_PORT=$(PORT) MQTT_TOPIC="$(TOPIC)" ./$(SUBSCRIBER)

smoke: all
	@rm -f broker.log publisher.log subscriber.log
	@echo "Starting local Mosquitto broker on $(BROKER):$(PORT)..."
	@mosquitto -p $(PORT) -v > broker.log 2>&1 & broker_pid=$$!; \
	trap 'kill $$broker_pid 2>/dev/null || true; kill $$sub_pid 2>/dev/null || true' EXIT; \
	sleep 1; \
	MQTT_HOST=$(BROKER) MQTT_PORT=$(PORT) MQTT_TOPIC="$(TOPIC)" ./$(SUBSCRIBER) > subscriber.log 2>&1 & sub_pid=$$!; \
	sleep 1; \
	MQTT_HOST=$(BROKER) MQTT_PORT=$(PORT) MQTT_TOPIC="$(TOPIC)" ./$(PUBLISHER) > publisher.log 2>&1; \
	publisher_rc=$$?; \
	sleep 1; \
	kill $$sub_pid 2>/dev/null || true; \
	wait $$sub_pid 2>/dev/null || true; \
	kill $$broker_pid 2>/dev/null || true; \
	wait $$broker_pid 2>/dev/null || true; \
	echo ""; \
	echo "--- publisher.log ---"; cat publisher.log; \
	echo ""; \
	echo "--- subscriber.log ---"; cat subscriber.log; \
	if [ $$publisher_rc -ne 0 ]; then exit $$publisher_rc; fi; \
	grep -q "Message #5 received" subscriber.log

test: smoke
