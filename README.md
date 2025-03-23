# Stacje Pomiarowe

Aplikacja do wyszukiwania i analizy stacji pomiarowych jakości powietrza, zbudowana przy użyciu Qt i QML.

## Opis
Aplikacja pozwala użytkownikom:
- Wyszukiwać wszystkie stacje pomiarowe w Polsce.
- Wyszukiwać stacje w określonej miejscowości.
- Znaleźć najbliższą stację na podstawie podanej lokalizacji.
- Przeglądać historię wyszukiwania.
- Analizować dane historyczne (wykresy i statystyki).

## Wymagania
- Qt 6.2 lub nowsze (z modułami `core`, `gui`, `network`, `qml`, `quick`, `positioning`, `location`).
- Kompilator C++17 (np. MSVC, GCC, Clang).
- Doxygen (do generowania dokumentacji).
- Internet (do pobierania danych z API).

## Instalacja i kompilacja
1. Sklonuj repozytorium:
   ```bash
   git clone https://github.com/<twoj-username>/stacje_pomiarowe.git
   cd stacje_pomiarowe