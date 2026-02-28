# ESP-PORTATEC

ESP-PORTATEC is a robust firmware solution designed for ESP8266 microcontrollers (WeMos D1 Mini compatible), specifically tailored for access control systems for doors and gates. The system features MQTT connectivity for real-time cloud control, time-bound temporary access PINs, and a fallback mechanism for local control when internet connectivity is unavailable.

## Features

- **Dual-Mode Operation**
  - **Online Mode:** MQTT connection to the PortaTEC broker for real-time commands and status updates.
  - **Offline/AP Mode:** Local access point for direct control and configuration when internet is unavailable.

- **Advanced Access Control**
  - **Master PIN:** Permanent PIN stored in the device's EEPROM.
  - **Temporary PINs:** Support for time-bound access codes received via MQTT. The device validates these PINs locally based on start/end timestamps, ensuring access works even if the connection drops temporarily.
  - **Access Events:** Real-time notification (code + result: valid/invalid) sent to the broker when a PIN is used.

- **Real-time Monitoring & Sync**
  - **MQTT Sync:** Subscribes to `device/{chipId}/command` and `device/{chipId}/access-codes/sync`; publishes status, ack, and events.
  - **Sensor Monitoring:** Detects and reports gate state (Open/Closed) using a magnetic sensor (Hall effect or Reed switch).
  - **OTA Updates:** Supports remote firmware updates triggered via MQTT command (`action: "update_firmware"`).

- **Easy Configuration**
  - Captive portal for WiFi and device setup.
  - Configurable GPIO pins for relay (pulse) and sensor.
  - Detailed device diagnostics page (`/info`).

## Hardware Requirements

- ESP8266 microcontroller (e.g., WeMos D1 Mini)
- Relay module (for door/gate control)
- Magnetic sensor (Reed switch) for state monitoring
- Power supply (5V via USB or external source)

## Installation & Upload

1. Clone this repository:
   ```bash
   git clone https://github.com/rodrigomedeirosbrazil/esp-portatec.git
   ```

2. **Build & Upload Firmware:**
   Use the PlatformIO CLI to compile and flash the firmware to your device.
   ```bash
   # Build the project
   ~/.platformio/penv/bin/pio run

   # Upload Firmware to the device
   ~/.platformio/penv/bin/pio run -t upload
   ```

3. **Upload Filesystem (LittleFS):**
   The web interface files (`index.html`, `config.html`, etc.) are stored in the `data/` folder and must be uploaded to the device's flash memory separately.
   ```bash
   # Upload all files from the data/ directory to LittleFS
   ~/.platformio/penv/bin/pio run -t uploadfs
   ```

## Configuration

### First-Time Setup
1. Power on your ESP device.
2. Connect to the WiFi Access Point named **"ESP-PORTATEC"** (or similar).
3. A captive portal should open automatically. If not, navigate to `http://192.168.4.1`.
4. Configure:
   - **Device Name**: Identifier for the device.
   - **WiFi Credentials**: SSID and Password for internet connectivity.
   - **Master PIN**: The permanent access code.
   - **GPIO Settings**: Pins for the relay (Pulse) and sensor.
   - **MQTT**: Host, port (default 1883), user and password for the MQTT broker (stored in EEPROM).
5. Save and Restart.

### Web Interface Endpoints
- `/`: Main control interface (requires auth/configuration).
- `/config`: Configuration page.
- `/info`: System status, uptime, and diagnostic information.
- `/pulse?pin=YOUR_PIN`: API endpoint to trigger the relay. Accepts Master PIN or valid Temporary PINs.

## MQTT Protocol

The device connects to an MQTT broker (configurable via DeviceConfig).

- **Subscribed Topics (Broker -> Device):**
  - `device/{chipId}/command`: Commands with `action` (`pulse`, `toggle`, `update_firmware`).
  - `device/{chipId}/access-codes/sync`: Full sync of access codes.
    ```json
    { "action": "sync_access_codes", "default_pin": "...", "access_codes": [
      { "pin": "1234", "start_unix": 1700000000, "end_unix": 1700003600 }
    ]}
    ```

- **Published Topics (Device -> Broker):**
  - `device/{chipId}/status`: Heartbeat and sensor status (RSSI, uptime, sensor_value).
  - `device/{chipId}/ack`: Command acknowledgments.
  - `device/{chipId}/event`: Access events with `pin` (code used), `result` (valid/invalid), `timestamp_device`.
  - `device/{chipId}/access-codes/ack`: Confirmation of access codes sync.

## Filesystem Management

The web interface files (`index.html`, `config.html`, etc.) are stored in the LittleFS filesystem.

1.  **Erase Flash (Factory Reset):**
    ```bash
    pio run -t erase -e esp01
    ```

2.  **Upload Filesystem:**
    ```bash
    pio run -t uploadfs -e esp01
    ```

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the LICENSE file for details.
