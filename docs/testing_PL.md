# Checklista walidacji

**[English](testing.md) | Polski**

Używaj tej checklisty po złożeniu stacji, po zmianach firmware oraz przed publikacją release.

## 1. Start i identyfikacja

- Numer seryjny zaczyna się od `TRX-FS-`.
- W panelu WWW widoczne są wersje firmware i hardware.
- Nazwa sieci konfiguracyjnej zawiera ten sam numer seryjny.

## 2. Czujniki

- Temperatura, wilgotność i ciśnienie z BME280 mają wiarygodne wartości.
- Lux wyraźnie zmienia się po zasłonięciu i odsłonięciu VEML7700.
- RainPoint pokazuje sucho przy zamkniętym styku i deszcz przy otwartym styku.

## 3. Telemetria baterii

- `battery_raw_v` ma wiarygodną wartość.
- Skalibrowane `battery_v` jest porównane z multimetrem.
- Procent i status baterii odpowiadają aktualnej tabeli kalibracyjnej.

## 4. Konfiguracja Wi-Fi

- Po fizycznym włączeniu/resetcie uruchamia się AP konfiguracyjny.
- Strona `192.168.4.1` otwiera się lokalnie.
- Skanowanie sieci działa.
- Dane Wi-Fi można zapisać.
- Stacja ponownie łączy się z zapisaną siecią 2.4 GHz.
- Widoczne są adres IP i RSSI.

## 5. Protokół UDP

Na porcie UDP 4210 sprawdź:

- okresowy broadcast telemetrii,
- `TREX_DISCOVER_V1` zwraca pakiet `announce`, gdy stacja jest aktywna,
- `TREX_GET:*` zwraca aktualną telemetrię,
- `TREX_GET:<serial>` działa dla własnego numeru seryjnego stacji,
- JSON zawiera `protocol: trex-wall/1`.

## 6. Deep sleep i wybudzanie przez deszcz

- Okresowe wybudzenie działa zgodnie z interwałem firmware.
- Przy stanie sucho/HIGH zmiana RainPoint na LOW wybudza stację.
- Przy stanie deszcz/LOW zmiana RainPoint na HIGH wybudza stację.
- Stały stan deszczu nie powoduje niekontrolowanych, powtarzających się wybudzeń.

## 7. Kontrola mechaniczna

- Obudowa zamyka się bez przycinania przewodów.
- Pokrywa nie naciska mechanicznie na PCB.
- Mechanizm RainPoint porusza się swobodnie.
- VEML7700 nie jest zasłonięty optycznie.
- BME280 ma kontakt z powietrzem zewnętrznym, ale jest chroniony przed bezpośrednim dostaniem się wody.

## Materiał do release

Przed publicznym release zachowaj co najmniej:

- jedno zdjęcie wnętrza stacji,
- jedno zdjęcie mechanizmu/styku RainPoint,
- jeden screenshot głównego panelu WWW,
- jeden screenshot konfiguracji Wi-Fi,
- numer wersji firmware użytej podczas testu.
