# T-REX facade station UDP protocol

**English | [Polski](protocol_PL.md)**

This is a UDP protocol, not an HTTP API.

## Transport

- IPv4 UDP port: `4210`
- protocol marker: `trex-wall/1`
- normal telemetry: IPv4 subnet broadcast
- responses: unicast to the sender IP and source port
- requests: ASCII text; responses: UTF-8 JSON

The battery-powered station listens for requests for about eight seconds after normal telemetry.
Repeat discovery if the station is sleeping.

## Requests

| Datagram | Meaning | Response |
| --- | --- | --- |
| `TREX_DISCOVER_V1` | discover awake stations | JSON `announce` |
| `TREX_GET:*` | query any awake station | JSON `telemetry` |
| `TREX_GET:TRX-FS-XXXXXXXX` | query one serial | JSON `telemetry` when it matches |

## Telemetry schema

| Field | Type | Meaning |
| --- | --- | --- |
| `protocol` | string | literal `trex-wall/1` |
| `type` | string | `telemetry` or `announce` |
| `serial` | string | stable `TRX-FS-XXXXXXXX` station ID |
| `fw_version` | string | firmware version, currently `1.2.2` |
| `hw_version` | string | hardware version, currently `1.0` |
| `ip` | string/null | IPv4 while connected |
| `temperature_c` | number/null | BME280 degrees Celsius |
| `humidity_pct` | number/null | BME280 relative humidity percent |
| `pressure_hpa` | number/null | BME280 pressure hPa |
| `lux` | number/null | VEML7700 illuminance |
| `rain` | boolean | true when RainPoint GPIO4 is LOW |
| `battery_raw_v` | number | voltage before calibration |
| `battery_v` | number | voltage after calibration |
| `battery_pct` | integer | estimate from 0 to 100 |
| `battery_status` | string | `OK`, `LOW` or `CRITICAL` |
| `rssi` | integer/null | Wi-Fi RSSI dBm |

When BME280 or VEML7700 is unavailable, that sensor's values are `null`.

## Example packet

```json
{"protocol":"trex-wall/1","type":"telemetry","serial":"TRX-FS-1D3F1FB8","fw_version":"1.2.2","hw_version":"1.0","ip":"192.168.1.123","temperature_c":21.45,"humidity_pct":58.2,"pressure_hpa":1008.34,"lux":12540.0,"rain":false,"battery_raw_v":4.179,"battery_v":4.2,"battery_pct":100,"battery_status":"OK","rssi":-61}
```

## Minimal Python receiver

```python
import socket
import time

port = 4210
sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)
sock.bind(("", port))
sock.settimeout(1.0)
last_discovery = 0.0

while True:
    if time.monotonic() - last_discovery > 5:
        sock.sendto(b"TREX_DISCOVER_V1", ("255.255.255.255", port))
        last_discovery = time.monotonic()
    try:
        packet, sender = sock.recvfrom(2048)
        print(sender, packet.decode("utf-8"))
    except socket.timeout:
        pass
```

For a directed request, send `TREX_GET:*` or `TREX_GET:TRX-FS-XXXXXXXX` to port `4210`.
