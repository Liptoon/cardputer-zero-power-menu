# CardputerZero Power Menu

Aplikacja LVGL 9.5 dla M5Stack CardputerZero (ekran 320 x 170, Linux framebuffer),
która pozwala bezpiecznie zrestartować albo wyłączyć system z menu APPLaunch.

## Co robi

- Wyświetla listę: **Restart**, **Shut down**, **Exit**.
- Każda akcja systemowa wymaga potwierdzenia w oknie dialogowym.
- Restart uruchamia `systemctl reboot`, a wyłączenie `systemctl poweroff`.
  Gdy `systemctl` jest niedostępny, używane są kolejno `reboot` / `poweroff`
  i `shutdown -r now` / `shutdown -h now`.
- `Exit` zamyka aplikację i wraca do APPLaunch.

Aplikacja nie wywołuje `sudo`: APPLaunch uruchamia wpisy jako `root`, więc
polecenia systemowe mają wymagane uprawnienia.

## Sterowanie

| Klawisz | Działanie |
|---|---|
| `↑` / `↓` (lub `F` / `X`, `K`/`J`, `W`/`S`) | wybór pozycji |
| `←` / `→` (lub `Z` / `C`, `H`/`L`, `A`/`D`) | wybór pozycji |
| `Tab` | następna pozycja |
| `Enter` / `Spacja` | zatwierdzenie |
| `1`, `2`, `3` | skrót do pozycji menu |
| `Esc` (krótko) | powrót / anulowanie |
| `Esc` (przytrzymany ok. 0,7 s) | wyjście z aplikacji |

Na CardputerZero dokumentacja M5Stack podaje `Fn+F`/`Fn+X` (góra/dół)
oraz `Fn+Z`/`Fn+C` (lewo/prawo). W aplikacjach bez pola tekstowego te same
klawisze działają także bez `Fn`: `F`, `X`, `Z`, `C`. Mapowanie `;`, `,`, `.`, `/`
dotyczy Cardputera ADV i nie jest używane.
Klawisze można zmienić w `main/src/keypad.cpp` (`handle_event`).

Po zatwierdzeniu restartu lub wyłączenia ekran z animacją zostaje na miejscu
i pozostaje widoczny aż do faktycznego rebootu/poweroffu. Nie pojawia się
komunikat z wynikiem polecenia. Komunikat wyniku jest używany tylko w trybie
`--dry-run` i przy błędzie wywołania.

## Wymagania

- LVGL 9.5 w `third_party/lvgl` — pobiera `tools/fetch_lvgl.sh`.
- Kompilator hosta: `g++`, `cmake`, `make`.
- Do builda urządzenia: `aarch64-linux-gnu-g++` (np. pakiet
  `g++-aarch64-linux-gnu`).
- Opcjonalnie ImageMagick (`magick`) do konwersji podglądów PPM na PNG.

## Budowanie

```bash
tools/fetch_lvgl.sh        # jednorazowo: LVGL v9.5.0
make                      # build dla hosta, sprawdzenie kompilacji
make device               # build AArch64 -> dist/power-menu
make preview              # zrzuty ekranu 320x170 -> build/preview/shots
make check                # walidacja .desktop, ikony i binarki
```

Build urządzenia używa domyślnie `aarch64-linux-gnu-`; inny prefiks podaj przez
`make device DEVICE_CROSS_COMPILE=/sciezka/do/toolchain-`.

Binarka na urządzenie jest domyślnie linkowana **statycznie** (`STATIC=1`),
żeby uniknąć niezgodności glibc/libstdc++ z wersją na CardputerZero. Wersję
dynamiczną zbudujesz przez `make device STATIC=0`.

Toolchain bez roota: gotowy zestaw Bootlin (glibc, 80 MB) wystarczy rozpakować
w katalogu domowym, np.:

```bash
curl -fL -o aarch64.tar.xz \
  https://toolchains.bootlin.com/downloads/releases/toolchains/aarch64/tarballs/aarch64--glibc--stable-2024.05-1.tar.xz
tar -xf aarch64.tar.xz -C ~/toolchains
make device DEVICE_CROSS_COMPILE=~/toolchains/aarch64--glibc--stable-2024.05-1/bin/aarch64-buildroot-linux-gnu-
```

Na tej maszynie toolchain jest już rozpakowany w
`/home/publius/toolchains/aarch64--glibc--stable-2024.05-1`.

## Instalacja na CardputerZero

```bash
make device
make stage                # drzewo pakietu w build/stage
```

Następnie, jako `root` na urządzeniu:

```bash
cp -a build/stage/usr/share/APPLaunch/. /usr/share/APPLaunch/
```

albo `make install` (wymaga uruchomienia jako root; nie używa `sudo`).
Restart APPLaunch nie jest potrzebny: launcher skanuje katalog `applications`
i wykrywa nowy wpis sam. Wpis **Power Menu** pojawia się na karuzeli po kilku
sekundach. Jeśli jednostka systemd istnieje, `make install` i `tools/deploy.sh`
uruchomią ją automatycznie; w przeciwnym razie pomijają restart z komunikatem.

