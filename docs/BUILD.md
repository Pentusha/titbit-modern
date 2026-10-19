# Сборка из исходников

Команды выполняются из корня репозитория в PowerShell.
**Чтобы просто играть, используйте portable-пакет:** [первый запуск](GETTING_STARTED.md).
Приведённые ниже зависимости нужны разработчику, а не игроку.

## Инструменты

- **Windows 10/11 x64** и **Python 3.11 x64**.
- **Visual C++ 6.0 SP6**, x86: `Bin/CL.EXE`, `LINK.EXE`, `LIB.EXE`,
  `CVTRES.EXE`, `ML.EXE` (MASM с `/coff`), `Include`, `ATL/Include` и `Lib`.
  Нужны `LIBC.LIB`, `LIBCP.LIB` и штатные Win32/OpenGL/DirectInput/VfW библиотеки.
  MASM при необходимости устанавливается отдельно.
- **rc.exe** из Windows SDK 10/11: в `PATH` либо стандартном каталоге
  `Windows Kits/10/bin`. Устанавливать современную Visual Studio целиком не нужно.

Инструменты Microsoft и их исходники в проект не включены.
`TITBIT_VC6` должен указывать именно на установленный **VC98**:
C-рантайм берётся из `LIBC.LIB`, C++-рантайм и STL — из `LIBCP.LIB`
и штатных заголовков. Оригинальный `titbit.exe` для сборки не нужен.

## Подготовка

```powershell
git clone https://github.com/offx366/titbit-modern.git
cd titbit-modern
$env:TITBIT_VC6 = 'C:\Program Files (x86)\Microsoft Visual Studio\VC98'
py -3.11 -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
```

## Игра

```powershell
.\.venv\Scripts\python.exe tools/build_modern.py --check
.\.venv\Scripts\python.exe tools/build_modern.py --fresh
```

Результат в `build/modern/`:

- `TitbitModern.exe` — игра с `TITBIT_COMPAT`.
- `TitbitSettings.exe` — настройки экрана.
- `titbit_compat.ini`, `build-info.json` и журналы сборки.

Игра остаётся 32-битным приложением. Portable-оболочки Setup и Studio
собираются для 64-битной Windows.

### Добавить игровые данные

Импортёр использует только стандартную библиотеку Python. При запуске без
аргументов открывается окно выбора:

```powershell
.\.venv\Scripts\python.exe tools/titbit_setup.py
```

Те же действия из командной строки:

```powershell
# Посмотреть содержимое установленной игры или образа:
.\.venv\Scripts\python.exe tools/titbit_setup.py --inspect 'D:\Games\Titbit'
.\.venv\Scripts\python.exe tools/titbit_setup.py --inspect 'D:\Images\Titbit.iso'

# Импортировать в папку сборки:
.\.venv\Scripts\python.exe tools/titbit_setup.py --import 'D:\Images\Titbit.iso' --dest build/modern

# Проверить готовую папку:
.\.venv\Scripts\python.exe tools/titbit_setup.py --verify build/modern
```

Можно указать CD, ISO или набор IMG/CUE/CCD. Исходник не меняется.
Файлы с отличающимся содержимым и сохранения не перезаписываются.
Поддерживаемые источники: [GETTING_STARTED.md](GETTING_STARTED.md).

## Studio из исходников

Редактор можно использовать без сборки игрового EXE:

```powershell
.\.venv\Scripts\python.exe editor/server.py --archive 'D:\Games\Titbit\QUEST.ezd'
```

Откройте **http://127.0.0.1:4318/**. Другой порт: `--port 4319`.
Проекты сохраняются в `editor/projects/`. Three.js находится в репозитории,
интернет редактору не нужен.

Для локальной оболочки с автоматическим открытием браузера:

```powershell
.\.venv\Scripts\python.exe editor/studio_launcher.py
```

Оболочка спросит путь к архиву, если не найдёт его автоматически.
Упакованная Studio хранит проекты рядом с EXE в `TitbitStudio-projects/`.

## Portable-пакет для игроков

```powershell
.\.venv\Scripts\python.exe -m pip install -r requirements-build.txt
.\.venv\Scripts\python.exe tools/package_portable.py --game-build build/modern
```

Упаковщик собирает **TitbitSetup.exe** и **TitbitStudio.exe** через PyInstaller,
добавляет современную игру, настройки и инструкции. Результат:
`dist/TitbitModern-<version>-windows-x64.zip`.

**QUEST.ezd, AVI, сохранения и образы диска в ZIP не добавляются**, даже если
они есть в папке сборки. Игрок распаковывает ZIP и добавляет свои данные
через TitbitSetup. Python и зависимости Studio уже упакованы в программы.

## Вариант без слоя совместимости

```powershell
.\.venv\Scripts\python.exe tools/build_original.py --fresh
```

Результат — `build/original/titbit.exe`, без `TITBIT_COMPAT`.
Сохраняются исходные требования к диску, видео и Win32 API; это вариант для
исследования оригинала. Штатный рантайм берётся из установленного VC6,
побайтное совпадение с дисковым EXE не обещается.

## Проверки

Проверки чтения диска, импорта и границ portable-пакета:

```powershell
.\.venv\Scripts\python.exe -m pip install -r requirements-build.txt
.\.venv\Scripts\python.exe -m unittest discover -s tools/tests -p 'test_*.py'
```

Два дополнительных теста оригинальных данных без переменных `TITBIT_TEST_*`
пропускаются; обычные проверки используют небольшие синтетические файлы.

Для тестов на полном архиве укажите папку со своим `QUEST.ezd`:

```powershell
$env:TITBIT_GAME = 'D:\Games\Titbit'
.\.venv\Scripts\python.exe -m unittest discover -s editor -p 'test_*.py'
```

Без архива проверки оригинальных ресурсов пропускаются. Проверенные запуски,
контрольные суммы и ограничения: [VALIDATION.md](VALIDATION.md).

Без `--fresh` сборка использует кеш с проверкой команд, объектов, исходников
и заголовков; LINK запускается заново. `/FORCE` не используется.
`config/source-build.json` содержит рецепты сборки и символы,
без машинного кода или данных исходного EXE.
