# Watering System — ESP32 Plant Watering Controller

Automatic plant watering system based on soil moisture sensing. The ESP32 reads an analog soil moisture sensor and activates a submersible pump via a MOSFET when the soil is too dry.

## Hardware BOM

| Component | Notes |
|---|---|
| ESP32 Dev Module | Any 38-pin variant works |
| Capacitive soil moisture sensor | Preferred over resistive (doesn't corrode) |
| N-channel MOSFET | e.g. IRLZ44N or IRF520 — logic-level gate drive |
| Flyback diode | e.g. 1N4007 — across pump terminals (protects MOSFET) |
| 5V submersible mini pump | Most USB-powered 3–5V pumps work |
| 10kΩ resistor | Gate pull-down resistor on MOSFET |
| Power supply | 5V for pump + 3.3V ESP32 (USB or separate rail) |

## Wiring

```
ESP32 GPIO34  ──────────────────────── Sensor AOUT
ESP32 3.3V    ──────────────────────── Sensor VCC
ESP32 GND     ──────────────────────── Sensor GND

ESP32 GPIO26  ──── 10kΩ to GND ─────── MOSFET Gate
MOSFET Source ──────────────────────── GND (common)
MOSFET Drain  ──────────────────────── Pump negative terminal
5V rail       ──────────────────────── Pump positive terminal
1N4007 diode  ──── cathode to 5V, anode to MOSFET Drain (flyback)
```

## Sketch Location

The Arduino sketch lives in `watering_system/watering_system.ino`.
Arduino IDE requires the folder and `.ino` file to share the same name.

## Calibrating the Moisture Threshold

The ESP32 ADC returns values from 0 to 4095. Capacitive sensors read **higher in dry soil** and **lower in wet soil**.

1. Flash the sketch, open Serial Monitor at **115200 baud**
2. Insert the sensor into very dry soil, note the ADC reading
3. Insert into freshly watered soil, note the reading
4. Set `MOISTURE_DRY_THRESHOLD` to a value between the two (closer to the dry reading)

Example readings (your sensor may differ):
- Dry soil: ~2800
- Wet soil: ~1100
- Good threshold: ~2500

## Key Configuration Constants

All in the `BEHAVIOR CONFIG` section at the top of `watering_system.ino`:

| Constant | Default | Description |
|---|---|---|
| `MOISTURE_DRY_THRESHOLD` | 2500 | Water when ADC reading is above this |
| `PUMP_RUN_MS` | 5000 | Pump on duration per cycle (ms) |
| `CHECK_INTERVAL_MS` | 30000 | How often to read sensor (ms) |
| `COOLDOWN_MS` | 120000 | Wait after watering before next check (ms) |

## Flashing

1. Install the ESP32 board package in Arduino IDE (Boards Manager → search "esp32" by Espressif)
2. Open `watering_system/watering_system.ino`
3. Tools → Board → ESP32 Arduino → **ESP32 Dev Module**
4. Tools → Port → select your USB port
5. Click Upload

## Future: Home Assistant Integration

The control loop is non-blocking (`millis()`-based), which makes WiFi/MQTT easy to add later. Planned approach:
- Library: `PubSubClient` for MQTT, `WiFi.h` (built-in)
- Integration: MQTT Discovery — HA auto-detects the device
- Topics to publish: `home/plants/moisture` (ADC value), `home/plants/pump` (ON/OFF state)
- Topic to subscribe: `home/plants/pump/set` (manual trigger from HA)

The loop already has a placeholder comment showing exactly where to insert the MQTT calls.
