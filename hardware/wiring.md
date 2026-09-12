# Hardware and wiring

## I2C sensors

| Device | VCC | GND | SDA | SCL | Address |
| --- | --- | --- | --- | --- | --- |
| BME280 | 3.3 V | common GND | GPIO8 | GPIO9 | `0x76` |
| VEML7700 | 3.3 V | common GND | GPIO8 | GPIO9 | `0x10` |

Use 3.3 V logic modules. Most breakouts already include I2C pull-ups; do not add excessive parallel pull-ups.

## RainPoint

```text
ESP32-C3 3V3 ---- RainPoint dry contact ---- GPIO4
```

GPIO4 uses `INPUT_PULLDOWN`. Closed contact reads HIGH and means dry. Open contact reads LOW and means rain.
The RainPoint must not apply an external voltage to GPIO4.

## Battery input

```text
BAT+ ---- 220 kOhm ----+---- GPIO3
                       |
                     100 kOhm
                       |
                      GND

GPIO3 ---- 100 nF ---- GND
```

Connect battery negative to ESP32 GND. The firmware has a prototype ADC calibration table for this divider.

## Pin summary

| Function | GPIO |
| --- | --- |
| SDA | GPIO8 |
| SCL | GPIO9 |
| RainPoint | GPIO4 |
| Battery ADC | GPIO3 |
