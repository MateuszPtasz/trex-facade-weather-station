# Prototype gallery

**English | [Polski](gallery_PL.md)**

This page documents the current working T-REX Facade Weather Station prototype.

## Internal assembly

The current prototype reuses the RainPoint enclosure. The ESP32-C3 controller and sensor/power electronics are packed inside the original housing while the RainPoint mechanical rain contact remains in use.

![Internal assembly](../images/station-internal.jpg)

## Rain sensor and enclosure detail

The original RainPoint dry-contact mechanism remains part of the build and is connected to the ESP32-C3 rain input.

![Rain sensor detail](../images/station-rain-sensor.jpg)

## Enclosure view

This shows how the electronics fit into the RainPoint body before final closure.

![Enclosure view](../images/station-enclosure.jpg)

## Web interface

After power-on/reset the station exposes a local configuration interface. The dashboard shows the station identity, firmware/hardware version, current sensor values, battery state and Wi-Fi status.

![Web dashboard](../images/web-dashboard.jpg)

The lower part of the page provides Wi-Fi network scanning, credential setup and diagnostic actions.

![Web configuration](../images/web-config.jpg)

## Notes

These photographs show the current prototype rather than a production PCB or final mechanical design. Component placement may change between builds. The important reproducible parts are the electrical wiring, firmware behavior and RainPoint dry-contact logic documented elsewhere in this repository.
