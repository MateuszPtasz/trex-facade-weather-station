# Contributing

T-REX Facade Weather Station is an open DIY project. Contributions are welcome, especially fixes that improve reproducibility, protocol documentation, power consumption, sensor handling and enclosure integration.

## Before opening a pull request

1. Keep compatibility with ESP32-C3 Super Mini unless the change explicitly introduces another supported target.
2. Do not silently change the UDP protocol. Document every field or behavior change in `docs/protocol.md` and `docs/protocol_PL.md`.
3. Keep the RainPoint logic consistent with the documented hardware:
   - closed/HIGH = dry,
   - open/LOW = rain,
   - GPIO4 uses `INPUT_PULLDOWN`.
4. Preserve local-first operation. Core sensing and telemetry must not depend on cloud services.
5. Test the change against `docs/testing.md` where applicable.
6. Update `CHANGELOG.md` for user-visible changes.

## Documentation

English is the default README language. Important end-user documentation should also have a Polish equivalent where practical.

## Hardware changes

For wiring changes, update both files:

- `hardware/wiring.md`
- `hardware/wiring_PL.md`

For mechanical changes, add photos or diagrams when they materially improve reproducibility.

## Security

Do not commit Wi-Fi passwords, private IP credentials, API keys, personal data or machine-specific serial-port configuration.

## Scope

This repository contains the open facade weather station. T-REX HOME is a separate project and its controller firmware is not part of this repository.
