# ESP32-HomeKit-Thermostat

An ESP32-based thermostat and hot water controller with native Apple HomeKit support, built using the [HomeSpan](https://github.com/HomeSpan/HomeSpan) framework.  
The device controls a heating relay based on ambient temperature with hysteresis and exposes a separate switch for hot water control.

Temperature is measured using a DS18B20 sensor, while heating and hot water are switched via a 2-channel ESP32 relay board powered from mains voltage.  
All settings are persisted across reboots using NVS preferences.

The accessory can be paired directly with Apple Home or Home Assistant (via HomeKit Controller).

## Hardware

- [ESP32 2-Channel Relay Board](https://www.aliexpress.com/item/1005007027676026.html)
- [DS18B20 Waterproof Temperature Sensor](https://www.aliexpress.com/item/1005010154075608.html)
- [FT232RL USB-to-Serial Adapter](https://www.aliexpress.com/item/1005007395673301.html)
  

## Pin Configuration

| Function        | GPIO |
|-----------------|------|
| DS18B20 Data    | 4    |
| Heating Relay   | 16   |
| Hot Water Relay | 17   |

Relay logic levels:
- `HIGH` → Relay ON  
- `LOW`  → Relay OFF

## HomeKit Services

### Thermostat
- Current temperature
- Target temperature (10–30 °C, 0.5 °C steps)
- Heating on/off mode
- Temperature hysteresis: ±0.2 °C

### Hot Water
- Simple on/off switch exposed as a HomeKit accessory

## Usage

### Flashing

Configure the ESP32 Dev Module in the Arduino IDE with the following settings:

```
CPU Frequency: "240MHz (WiFi/BT)"
Events Run On: "Core 1"
Flash Frequency: "80MHz"
Flash Mode: "QIO"
Flash Size: "4MB (32Mb)"
Partition Scheme: "Huge APP (3MB No OTA/1MB SPIFFS)"
Upload Speed: "921600"
```

Upload the sketch using an FT232RL (or compatible) USB-to-serial adapter.

### Wi-Fi Setup

On first boot, Wi-Fi credentials must be configured via the serial console using the HomeSpan provisioning interface.

Open a serial monitor at **115200 baud** and follow the on-screen instructions.

### Pairing

The device advertises itself as a HomeKit Thermostat accessory.

- **Pairing code:** `111-22-333`
- Can be added using:
  - Apple Home app
  - Home Assistant (HomeKit Controller integration)

## Notes

- The device stores the target temperature, thermostat mode, and hot water state in NVS and restores them after reboot.
- Heating control uses a simple hysteresis-based on/off strategy to prevent relay chatter.
- This project is intended for **experienced users** working with **mains voltage** — proper isolation, enclosures, and electrical safety practices are mandatory.
