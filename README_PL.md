# T-REX Facade Weather Station

**[English](README.md) | Polski**

## Czym jest T-REX Facade Weather Station

Otwarta stacja pogodowa DIY montowana na elewacji, oparta na ESP32-C3 Super Mini.

Mierzy temperaturę, wilgotność, ciśnienie, natężenie oświetlenia, stan deszczu, napięcie baterii i RSSI Wi-Fi. Dane publikuje przez UDP i może działać bez T-REX Smart Home.

Stacja powstała jako element automatyki rolet T-REX Smart Home, ale jest niezależnym projektem i można ją zintegrować z własnym systemem.

## Dokumentacja

- [Instrukcja budowy](docs/build_PL.md) — części, montaż, pierwsze uruchomienie i kontrola przed zamknięciem obudowy.
- [Galeria prototypu](docs/gallery_PL.md) — obudowa RainPoint, wnętrze i panel WWW.
- [Połączenia](hardware/wiring_PL.md) — pinout i funkcjonalny diagram połączeń.
- [Protokół UDP](docs/protocol_PL.md) — discovery, telemetria i schema JSON.
- [Checklista walidacji](docs/testing_PL.md) — powtarzalne sprawdzenie hardware i firmware.
- [Changelog](CHANGELOG.md)
- [Zasady współtworzenia](CONTRIBUTING.md)

## Funkcje

- Firmware v1.2.2 dla ESP32-C3 Super Mini.
- BME280 (`0x76`) do pomiaru temperatury, wilgotności i ciśnienia.
- VEML7700 (`0x10`) do pomiaru natężenia oświetlenia.
- RainPoint jako wejście bezpotencjałowe z wybudzaniem z deep sleep po zmianie stanu deszczu.
- Pomiar napięcia baterii z edytowalną tabelą kalibracyjną.
- Portal konfiguracji Wi-Fi oraz telemetria i discovery przez UDP.

## Obudowa prototypu i wykorzystanie RainPoint

W aktualnym prototypie wykorzystujemy obudowę RainPoint jako obudowę całej stacji fasadowej.

Oryginalny mechaniczny czujnik deszczu RainPoint pozostaje aktywnym elementem urządzenia i jest używany jako bezpotencjałowy styk sygnalizujący stan deszczu. W tej samej obudowie umieszczone są ESP32-C3, BME280, VEML7700, układ pomiaru baterii oraz zasilanie.

Dzięki temu nie trzeba projektować od zera osobnej obudowy odpornej na warunki zewnętrzne, a oryginalny mechanizm detekcji deszczu pozostaje częścią gotowej stacji.

Dokładne rozmieszczenie elementów może zależeć od wersji obudowy RainPoint oraz użytych podzespołów. Czujniki środowiskowe powinny mieć kontakt z warunkami, które mają mierzyć, a elektronika powinna być chroniona przed bezpośrednim dostaniem się wody.

Aktualny montaż można zobaczyć w [galerii prototypu](docs/gallery_PL.md).

## Sprzęt

| Element | Zastosowanie |
| --- | --- |
| ESP32-C3 Super Mini | sterownik i Wi-Fi |
| BME280 | temperatura, wilgotność i ciśnienie |
| VEML7700 | natężenie oświetlenia |
| RainPoint | obudowa prototypu i bezpotencjałowy czujnik deszczu |
| bateria 1S + dzielnik napięcia | zasilanie i telemetria baterii |

Pełne połączenia i diagram znajdują się w [hardware/wiring_PL.md](hardware/wiring_PL.md).

## Podłączenie

| Sygnał | Pin ESP32-C3 |
| --- | --- |
| I2C SDA | GPIO8 |
| I2C SCL | GPIO9 |
| RainPoint | GPIO4 |
| ADC baterii | GPIO3 |

Oba moduły I2C zasilaj napięciem 3.3 V i połącz ze wspólną masą.

RainPoint podłącz jako `3V3 -> styk bezpotencjałowy -> GPIO4`. GPIO4 pracuje jako `INPUT_PULLDOWN`.

- styk zamknięty / HIGH = sucho
- styk otwarty / LOW = deszcz

## Firmware

Projekt PlatformIO znajduje się w katalogu [`firmware/`](firmware/).

```sh
cd firmware
pio run
pio run -t upload
pio device monitor -b 115200
```

Target to `esp32-c3-devkitm-1`, zgodny z ESP32-C3 Super Mini.
Port COM należy wybrać lokalnie — nie jest zapisany w repozytorium.

## Konfiguracja Wi-Fi

Po fizycznym uruchomieniu lub resecie stacja wchodzi w tryb konfiguracji i tworzy punkt dostępowy:

`T-REX-FS-XXXXXXXX`

1. Połącz się z tym Wi-Fi.
2. Otwórz `http://192.168.4.1`.
3. Wyszukaj sieci, wybierz SSID 2.4 GHz i zapisz hasło.
4. Dane logowania są przechowywane lokalnie w Preferences/NVS.

W normalnym trybie stacja: wybudza się, wykonuje pomiary, łączy z Wi-Fi, wysyła broadcast telemetrii, przez około 8 sekund nasłuchuje zapytań UDP, a następnie wraca do deep sleep.

## Protokół UDP

Port UDP: **4210**  
Znacznik protokołu: **`trex-wall/1`**

| Zapytanie | Odpowiedź |
| --- | --- |
| `TREX_DISCOVER_V1` | JSON z `type: "announce"` |
| `TREX_GET:*` | aktualna telemetria JSON |
| `TREX_GET:TRX-FS-XXXXXXXX` | aktualna telemetria tylko stacji o pasującym numerze seryjnym |

Pełny opis protokołu znajduje się w [docs/protocol_PL.md](docs/protocol_PL.md).

## Format telemetrii

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

Jeżeli BME280 albo VEML7700 nie jest dostępny, wartości powiązane z tym czujnikiem są wysyłane jako JSON `null`.

## Deep sleep i wybudzanie

Okresowe wybudzenie jest ustawione co 10 minut.

- gdy jest sucho (`HIGH`), GPIO4 oczekuje na `LOW`, czyli początek deszczu,
- gdy pada (`LOW`), GPIO4 oczekuje na `HIGH`, czyli powrót do stanu suchego.

Dzięki temu zmiana stanu RainPoint może wybudzić stację niezależnie od okresowego timera.

## Pomiar baterii

```text
BAT+ -- 220 kOhm --+-- GPIO3
                   |
                 100 kOhm
                   |
                  GND

GPIO3 -- 100 nF -- GND
```

Firmware zawiera tabelę kalibracyjną przygotowaną dla prototypu. Przy innym egzemplarzu ESP32 lub innych rezystorach warto porównać wskazania z multimetrem i w razie potrzeby dostosować tabelę.

## Ograniczenia

- Punkt dostępowy do konfiguracji Wi-Fi jest otwarty — konfigurację wykonuj w zaufanym otoczeniu.
- Stacja odpowiada na discovery tylko podczas krótkiego okresu aktywności po wybudzeniu.
- Aktualny firmware ma `CONFIG_TIME_MS` ustawione na 10 minut, mimo że jeden z komunikatów tekstowych nadal mówi o 20 minutach.
- Repo nie zawiera gotowej integracji Home Assistant; dane można odbierać bezpośrednio z UDP albo przepuścić przez własny bridge.
- Integracja mechaniczna z obudową RainPoint dotyczy prototypu; sposób montażu może się różnić między wykonaniami.

## Licencja

MIT. Zobacz [LICENSE](LICENSE).