Gotowy pakiet z tej maszyny: `dist/power-menu_0.1.2_arm64.deb`
(binarka AArch64, statyczna — nie wymaga zgodności wersji glibc na urządzeniu).

### Instalacja ręczna z konta `pi`

Na komputerze (podmień `<HOST>` na adres CardputerZero):

```bash
scp dist/power-menu_0.1.2_arm64.deb pi@<HOST>:/tmp/
```

Na CardputerZero:

```bash
sudo dpkg -i /tmp/power-menu_0.1.2_arm64.deb
sudo ls -l /usr/share/APPLaunch/applications/power-menu.desktop \
           /usr/share/APPLaunch/bin/power-menu \
           /usr/share/APPLaunch/share/images/power-menu.png
```

Nie wykonuj `systemctl restart APPLaunch.service` — na tym urządzeniu nie ma
takiej jednostki (`Unit APPLaunch.service not found`). Aplikacja i tak działa,
bo APPLaunch wykrywa nowe wpisy sam. Jeśli chcesz sprawdzić, jak launcher jest
uruchomiony:

```bash
ps aux | grep -i applaunch | grep -v grep
systemctl list-units --all --no-pager | grep -i applaunch
```

Jeśli instalacja przez `dpkg` się nie uda, można skopiować samo drzewo z
`build/stage/usr/share/APPLaunch` do `/usr/share/APPLaunch` — zawartość jest
identyczna z zawartością pakietu.

## Pakiet .deb

```bash
make deb
# dist/power-menu_0.1.2_arm64.deb
```

## Wdrożenie przez SSH

Skrypt `tools/deploy.sh` (albo `make deploy`) buduje paczkę, wysyła ją przez
`scp`, instaluje `dpkg -i` i — jeśli jednostka systemd istnieje — restartuje
APPLaunch. Wymaga logowania jako `root`
albo konta z uprawnieniami do zapisu w `/usr/share/APPLaunch`; nie wywołuje
`sudo` samodzielnie — jeśli użytkownik nie jest rootem, wypisuje gotowe
polecenia do wykonania.

```bash
tools/deploy.sh --host pi@cardputer.local              # build + deb + install
tools/deploy.sh --host pi@cardputer.local --no-build    # użyj gotowego .deb
CARDPUTER_HOST=pi@cardputer.local tools/deploy.sh --dry-run
make deploy HOST=pi@cardputer.local METHOD=copy         # tar zamiast dpkg
```

| Zmienna / opcja | Znaczenie |
|---|---|
| `CARDPUTER_HOST` / `--host` | `user@host` (wymagane) |
| `CARDPUTER_PORT` / `--port` | port SSH, domyślnie 22 |
| `CARDPUTER_KEY` / `--key` | plik klucza SSH |
| `--method deb\|copy` | instalacja pakietu (domyślnie) albo kopia drzewa |
| `--no-build` | pomiń `make device && make deb` |
| `--dry-run` | pokaż polecenia bez wykonywania |

Wymagania: klucz SSH do urządzenia (`BatchMode`, bez hasła) i zainstalowany
toolchain `aarch64-linux-gnu-g++` na komputerze budującym.

Alternatywnie ten sam staging można spakować skryptem ze skilla
`cardputer-zero-application`:

```bash
python3 ~/.config/opencode/skills/cardputer-zero-application/scripts/make_applauncher_package.py \
  --app-name "Power Menu" \
  --binary dist/power-menu \
  --icon applaunch/share/images/power-menu.png \
  --description "Restart or shut down the CardputerZero safely" \
  --out build/applauncher-packages --deb
```

## Tryb testowy i konfiguracja

Flaga `--dry-run` (albo `POWER_MENU_DRY_RUN=1`) wypisuje polecenie systemowe
zamiast je wykonywać:

```bash
./build/host/bin/power-menu --dry-run
```

| Zmienna | Znaczenie |
|---|---|
| `POWER_MENU_FB_DEVICE` | framebuffer, np. `/dev/fb1` |
| `POWER_MENU_KEY_DEVICE` | urządzenie evdev klawiatury |
| `POWER_MENU_DRY_RUN` | `1`/`true`/`yes` — nie wykonuj akcji |

Bez zmiennych aplikacja szuka `fb_st7789v` w `/proc/fb` (na CardputerZero to
zwykle `/dev/fb1`, nie `/dev/fb0`) oraz klawiatury TCA8418 w
`/dev/input/by-path/platform-3f804000.i2c-event`, a na komputerze deweloperskim
pierwszej klawiatury w `/dev/input/event*`.

## Struktura

```
main/src/          źródła aplikacji (UI, evdev, akcje systemowe, platforma)
applaunch/         wpis .desktop i ikona w układzie /usr/share/APPLaunch
store/screenshots/ cztery zrzuty 320x170 do sklepu z aplikacjami
tests/             bezgłowy renderer podglądu (LVGL bez framebufferu)
tools/             pobieranie LVGL
lv_conf.h          konfiguracja LVGL (fbdev, evdev, czcionki Montserrat)
```

## Ikona

`applaunch/share/images/power-menu.png` (256 x 256) powstała z symbolu
`power_settings_new` z Google Material Symbols (licencja Apache 2.0) —
źródło i atrybucja w `assets/ICON-SOURCE.md`.
