# Сторонние компоненты и лицензии

Права на оригинальную игру «Ядерный Титбит» и её материалы сохраняются
за их правообладателями. Проект не предоставляет лицензию на эти материалы
и не заявляет о разрешении правообладателей на публикацию.
Условия перечисленных ниже сторонних компонентов сохраняются отдельно.

## Сторонние библиотеки в репозитории

| Библиотека | Где | Лицензия |
|---|---|---|
| SeeR 0.94a, П. Подсядлы (1998–99) | `src/seer` | бесплатная библиотека, условия в `src/seer/README.TXT` |
| IJG JPEG 6a | `src/jpeg` | IJG License (README в каталоге) |
| Audiere | `src/audiere` | [GNU LGPL 2.1](docs/licenses/audiere-LGPL-2.1.txt) |
| Ogg / Vorbis (Xiph.Org) | `src/lanes/codex/ogg_*`, `vorbis_*`, `third_party/audiere-reference` | [Ogg](docs/licenses/libogg-1.0-COPYING), [Vorbis](docs/licenses/libvorbis-1.0-COPYING); оригинальные тексты из архивов 1.0 |
| WTL 7.5 (фрагменты) | `src/native/app_module*`, `archive_*` | Common Public License 1.0 (`src/native/app_module-CPL.TXT`) |
| three.js (редактор) | `editor/web/vendor` | MIT |
| zlib 1.1.3 | `third_party/zlib-1.1.3` | условия в её README / zlib.h |
| CenterPoint/XML | `src/lanes/engine41/cpxml43`, `engine42/cpxml` | CenterPoint Public License 1.0 (текст в каждом каталоге) |

Источники и контрольные суммы архивов Xiph: [provenance.json](docs/licenses/provenance.json).

## Нативная Linux-сборка

Пакеты в Releases собраны с `--minimal`: компоненты из таблицы ниже вшиты в игру
статически, а OpenGL, glibc, X11/Wayland и звуковые библиотеки берутся из системы
(во Flatpak — из рантайма `org.freedesktop.Platform` и его расширений `Compat.i386`, `GL32`).
`titbit-tools` (мастер импорта и Studio) — сборка PyInstaller: внутри Python, Tcl/Tk,
Pillow и системные библиотеки Ubuntu, на которой он собран. Их тексты лицензий лежат
в пакете в `docs/licenses/runtime/` (во Flatpak — `/app/share/doc/titbit/licenses/runtime/`).
Linux-пакет содержит исходники порта; версия коммита и SHA256 файлов указаны в `package.json`.
Заголовки и инструменты Microsoft, игровые данные и оригинальный EXE не включены.

## Windows-пакет

Нормальная сборка больше не использует VC6. Объявления Win32 берутся из открытого
mingw-w64, а Microsoft CRT — из локально установленного современного toolchain.
SDK и заголовки mingw-w64 в репозиторий не копируются. Используемый движком набор
C++/ATL в `portable/` написан для этого проекта; это не исходники Microsoft STL/ATL.

Готовые Windows Setup и Studio также включают Python, Tcl/Tk, Pillow и загрузчик
PyInstaller. Их тексты лицензий находятся в `docs/licenses/runtime/` внутри
portable-пакета. Исходники использованных компонентов движка находятся в этом
репозитории; версия исходников конкретной сборки записана в `package.json` пакета.

## Сборка `--minimal`

| Компонент | Лицензия | Как используется |
|---|---|---|
| FFmpeg 6.1.1 (libavformat, libavcodec, libavutil, libswscale, libswresample) | GNU LGPL 2.1+ | статически: AVI + MP42 + CRAM, собран `tools/build_ffmpeg_min.sh` без GPL-компонентов; исходники игры открыты, поэтому её можно перелинковать с любой версией FFmpeg |
| SDL 2.30.9 | zlib | статически, `tools/build_sdl2_min.sh` |
| Mesa GLU | SGI Free Software License B | статически (`libGLU.a`) |
| GCC libstdc++ / libgcc | GPL с исключением GCC Runtime Library | статически |
| zlib | zlib | статически |
