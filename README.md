# ESP-PORTATEC

ESP-PORTATEC is a robust firmware solution designed for ESP8266 and ESP32 microcontrollers, specifically tailored for access control systems for doors and gates. The system features a fallback mechanism that creates a local access point when internet connectivity is unavailable, ensuring reliable operation even in offline scenarios.

## Features

- **Dual-Mode Operation**
  - Primary mode: Cloud-based control through PortaTEC platform
  - Fallback mode: Local access point for direct control when internet is unavailable

- **Easy Configuration**
  - Web-based configuration interface
  - Customizable device name
  - Configurable GPIO pins for pulse control
  - Secure password protection

- **User-Friendly Interface**
  - Responsive web interface
  - Simple one-click operation
  - Visual feedback for actions
  - Mobile-friendly design

## Hardware Requirements

- ESP8266 or ESP32 microcontroller
- Relay module or direct connection to door/gate control system
- Power supply (5V or 3.3V depending on your setup)

## Installation

1. Clone this repository:
   ```bash
   git clone https://github.com/rodrigomedeirosbrazil/esp-portatec.git
   ```

2. Open the project in Arduino IDE or PlatformIO

3. Install required dependencies:
   - ESP8266WiFi
   - ESP8266WebServer
   - DNSServer
   - EEPROM

4. Configure your device:
   - Set your device name
   - Set your WiFi password
   - Configure the pulse pin according to your hardware setup

5. Upload the firmware to your ESP device

## First-Time Setup

1. Power on your ESP device
2. Connect to the ESP-PORTATEC access point (default name: "ESP-PORTATEC")
3. Open a web browser and navigate to `http://192.168.4.1`
4. Configure your device settings:
   - Set a device name
   - Set a secure password
   - Configure the pulse pin number
5. Save the configuration

## Usage

### Normal Operation (Internet Available)
- Control your device through the PortaTEC platform
- Access your device remotely
- Monitor device status

### Fallback Mode (No Internet)
1. Connect to the ESP-PORTATEC access point
2. Open a web browser and navigate to `http://192.168.4.1`
3. Use the web interface to control your device

## Security Features

- Password-protected access point
- Secure configuration storage
- Protected configuration interface

## Contributing

Contributions are welcome! Please feel free to submit a Pull Request.

## License

This project is licensed under the MIT License - see the LICENSE file for details.

## Support

For support, please open an issue in the GitHub repository or contact the PortaTEC support team.

## Acknowledgments

- ESP8266/ESP32 community
- Arduino community
- All contributors to this project