# Ядерный титбит — Linux

Нативная Linux-сборка восстановленных исходников игры VZlab, изданной «Букой».
Исполняемый файл — **ELF32 для x86**. Wine и Windows EXE при запуске не используются.

## Запуск

Готовый пакет: [Linux Release](https://github.com/offx366/titbit-modern/releases/tag/v0.2.0-linux.1).
Установка системных библиотек: [основная инструкция](https://github.com/offx366/titbit-modern#установка-linux).
Для запуска пакета компилятор и VC98 не нужны. Сначала импортируйте свою копию игры:

```sh
sh tools/import_assets.sh
sh start-game.sh
```

Импортёр имеет графический и консольный режимы, читает образы диска и оригинальные
InstallShield 6 кабинеты (`data1.hdr`, `data*.cab`), проверяет SHA256 всех 19 файлов.
Однофайловые установщики произвольных репаков не поддерживаются.

Для другой папки данных:

```sh
sh start-game.sh --data /path/to/game
```

В каталоге игры нужны `QUEST.EZD` и видеоролики из своей копии игры. Импорт из
установленной игры, ISO, IMG, CUE или CCD описан в [инструкции](docs/STUDIO.md).
Без аргументов скрипт использует `TITBIT_GAME`, локальный `game/` либо
`~/.local/share/titbit/game/` — каталог по умолчанию импортёра.
В WSL после сборки можно использовать `Start Linux.cmd` (Ubuntu-24.04 / WSLg).

## Сборка

```sh
export TITBIT_VC6=/path/to/VC98
python3 tools/build_linux.py --test-runtime
```

Зависимости и команды для Ubuntu: [BUILD-LINUX.md](docs/BUILD-LINUX.md).
Результат: `build/titbit`; журнал сборки: `build/linux-build.json`.

32-битная компоновка объектов и виртуальных таблиц Microsoft сохранена:
Clang формирует LLVM IR, адаптер переводит обработку исключений в DWARF,
затем LLVM выпускает Linux ELF. Заголовки установленного VC98 нужны при
сборке; Windows CRT-библиотеки в ELF не линкуются.

SDL2 обслуживает окно, ввод и PCM-звук; OpenGL — исходный рендер;
FFmpeg — AVI. Оригинальные исходники Windows находятся в отдельном дереве.
Порядок инициализации описан в [INITIALIZERS.md](docs/INITIALIZERS.md).

## Редактор

```sh
sh tools/start_studio.sh --archive /path/to/game/QUEST.EZD
```

Локальный веб-редактор сцен: выбор объектов, преобразования, модели,
материалы и предпросмотр анимации. [Запуск и импорт данных](docs/STUDIO.md).

## Диагностика

```sh
python3 tools/check_initializers.py
python3 tools/check_cpp_runtime.py --vc98 "$TITBIT_VC6"
OPT_LEVEL=2 sh probes/run_verified.sh
python3 tools/headless_probe.py --data /path/to/game --frames 50
```

Последняя команда создаёт собственный Xvfb и сохраняет реальные кадры
OpenGL, статистику и журнал. Она не открывает игровое окно на рабочем столе.

Целевая среда проверки — Ubuntu 24.04 / WSL2 на x86-64 с 32-битными
библиотеками. Открытие внешних документов через `ShellExecute` пока
возвращает ошибку; DirectSound заменён работающим SDL/waveOut-путём.

## Проверено в игре

Нативная сборка прошла 20 000 кадров под GDB без падений: ролики, главное меню,
начальная улица, ходьба и анимация, сохранение в слот, загрузка и диалог охранника.
Прогон выполнялся в отдельном Xvfb; захватывался настоящий OpenGL framebuffer.
Игровой Audiere/Vorbis выдаёт проверенный PCM 44,1 кГц, стерео, 16 бит;
вывод через WSLg PulseAudio проверен отдельно на буфере тишины.
Это проверка начального участка, а не подтверждение полного прохождения.
Сценарии повторения: [probes/README.md](probes/README.md).
