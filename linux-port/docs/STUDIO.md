# Studio и импорт данных — Linux

Studio — локальный веб-редактор сцен. Сервер читает оригинальный `QUEST.ezd`,
а изменения сохраняет отдельно. Оригинальный архив не перезаписывается.

## Установка

Команды выполняются из `linux-port/`. Для Ubuntu 24.04:

```sh
sudo apt install python3 python3-tk python3-pil
```

Редактору нужен Pillow и браузер с WebGL. Для консольного импорта достаточно
Python 3.10 или новее, без дополнительных пакетов. Tk нужен только для GUI.
На Ubuntu можно использовать системный Python без pip и виртуального окружения.
Для отдельного окружения: `python3 -m venv .venv`, затем
`.venv/bin/pip install -r requirements-studio.txt`.

## Добавление оригинальных данных

```sh
sh tools/import_assets.sh
```

Выбрать установленную копию, каталог CD либо ISO/IMG/CUE/CCD. Монтировать образ
и запускать старый установщик не требуется. Каталог данных по умолчанию:
`~/.local/share/titbit/game/`, либо `$XDG_DATA_HOME/titbit/game/`.

Без графического интерфейса:

```sh
sh tools/import_assets.sh --import /path/to/disc.iso --dest "$HOME/.local/share/titbit/game"
sh tools/import_assets.sh --verify "$HOME/.local/share/titbit/game"
```

Проверяются размеры и SHA256 всех 19 файлов: `QUEST.ezd` и 18 AVI. Совпавшие файлы
используются повторно; отличающиеся файлы назначения не перезаписываются.
Поддерживается набор оригинального диска из `config/assets.json`.
Репаки с изменёнными ресурсами могут не пройти проверку.

## Запуск редактора

```sh
sh tools/start_studio.sh
```

Адрес: `http://127.0.0.1:4318/`. Сервер слушает только localhost.
Остановка — `Ctrl+C` в терминале. Закрытие вкладки сервер не останавливает.

Другой архив, порт или папка проектов:

```sh
sh tools/start_studio.sh --archive /path/to/QUEST.ezd --port 4319 --projects "$HOME/titbit-projects"
```

В WSL браузер можно оставить на стороне Windows:

```sh
sh tools/start_studio.sh --archive /mnt/c/Games/Titbit/QUEST.ezd --no-browser
```

Затем открыть адрес из терминала в браузере Windows. `--port 0` выбирает свободный
порт и выводит фактический адрес. Настройки сети WSL автоматически не изменяются.

## Работа со сценами

- Выбрать локацию; «Камера игры» показывает авторский ракурс, «Вся сцена» — свободный обзор.
- Выбрать объект в сцене или списке; инспектор показывает компоненты, ресурсы и преобразования.
- Перемещение, вращение, масштаб, дублирование и удаление сохраняются в проекте редактора.
- Панель анимации воспроизводит EZA: клип, положение на шкале, скорость и повтор.
- Экспорт: OBJ/MTL с текстурами, GLB и отдельный модифицированный архив.
- Импорт OBJ/GLB/glTF — для статических моделей. Импорт скиннинга и анимаций не реализован.

Проект по умолчанию: `~/.local/share/titbit/studio-projects/default.json`.
При сохранении предыдущая версия остаётся в `default.previous.json`.
Экспорт для игры создаёт отдельный архив. Применять его к игровой копии следует
после резервного копирования. EZA-предпросмотр не запускает игровую VM и сценарии.

## Проверка

```sh
TITBIT_GAME=/path/to/game python3 -m unittest discover -s editor -p 'test_*.py' -v
python3 -m unittest discover -s tools/tests -p 'test_*.py' -v
python3 tools/tests/check_studio_http.py --archive /path/to/QUEST.ezd --report /tmp/studio-http.json
```

GUI проверяется без окон на пользовательском рабочем столе:

```sh
sudo apt install xvfb xauth
TITBIT_XVFB_TEST=1 xvfb-run -a python3 tools/tests/check_setup_gui_xvfb.py \
  --source /path/to/disc.iso --dest /tmp/titbit-gui-test --report /tmp/titbit-gui-test.json
```

Проверено на Ubuntu 24.04 в WSL: 28 тестов редактора, включая 455 EZA,
314 моделей, 161 материал и 70 сцен; HTTP-запуск, чтение всех 70 сцен,
выдача модели/EZA/PNG, запись проекта и отказ в стороннем Origin.
Чистый импорт ISO восстановил 19 файлов (311 181 440 байт) с совпадающими SHA256.
Мастер Tk проверен в Xvfb: выбор ISO, 19 файлов в списке, импорт, SHA256-проверка и закрытие.
Ресурсы берутся из пользовательской копии; в исходники порта они не включены.
