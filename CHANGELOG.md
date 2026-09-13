# Changelog

All notable public changes to T-REX Facade Weather Station are documented here.

## Unreleased

- Added prototype gallery documentation.
- Added build guides in English and Polish.
- Added validation checklists in English and Polish.
- Added contribution guidelines.

## 1.2.2

- ESP32-C3 Super Mini firmware baseline.
- BME280 temperature/humidity/pressure sensing.
- VEML7700 illuminance sensing.
- RainPoint dry-contact support using `INPUT_PULLDOWN`.
- Correct rain semantics: closed/HIGH = dry, open/LOW = rain.
- Deep-sleep wake on RainPoint state changes.
- Battery ADC measurement and calibration table.
- Wi-Fi configuration portal.
- UDP telemetry and discovery using protocol marker `trex-wall/1` on port 4210.
- Persistent serial derived from ESP32 identity.

## Hardware 1.0 prototype

- RainPoint enclosure reused as the physical station enclosure.
- RainPoint mechanical dry contact retained as the rain detector.
- Battery divider: 220 kOhm / 100 kOhm with 100 nF ADC filtering capacitor.
