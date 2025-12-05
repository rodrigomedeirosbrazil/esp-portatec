# ESP-PORTATEC

ESP-PORTATEC is a robust firmware solution designed for ESP8266 microcontrollers (WeMos D1 Mini compatible), specifically tailored for access control systems for doors and gates. The system features a persistent WebSocket connection for real-time cloud control, time-bound temporary access PINs, and a fallback mechanism for local control when internet connectivity is unavailable.

## Features

- **Dual-Mode Operation**
  - **Online Mode:** Persistent WebSocket connection to the PortaTEC platform for real-time commands and status updates.
  - **Offline/AP Mode:** Local access point for direct control and configuration when internet is unavailable.

- **Advanced Access Control**
  - **Master PIN:** Permanent PIN stored in the device's EEPROM.
  - **Temporary PINs:** Support for time-bound access codes received via WebSocket. The device validates these PINs locally based on start/end timestamps, ensuring access works even if the connection drops temporarily.
  - **Usage Logging:** Real-time notification sent to the server when a valid PIN is used.

- **Real-time Monitoring & Sync**
  - **WebSocket Sync:** Listens for commands (`pulse`, `access-pin`, `update-firmware`) and pushes status updates.
  - **Sensor Monitoring:** Detects and reports gate state (Open/Closed) using a magnetic sensor (Hall effect or Reed switch).
  - **OTA Updates:** Supports remote firmware updates triggered via WebSocket commands.

- **Easy Configuration**
  - Captive portal for WiFi and device setup.
  - Configurable GPIO pins for relay (pulse) and sensor.
  - Detailed device diagnostics page (`/info`).

## Hardware Requirements

- ESP8266 microcontroller (e.g., WeMos D1 Mini)
- Relay module (for door/gate control)
- Magnetic sensor (Reed switch) for state monitoring
- Power supply (5V via USB or external source)

## Installation

1. Clone this repository:
   ```bash
   git clone https://github.com/rodrigomedeirosbrazil/esp-portatec.git
   ```

2. Open the project in **PlatformIO** (recommended) or Arduino IDE.

3. Build and Upload:
   Using PlatformIO CLI:
   ```bash
   # Build
   pio run

   # Upload Firmware
   pio run -t upload

   # Upload Filesystem (HTML/CSS/JS)
   pio run -t uploadfs
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
5. Save and Restart.

### Web Interface Endpoints
- `/`: Main control interface (requires auth/configuration).
- `/config`: Configuration page.
- `/info`: System status, uptime, and diagnostic information.
- `/pulse?pin=YOUR_PIN`: API endpoint to trigger the relay. Accepts Master PIN or valid Temporary PINs.

## WebSocket Protocol

The device connects to a Pusher-compatible WebSocket server.

- **Incoming Events (Server -> Device):**
  - `pulse`: Triggers the relay immediately.
  - `access-pin`: Adds/Updates/Deletes temporary PINs.
    ```json
    { "action": "create", "id": 101, "code": "1234", "start": 1700000000, "end": 1700003600 }
    ```
  - `update-firmware`: Initiates an OTA update.

- **Outgoing Events (Device -> Server):**
  - `client-device-status`: Heartbeat with RSSI, uptime, etc.
  - `client-sensor-status`: Reports gate open/close state changes.
  - `client-pin-usage`: Reports when a specific temporary PIN ID is used.
  - `client-command-ack`: Acknowledges received commands.

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
