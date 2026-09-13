# Prototype gallery

**English | [Polski](gallery_PL.md)**

This page documents the current working T-REX Facade Weather Station prototype.
Only public-safe photographs and screenshots are included: no saved Wi-Fi
credentials, private SSID or local network address is visible.

## RainPoint enclosure and power module

![RainPoint enclosure and stable 5 V module](../images/station-enclosure-power-module.jpeg)

The prototype reuses the RainPoint enclosure. An MT-series module provides a
stable 5 V supply inside the housing.

## Internal assembly

![ESP32-C3 Mini and BME sensor](../images/station-internal-esp32-c3-bme.jpeg)

ESP32-C3 Mini controller and BME temperature, humidity and pressure sensor
mounted in the prototype enclosure.

![Ambient-light sensor](../images/station-light-sensor.jpeg)

Dedicated ambient-light sensor prepared for the facade station.

## Web interface

![Wi-Fi configuration page](../images/web-config.png)

The local web panel provides Wi-Fi setup and basic diagnostics. The screenshot
shows an empty configuration form only; it does not disclose a saved network or
password.

## Notes

These photographs show the current prototype rather than a production PCB or
final mechanical design. Component placement may change between builds. The
reproducible parts are the electrical wiring, firmware behavior and RainPoint
dry-contact logic documented elsewhere in this repository.
