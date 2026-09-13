# Build guide

**English | [Polski](build_PL.md)**

This guide covers the current T-REX Facade Weather Station prototype built inside a RainPoint enclosure.

## Parts

| Part | Notes |
| --- | --- |
| ESP32-C3 Super Mini | main controller and Wi-Fi |
| BME280, 3.3 V | I2C address `0x76` in the tested build |
| VEML7700 | I2C address `0x10` |
| RainPoint enclosure and dry contact | reused as enclosure and rain detector |
| 1S battery / supply | according to your mechanical implementation |
| 220 kOhm resistor | battery-divider high side |
| 100 kOhm resistor | battery-divider low side |
| 100 nF capacitor | ADC filtering from GPIO3 to GND |
| wire, heat-shrink, insulation | for the prototype assembly |

## Electrical connections

Use [../hardware/wiring.md](../hardware/wiring.md) as the authoritative wiring reference.

Key pins:

| Function | ESP32-C3 |
| --- | --- |
| SDA | GPIO8 |
| SCL | GPIO9 |
| RainPoint | GPIO4 |
| Battery ADC | GPIO3 |

RainPoint is wired as `3V3 -> dry contact -> GPIO4` with `INPUT_PULLDOWN`.

- contact closed / HIGH = dry
- contact open / LOW = rain

## Mechanical assembly

The prototype intentionally reuses the RainPoint housing instead of introducing a new custom weather enclosure.

A practical assembly order is:

1. Keep the original RainPoint mechanical rain-contact assembly in place.
2. Place the ESP32-C3 where the USB connector remains reachable during development.
3. Mount the BME280 where it can sample ambient air but is protected from direct water ingress.
4. Position the VEML7700 so its optical path is not blocked by the enclosure or wiring.
5. Route the RainPoint contact wires to GPIO4 and 3.3 V.
6. Install the battery-divider components close to the ESP32 ADC input.
7. Secure all wiring so closing the housing cannot pinch a conductor or press on the sensors.
8. Verify the enclosure can close without loading the PCB or connectors mechanically.

See [gallery.md](gallery.md) for photographs of the current prototype.

## Firmware upload

The PlatformIO project is in `firmware/`.

```sh
cd firmware
pio run
pio run -t upload
pio device monitor -b 115200
```

Target: `esp32-c3-devkitm-1`.

For Arduino IDE, select an ESP32-C3 target compatible with the ESP32-C3 Super Mini. Do not compile this project for ESP32-S2 or a generic non-C3 target.

## First start

After a physical power-on or reset the station starts its configuration mode and exposes an access point named like:

`T-REX-FS-XXXXXXXX`

Connect to it and open:

`http://192.168.4.1`

Use the page to scan for a 2.4 GHz Wi-Fi network, enter the password and save it.

## Before closing the enclosure

Verify all of the following:

- BME280 values are visible.
- VEML7700 lux changes when the sensor is covered/exposed.
- RainPoint shows dry when the contact is closed.
- Opening the RainPoint contact changes the state to rain.
- Battery voltage is plausible compared with a multimeter.
- Wi-Fi connects and an IP address/RSSI are visible.
- UDP telemetry is received on port 4210.

Then close the enclosure and repeat the rain and Wi-Fi tests once more.

## Outdoor mounting notes

This repository documents the working prototype, not a certified outdoor product. Mount the station so the RainPoint mechanism can operate normally, ambient sensors can measure the intended environment, and direct water ingress into the electronics is minimized.
