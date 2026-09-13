# Instrukcja budowy

**[English](build.md) | Polski**

Ta instrukcja dotyczy aktualnego prototypu T-REX Facade Weather Station zbudowanego wewnątrz obudowy RainPoint.

## Części

| Element | Uwagi |
| --- | --- |
| ESP32-C3 Super Mini | główny sterownik i Wi-Fi |
| BME280, 3.3 V | w testowanym egzemplarzu adres I2C `0x76` |
| VEML7700 | adres I2C `0x10` |
| RainPoint | wykorzystany jednocześnie jako obudowa i czujnik deszczu |
| bateria / zasilanie 1S | zależnie od wykonania mechanicznego |
| rezystor 220 kOhm | górna część dzielnika napięcia baterii |
| rezystor 100 kOhm | dolna część dzielnika napięcia baterii |
| kondensator 100 nF | filtr ADC między GPIO3 a GND |
| przewody, izolacja, koszulki termokurczliwe | do montażu prototypowego |

## Połączenia elektryczne

Autorytatywny opis połączeń znajduje się w [../hardware/wiring_PL.md](../hardware/wiring_PL.md).

Najważniejsze piny:

| Funkcja | ESP32-C3 |
| --- | --- |
| SDA | GPIO8 |
| SCL | GPIO9 |
| RainPoint | GPIO4 |
| ADC baterii | GPIO3 |

RainPoint jest podłączony jako `3V3 -> styk bezpotencjałowy -> GPIO4`, a GPIO4 pracuje jako `INPUT_PULLDOWN`.

- styk zamknięty / HIGH = sucho
- styk otwarty / LOW = deszcz

## Montaż mechaniczny

Prototyp celowo wykorzystuje oryginalną obudowę RainPoint zamiast osobnej projektowanej obudowy pogodowej.

Praktyczna kolejność montażu:

1. Pozostaw oryginalny mechaniczny styk deszczowy RainPoint na swoim miejscu.
2. Umieść ESP32-C3 tak, aby podczas uruchamiania pozostał dostęp do USB.
3. BME280 umieść w miejscu mającym kontakt z powietrzem zewnętrznym, ale chronionym przed bezpośrednim dostaniem się wody.
4. VEML7700 ustaw tak, aby obudowa ani przewody nie zasłaniały toru optycznego.
5. Poprowadź przewody styku RainPoint do GPIO4 i 3.3 V.
6. Elementy dzielnika napięcia baterii umieść możliwie blisko wejścia ADC ESP32.
7. Zamocuj przewody tak, aby zamknięcie obudowy nie mogło ich przyciąć ani naciskać na czujniki.
8. Sprawdź, czy obudowa zamyka się bez mechanicznego obciążania PCB i złączy.

Zdjęcia aktualnego prototypu są w [gallery_PL.md](gallery_PL.md).

## Wgrywanie firmware

Projekt PlatformIO znajduje się w katalogu `firmware/`.

```sh
cd firmware
pio run
pio run -t upload
pio device monitor -b 115200
```

Target: `esp32-c3-devkitm-1`.

Przy Arduino IDE wybierz płytkę zgodną z ESP32-C3 Super Mini. Nie kompiluj tego firmware dla ESP32-S2 ani dla ogólnego targetu niebędącego C3.

## Pierwsze uruchomienie

Po fizycznym włączeniu zasilania lub resecie stacja uruchamia tryb konfiguracji i tworzy sieć o nazwie w rodzaju:

`T-REX-FS-XXXXXXXX`

Połącz się z nią i otwórz:

`http://192.168.4.1`

W panelu wyszukaj sieć Wi-Fi 2.4 GHz, wpisz hasło i zapisz konfigurację.

## Kontrola przed zamknięciem obudowy

Sprawdź wszystkie punkty:

- BME280 pokazuje temperaturę, wilgotność i ciśnienie.
- Wartość VEML7700 zmienia się po zasłonięciu/odsłonięciu czujnika.
- RainPoint pokazuje stan suchy przy zamkniętym styku.
- Otwarcie styku RainPoint zmienia stan na deszcz.
- Napięcie baterii jest zbliżone do pomiaru multimetrem.
- Wi-Fi łączy się i widoczne są IP oraz RSSI.
- Telemetria UDP jest odbierana na porcie 4210.

Po zamknięciu obudowy wykonaj ponownie test deszczu i połączenia Wi-Fi.

## Uwagi dotyczące montażu na zewnątrz

Repozytorium dokumentuje działający prototyp, a nie certyfikowany produkt zewnętrzny. Stację należy zamontować tak, aby mechanizm RainPoint działał prawidłowo, czujniki mierzyły rzeczywiste warunki na elewacji, a elektronika była możliwie dobrze chroniona przed bezpośrednim dostaniem się wody.
