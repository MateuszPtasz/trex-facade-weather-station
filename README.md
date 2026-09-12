# T-REX Facade Weather Station

**English | [Polski](README_PL.md)**

## What is T-REX Facade Weather Station

Open DIY facade weather station based on ESP32-C3 Super Mini.

It measures temperature, humidity, pressure, illuminance, rain, battery voltage and Wi-Fi RSSI.
It publishes data through UDP and may be used without T-REX Smart Home.

The station was created for T-REX Smart Home roller-shutter automation, but is an independent project.

## Features

- Firmware v1.2.2 for ESP32-C3 Super Mini.
- BME280 (`0x76`) for temperature, humidity and pressure.
- VEML7700 (`0x10`) for illuminance.
- RainPoint dry-contact input with deep-sleep wake on rain-state changes.
- Battery measurement with an editable calibration table.
- Wi-Fi configuration portal and UDP telemetry/discovery.

## Prototype enclosure and RainPoint integration

The current prototype reuses the RainPoint enclosure as the physical enclosure for the complete facade station.

The original RainPoint mechanical rain sensor remains in use as a dry-contact rain detector. The ESP32-C3, BME280, VEML7700, battery-measurement circuit and power source are installed inside the same enclosure.

This avoids designing a weather-resistant enclosure from scratch and keeps the original rain-sensing mechanism as part of the finished station.

The exact mechanical placement may depend on the RainPoint enclosure version and the builder's components. Keep the environmental sensors exposed to the conditions they are intended to measure, while protecting the electronics from direct water ingress.

## Hardware

| Part | Purpose |
| --- | --- |
| ESP32-C3 Super Mini | controller and Wi-Fi |
| BME280 | temperature, humidity and pressure |
| VEML7700 | illuminance |
| RainPoint | enclosure and dry-contact rain sensor in the prototype |
| 1S battery with divider | power and battery telemetry |

See [hardware/wiring.md](hardware/wiring.md) for the complete wiring table and connection diagram.

## Wiring

| Signal | ESP32-C3 pin |
| --- | --- |
| I2C SDA | GPIO8 |
| I2C SCL | GPIO9 |
| RainPoint input | GPIO4 |
| Battery ADC | GPIO3 |

Use 3.3 V power and a common ground for both I2C modules.
RainPoint wiring is `3V3 -> dry contact -> GPIO4` and GPIO4 uses `INPUT_PULLDOWN`.
Closed/HIGH means dry; open/LOW means rain.

## Firmware

The PlatformIO project is in [`firmware/`](firmware/).

```sh
cd firmware
pio run
pio run -t upload
pio device monitor -b 115200
```

The target is `esp32-c3-devkitm-1`, compatible with ESP32-C3 Super Mini.
Choose the local serial upload port yourself; it is intentionally not stored in this repository.

## Wi-Fi setup

After physical power-on or reset the station enters configuration mode and creates `T-REX-FS-XXXXXXXX`.

1. Join that AP.
2. Open `http://192.168.4.1`.
3. Scan for a network, choose the 2.4 GHz SSID and save the password.
4. Credentials are stored locally in Preferences/NVS.

Normal work: wake, measure, join Wi-Fi, broadcast telemetry, listen for UDP requests for about eight seconds, sleep.

## UDP protocol

UDP port: **4210**. Protocol marker: **`trex-wall/1`**.

| Request | Response |
| --- | --- |
| `TREX_DISCOVER_V1` | JSON with `type: "announce"` |
| `TREX_GET:*` | current JSON telemetry |
| `TREX_GET:TRX-FS-XXXXXXXX` | current JSON telemetry from that serial |

See [docs/protocol.md](docs/protocol.md) for the complete schema and a Python receiver without T-REX HOME dependencies.

## Telemetry format

```json
{
  "protocol": "trex-wall/1",
  "type": "telemetry",
  "serial": "TRX-FS-1D3F1FB8",
  "fw_version": "1.2.2",
  "hw_version": "1.0",
  "ip": "192.168.1.123",
  "temperature_c": 21.45,
  "humidity_pct": 58.2,
  "pressure_hpa": 1008.34,
  "lux": 12540.0,
  "rain": false,
  "battery_raw_v": 4.179,
  "battery_v": 4.2,
  "battery_pct": 100,
  "battery_status": "OK",
  "rssi": -61
}
```

Unavailable BME280 or VEML7700 values are JSON `null`.

## Deep sleep and wake behavior

The periodic wake interval is ten minutes.
While dry (`HIGH`), GPIO4 wake is armed for `LOW` (rain).
While raining (`LOW`), GPIO4 wake is armed for `HIGH` (dry).

## Battery measurement

```text
BAT+ -- 220 kOhm --+-- GPIO3
                   |
                 100 kOhm
                   |
                  GND

GPIO3 -- 100 nF -- GND
```

Firmware includes a prototype-specific ADC calibration table; verify it with a multimeter and adjust if required.

## Known limitations

- Wi-Fi setup AP is open; configure it in a trusted environment.
- The station receives discovery only while awake.
- Configuration serial text says 20 minutes, while `CONFIG_TIME_MS` is ten minutes.
- No Home Assistant integration is included; consume UDP directly or bridge it.
- RainPoint enclosure integration is prototype-specific; mechanical mounting details may differ between builds.

## License

MIT. See [LICENSE](LICENSE).
