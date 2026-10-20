# Сборка Linux

Целевая платформа — x86 Linux с поддержкой ELF32. Проверочная среда — Ubuntu 24.04
в WSL2. Код сохраняет четырёхбайтовые указатели и Microsoft C++ ABI оригинала.
Wine и оригинальный EXE при сборке не используются.

## Зависимости

Для Ubuntu 24.04:

```sh
sudo dpkg --add-architecture i386
sudo apt update
sudo apt install python3 clang-18 llvm-18 lld-18 gcc-multilib g++-multilib pkg-config mingw-w64-i686-dev \
  libsdl2-dev:i386 libgl1-mesa-dev:i386 libglu1-mesa-dev:i386 \
  libavformat-dev:i386 libavcodec-dev:i386 libavutil-dev:i386 \
  libswscale-dev:i386 libswresample-dev:i386 zlib1g-dev:i386
```

**VC6 и Windows SDK не нужны.** Системные Win32-объявления предоставляет открытый
пакет `mingw-w64-i686-dev`. Используемые игрой строки, контейнеры, потоки и небольшой
набор ATL реализованы в общем каталоге `portable/` проекта. В Git-репозитории он
находится рядом с `linux-port/`; в отдельном Linux-экспорте — внутри его корня.
Clang сохраняет исходную 32-битную компоновку классов, а C runtime обслуживается
нативным Linux-слоем. Заголовки и библиотеки Microsoft не читаются.

## Команды

Выполнять из `linux-port/`:

```sh
python3 tools/build_linux.py --check
python3 tools/build_linux.py --jobs 4 --test-runtime
```

Результат — `build/titbit`. Логи этапов: `build/logs/`; итоговый отчёт и SHA256:
`build/linux-build.json`. Одновременные запуски сборочного скрипта блокируются.

Сборка проходит через Microsoft-ABI LLVM IR, преобразование обработки исключений
в DWARF и нативный backend ELF32. Окно, ввод, звук и видео обслуживают SDL2,
OpenGL и FFmpeg. Исходники оригинального движка и платформенный слой разделены.

## Сборка с минимумом зависимостей (рекомендуется для пакетов)

```sh
python3 tools/build_linux.py --minimal --jobs 4 --test-runtime
```

Один раз собирает в `~/.cache/titbit/` статический FFmpeg только с тем, что нужно игре
(контейнер AVI, кодеки MP42 и CRAM) и статический SDL2, который сам подгружает X11,
Wayland, PulseAudio, ALSA и PipeWire во время работы. libstdc++, libgcc, zlib и GLU
вшиваются в исполняемый файл. Итог — ELF зависит только от `libGL.so.1` и glibc:

```sh
# для запуска (SDL подгружает X11 и звук сам, когда они есть)
sudo apt install libgl1:i386 libgl1-mesa-dri:i386 libx11-6:i386 libxext6:i386 libxcursor1:i386 libxi6:i386 libxrandr2:i386 libpulse0:i386
```

При первой сборке нужны `curl` и сеть: скачиваются официальные архивы FFmpeg 6.1.1 и
SDL 2.30.9, каждый проверяется по зашитому в скрипт SHA-256.
Отдельно: `tools/build_ffmpeg_min.sh`, `tools/build_sdl2_min.sh`, флаги `--ffmpeg` / `--sdl2`.

Готовый ELF требует glibc 2.38+, если собран на Ubuntu 24.04: порог задаёт glibc
системы сборки. Для более старых систем есть Flatpak — у него своя glibc.

## Пакеты для игроков

```sh
sudo apt install python3-venv python3-tk flatpak-builder
sh tools/build_tools.sh                     # build/titbit-tools
python3 tools/package_linux.py --version dev --commit "$(git rev-parse --short HEAD)" \
  --out dist/Titbit-dev-linux-x86.tar.gz
sh flatpak/build_flatpak.sh dev             # dist/Titbit-dev-x86_64.flatpak
```

`titbit-tools` — мастер импорта и Studio одним x86-64 файлом (PyInstaller): игроку не нужен
Python. Лицензии всего, что в него упаковано, собираются в `build/tools-licenses/` и кладутся
в пакеты (`docs/licenses/runtime/`). `start-game.sh` при отсутствии данных сам открывает мастер.

Flatpak (`io.github.offx366.Titbit`, рантайм `org.freedesktop.Platform//25.08`) запускает ELF32
через расширения `Compat.i386` и `GL32`. Для сборки один раз:

```sh
flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak install --user flathub org.freedesktop.Platform//25.08 org.freedesktop.Sdk//25.08 \
  org.freedesktop.Platform.Compat.i386//25.08 org.freedesktop.Platform.GL32.default//25.08
```

Данные игры Flatpak хранит в `~/.var/app/io.github.offx366.Titbit/data/titbit/game`.
Доступ к файлам — только чтение (`home`, `/media`, `/run/media`, `/mnt`) для выбора образа;
сеть открыта только ради Studio: это локальный веб-сервер, который открывается в браузере.

## Данные и запуск

```sh
python3 tools/titbit_setup.py --import /path/to/disc.iso --dest "$HOME/.local/share/titbit/game"
./build/titbit --data "$HOME/.local/share/titbit/game"
```

Импорт поддерживает установленную игру, каталог диска и ISO/IMG/CUE/CCD. Все 19
файлов проверяются по SHA256. Оригинальные данные в репозиторий не включены.
В WSL для обычного запуска нужен работающий WSLg либо другой X11/Wayland дисплей.

## Проверки

```sh
python3 tools/check_cpp_runtime.py
sh probes/run_verified.sh
# Дополнительная проверка startup; нужен gdb.
python3 tools/check_initializers.py
```

Первый тест выполняет нативный ELF32, проверяя строки, контейнеры, типизированные
исключения, RAII, C-locale, FILE32 и макросы `getc`, потоки, EOF и 64-битные
позиции файлов. Второй проверяет расположение объектов, виртуальные вызовы,
`thiscall`/`stdcall`, RTTI и перенос исключений.
`check_initializers.py` проверяет восемь настоящих глобальных объектов при входе
в игровой цикл. Он работает с SDL dummy, без окна и игровых данных.
Схема инициализации: [INITIALIZERS.md](INITIALIZERS.md).

Для прогона игры без видимого окна:

```sh
sudo apt install xvfb xauth xdotool gdb python3-pil
python3 tools/headless_probe.py --data /path/to/test-game --frames 20000 \
  --timeout 360 --out build/headless-gameplay --actions probes/gameplay-actions.json --gdb
```

Сценарий записывает слот 1: используйте отдельную тестовую копию данных.
Проверены ролики, меню, начальная улица, ходьба, сохранение, загрузка и диалог;
20 000 кадров, штатный выход, без сигналов GDB. В отчёте сохраняется SHA256 ELF.
Полное прохождение игры пока не проверялось.

[Studio и импортёр данных](STUDIO.md)
