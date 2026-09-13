# Validation checklist

**English | [Polski](testing_PL.md)**

Use this checklist after assembly, after firmware changes and before publishing a release.

## 1. Boot and identity

- Serial begins with `TRX-FS-`.
- Firmware and hardware versions are visible in the web interface.
- Configuration AP name contains the same serial.

## 2. Sensors

- Temperature, humidity and pressure from BME280 are plausible.
- Lux changes clearly when VEML7700 is covered and exposed.
- RainPoint reports dry for a closed contact and rain for an open contact.

## 3. Battery telemetry

- `battery_raw_v` is plausible.
- Calibrated `battery_v` is compared with a multimeter.
- Battery percentage and status follow the current calibration table.

## 4. Wi-Fi configuration

- Configuration AP starts after physical power-on/reset.
- `192.168.4.1` opens the local page.
- Network scan works.
- Credentials can be saved.
- Station reconnects to the configured 2.4 GHz network.
- IP address and RSSI are displayed.

## 5. UDP protocol

On UDP port 4210 verify:

- periodic telemetry is broadcast,
- `TREX_DISCOVER_V1` returns an `announce` packet while the station is awake,
- `TREX_GET:*` returns current telemetry,
- `TREX_GET:<serial>` works for the station's own serial,
- the JSON contains `protocol: trex-wall/1`.

## 6. Sleep and rain wake

- Normal periodic wake occurs according to the firmware interval.
- While dry/HIGH, changing RainPoint to LOW wakes the station.
- While raining/LOW, changing RainPoint to HIGH wakes the station.
- A sustained rain state does not cause uncontrolled repeated wakeups.

## 7. Mechanical check

- Housing closes without pinching wires.
- No PCB is mechanically loaded by the lid.
- Rain mechanism moves freely.
- VEML7700 is not optically blocked.
- BME280 has access to ambient air while remaining protected from direct water ingress.

## Release evidence

For a public release keep at least:

- one internal assembly photo,
- one RainPoint/rain-contact detail photo,
- one web dashboard screenshot,
- one Wi-Fi configuration screenshot,
- the firmware version used during the test.
