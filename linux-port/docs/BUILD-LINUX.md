# Сборка Linux

Целевая платформа — x86 Linux с поддержкой ELF32. Проверочная среда — Ubuntu 24.04
в WSL2. Код сохраняет четырёхбайтовые указатели и Microsoft C++ ABI оригинала.
Wine и оригинальный EXE при сборке не используются.

## Зависимости

Для Ubuntu 24.04:

```sh
sudo dpkg --add-architecture i386
sudo apt update
sudo apt install python3 clang-18 llvm-18 lld-18 gcc-multilib g++-multilib pkg-config \
  libsdl2-dev:i386 libgl1-mesa-dev:i386 libglu1-mesa-dev:i386 \
  libavformat-dev:i386 libavcodec-dev:i386 libavutil-dev:i386 \
  libswscale-dev:i386 libswresample-dev:i386 zlib1g-dev:i386
```

Нужны заголовки из пользовательской установки Visual C++ 6 (`VC98/Include` и
`VC98/ATL/Include`). SDK не входит в репозиторий. Его библиотеки и компилятор
Windows не запускаются: Clang читает заголовки через генерируемый VFS overlay.
Синтаксические адаптации старых заголовков находятся только в `build/`,
установленный SDK не изменяется.

## Команды

Выполнять из `linux-port/`:

```sh
export TITBIT_VC6=/path/to/VC98
python3 tools/build_linux.py --check
python3 tools/build_linux.py --jobs 4 --test-runtime
```

В WSL путь к Windows SDK имеет вид `/mnt/c/.../VC98`.
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
sudo apt install libc6:i386 libgl1:i386 libpulse0:i386   # для запуска
```

Нужны git и сеть при первой сборке (исходники FFmpeg 6.1.1 и SDL 2.30.9).
Отдельно: `tools/build_ffmpeg_min.sh`, `tools/build_sdl2_min.sh`, флаги `--ffmpeg` / `--sdl2`.

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
python3 tools/check_cpp_runtime.py --vc98 "$TITBIT_VC6"
sh probes/run_verified.sh
# Дополнительная проверка startup; нужен gdb.
python3 tools/check_initializers.py
```

Первый тест выполняет нативный ELF32, проверяя COW-строки, типизированные
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
  --timeout 240 --out build/headless-gameplay --actions probes/gameplay-actions.json --gdb
```

Сценарий записывает слот 1: используйте отдельную тестовую копию данных.
Проверены ролики, меню, начальная улица, ходьба, сохранение, загрузка и диалог;
20 000 кадров, штатный выход, без сигналов GDB. В отчёте сохраняется SHA256 ELF.
Полное прохождение игры пока не проверялось.

[Studio и импортёр данных](STUDIO.md)
