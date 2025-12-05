# ESP-Portatec Project Context

This file provides context for the Gemini CLI agent regarding the project structure, architecture, and build commands.

## 1. Project Overview
This is an ESP8266 firmware (Arduino framework) designed for gate control and monitoring ("Portatec"). 
It runs on **WeMos D1 Mini** (or compatible) boards.

### Key Features
- **WebSocket Sync:** Persistent connection to a Pusher-like server for real-time commands (`pulse`, `access-pin`, `update-firmware`) and status updates.
- **Access Manager:** Handles time-bound, temporary PINs received via WebSocket, alongside a master PIN stored in EEPROM.
- **Webserver:** Local configuration portal (Captive Portal in AP mode) and HTTP-based gate activation.
- **OTA Updates:** Firmware can be updated remotely via URL triggered by WebSocket command.
- **Sensor Monitoring:** Real-time status of the gate (open/closed) sent to the server.

## 2. Build & Flash Instructions (CRITICAL)

The `pio` command is typically **not** in the global `$PATH` for the non-interactive shell used by the agent. 

**ALWAYS use the full path to the PlatformIO executable:**

```bash
~/.platformio/penv/bin/pio run
```

### Common Commands
- **Build:** `~/.platformio/penv/bin/pio run`
- **Upload:** `~/.platformio/penv/bin/pio run -t upload`
- **Serial Monitor:** `~/.platformio/penv/bin/pio device monitor`

## 3. Architecture Modules

### `src/AccessManager/`
- **Responsibility:** Manages temporary PINs in RAM.
- **Logic:** Validates input PINs against a list of valid PINs (checking `start` and `end` timestamps) and the Master PIN.
- **Key Methods:** `validate(pin)`, `handlePinAction(create/update/delete)`, `cleanup()`.

### `src/Sync/`
- **Responsibility:** WebSocket client (Pusher protocol).
- **Logic:** 
    - Listens for: `pulse`, `access-pin`, `update-firmware`.
    - Sends: `client-device-status`, `client-sensor-status`, `client-pin-usage`, `client-command-ack`.

### `src/Webserver/`
- **Responsibility:** HTTP interface.
- **Logic:** 
    - `/config`: WiFi and Master PIN setup.
    - `/pulse?pin=...`: Triggers gate opening (validates via `AccessManager`).
    - `/info`: Device diagnostics.

### `src/DeviceConfig/`
- **Responsibility:** Persistent storage (EEPROM).
- **Data:** WiFi credentials, Master PIN, GPIO configurations, Device Name.

### `src/Clock/`
- **Responsibility:** Timekeeping.
- **Logic:** Syncs via NTP to provide `getUnixTime()` for PIN validation.

### `src/Sensor/`
- **Responsibility:** Reads the magnetic sensor (Hall effect/Reed switch).
- **Logic:** Debounces input and detects state changes.

## 4. Pinout Configuration (Default)
- **Pulse Pin:** Configurable (Default: GPIO defined in Config) - Triggers the relay.
- **Sensor Pin:** Configurable - Reads gate state.
