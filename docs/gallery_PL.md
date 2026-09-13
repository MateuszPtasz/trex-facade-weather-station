# Galeria prototypu

**[English](gallery.md) | Polski**

Ta strona dokumentuje aktualny, działający prototyp T-REX Facade Weather Station.

## Montaż wewnątrz obudowy

W aktualnym prototypie wykorzystana jest oryginalna obudowa RainPoint. Sterownik ESP32-C3 oraz elektronika czujników i zasilania są upakowane wewnątrz, a oryginalny mechaniczny styk deszczu RainPoint pozostaje aktywną częścią stacji.

![Montaż wewnętrzny](../images/station-internal.jpg)

## Czujnik deszczu i detal obudowy

Oryginalny mechanizm bezpotencjałowego styku RainPoint pozostaje w układzie i jest podłączony do wejścia deszczu ESP32-C3.

![Detal czujnika deszczu](../images/station-rain-sensor.jpg)

## Widok obudowy

Zdjęcie pokazuje, jak elektronika mieści się w korpusie RainPoint przed ostatecznym zamknięciem obudowy.

![Widok obudowy](../images/station-enclosure.jpg)

## Interfejs WWW

Po fizycznym włączeniu lub resecie stacja udostępnia lokalny panel konfiguracyjny. Panel pokazuje identyfikator stacji, wersję firmware i hardware, aktualne wartości czujników, stan baterii oraz status Wi-Fi.

![Panel WWW](../images/web-dashboard.jpg)

Dolna część strony umożliwia skanowanie sieci Wi-Fi, zapis danych dostępowych oraz wykonanie podstawowych czynności diagnostycznych.

![Konfiguracja WWW](../images/web-config.jpg)

## Uwagi

Zdjęcia przedstawiają aktualny prototyp, a nie produkcyjną płytkę PCB ani finalną konstrukcję mechaniczną. Rozmieszczenie elementów może się zmieniać pomiędzy egzemplarzami. Elementami odtwarzalnymi projektu są przede wszystkim połączenia elektryczne, zachowanie firmware oraz logika styku RainPoint opisana w pozostałej dokumentacji repozytorium.
