# Protokół UDP stacji fasadowej T-REX

**[English](protocol.md) | Polski**

To jest prosty protokół UDP, a nie HTTP API.

## Transport

- IPv4 UDP, port: `4210`
- znacznik protokołu: `trex-wall/1`
- zwykła telemetria: broadcast IPv4 w podsieci
- odpowiedzi: unicast do adresu IP i portu źródłowego nadawcy
- zapytania: tekst ASCII
- odpowiedzi: JSON UTF-8

Bateryjna stacja nasłuchuje zapytań przez około 8 sekund po wysłaniu normalnej telemetrii. Jeżeli śpi, discovery należy powtórzyć przy kolejnym wybudzeniu.

## Zapytania

| Datagram | Znaczenie | Odpowiedź |
| --- | --- | --- |
| `TREX_DISCOVER_V1` | wykrywanie aktualnie aktywnych stacji | JSON `announce` |
| `TREX_GET:*` | zapytanie do dowolnej aktywnej stacji | JSON `telemetry` |
| `TREX_GET:TRX-FS-XXXXXXXX` | zapytanie do konkretnego numeru seryjnego | JSON `telemetry`, jeśli serial pasuje |

## Schemat telemetrii

| Pole | Typ | Znaczenie |
| --- | --- | --- |
| `protocol` | string | stała wartość `trex-wall/1` |
| `type` | string | `telemetry` albo `announce` |
| `serial` | string | stały identyfikator stacji `TRX-FS-XXXXXXXX` |
| `fw_version` | string | wersja firmware, obecnie `1.2.2` |
| `hw_version` | string | wersja hardware, obecnie `1.0` |
| `ip` | string/null | aktualny IPv4 podczas połączenia |
| `temperature_c` | number/null | temperatura BME280 w °C |
| `humidity_pct` | number/null | wilgotność względna BME280 w % |
| `pressure_hpa` | number/null | ciśnienie BME280 w hPa |
| `lux` | number/null | natężenie oświetlenia VEML7700 w lux |
| `rain` | boolean | `true`, gdy RainPoint daje LOW na GPIO4 |
| `battery_raw_v` | number | napięcie baterii przed korekcją tabelą kalibracyjną |
| `battery_v` | number | napięcie baterii po kalibracji |
| `battery_pct` | integer | szacowany poziom 0–100% |
| `battery_status` | string | `OK`, `LOW` albo `CRITICAL` |
| `rssi` | integer/null | RSSI Wi-Fi w dBm |

Gdy BME280 albo VEML7700 jest niedostępny, odpowiadające mu wartości są wysyłane jako `null`.

## Przykładowy pakiet

```json
{"protocol":"trex-wall/1","type":"telemetry","serial":"TRX-FS-1D3F1FB8","fw_version":"1.2.2","hw_version":"1.0","ip":"192.168.1.123","temperature_c":21.45,"humidity_pct":58.2,"pressure_hpa":1008.34,"lux":12540.0,"rain":false,"battery_raw_v":4.179,"battery_v":4.2,"battery_pct":100,"battery_status":"OK","rssi":-61}
```

## Minimalny odbiornik Python

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

Dla zapytania kierowanego wyślij `TREX_GET:*` albo `TREX_GET:TRX-FS-XXXXXXXX` na port `4210`.
