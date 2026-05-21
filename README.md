# mqtt-lab

Small MQTT C lab using Mosquitto. It builds two programs:

- `test_subscriber` subscribes to `mqtt-lab/test/sensor` and prints received messages.
- `test_publisher` publishes five simulated sensor readings as JSON.

The programs default to a local Mosquitto broker at `localhost:1883`. You can override the connection with environment variables:

- `MQTT_HOST`
- `MQTT_PORT`
- `MQTT_TOPIC`

## Dependencies

On Windows, this repo was set up with MSYS2 and the UCRT64 packages:

```sh
pacman -S --needed make mingw-w64-ucrt-x86_64-gcc mingw-w64-ucrt-x86_64-make mingw-w64-ucrt-x86_64-pkgconf mingw-w64-ucrt-x86_64-mosquitto
```

Open an **MSYS2 UCRT64** shell, or make sure these directories are on `PATH` before building from PowerShell:

```powershell
$env:Path = "C:\msys64\ucrt64\bin;C:\msys64\usr\bin;$env:Path"
```

## Build

```sh
make
```

The Makefile uses `pkg-config` for the Mosquitto include and library paths, with `-lmosquitto` as a fallback.

## Run The Local Broker Server

Start the Mosquitto broker in one terminal and leave it running:

```sh
make broker
```

Equivalent command:

```sh
mosquitto -p 1883 -v
```

## Run The Subscriber And Publisher

Open a second terminal for the subscriber:

```sh
make subscriber
```

Open a third terminal for the publisher:

```sh
make publisher
```

You should see five JSON payloads printed in the subscriber terminal.

## One-Command Smoke Test

To build everything, start a temporary local broker, run the subscriber, publish five messages, and verify that the subscriber received all five:

```sh
make test
```

This target writes temporary `broker.log`, `publisher.log`, and `subscriber.log` files and removes them with `make clean`.

## Use A Different Broker

For a public broker or a broker on another host:

```sh
MQTT_HOST=test.mosquitto.org MQTT_PORT=1883 make publisher
MQTT_HOST=test.mosquitto.org MQTT_PORT=1883 make subscriber
```

On PowerShell, set the variables before running the program:

```powershell
$env:MQTT_HOST = "test.mosquitto.org"
$env:MQTT_PORT = "1883"
make publisher
```
