# Сборка из исходников

Команды выполняются из корня репозитория в PowerShell.
**Чтобы просто играть, используйте portable-пакет:** [первый запуск](GETTING_STARTED.md).
Приведённые ниже зависимости нужны разработчику, а не игроку.

## Инструменты

- **Windows 10/11 x64** и **Python 3.11 x64**.
- **LLVM-MinGW x86-64**: распакуйте официальный [архив](https://github.com/mstorsjo/llvm-mingw/releases/tag/20240619)
  `llvm-mingw-20240619-ucrt-x86_64.zip` (Clang 18.1.8). Он содержит Clang и открытые Win32-заголовки.
- **Visual Studio 2022 Build Tools**: компонент «Разработка классических приложений на C++»,
  инструменты MSVC x86/x64 и Windows SDK. Подходит и установленная Visual Studio Community.
  Сборочный скрипт находит инструменты автоматически; IDE запускать не требуется.
- Ресурсы компилирует `tools/rc_compile.py`; системный `rc.exe` не требуется.

**VC6 не нужен.** Строки, контейнеры и используемый набор ATL предоставляет авторский
слой `portable/`. C runtime и обработка исключений линкуются из современных
`libcmt`, `libvcruntime` и `libucrt`. Инструменты Microsoft в репозиторий не включены;
оригинальный `titbit.exe` для сборки не нужен.

## Подготовка

```powershell
git clone https://github.com/offx366/titbit-modern.git
cd titbit-modern
$env:TITBIT_LLVM = 'C:\Tools\llvm-mingw-20240619-ucrt-x86_64'
py -3.11 -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
```

## Игра

```powershell
.\.venv\Scripts\python.exe tools/build_modern.py --check
.\.venv\Scripts\python.exe tools/build_modern.py --fresh --test-runtime
```

Результат в `build/modern-portable/` (прежний `build/modern/` сохраняется):

- `TitbitModern.exe` — игра с `TITBIT_COMPAT`.
- `TitbitSettings.exe` — настройки экрана.
- `titbit_compat.ini`, `build-info.json` и журналы сборки.

Игра остаётся 32-битным приложением. Portable-оболочки Setup и Studio
собираются для 64-битной Windows.

`--test-runtime` запускает консольные проверки строк, контейнеров, исключений,
Win32 ABI и файлового ввода-вывода. Игровые данные для них не нужны.

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
.\.venv\Scripts\python.exe tools/titbit_setup.py --import 'D:\Images\Titbit.iso' --dest build/modern-portable

# Проверить готовую папку:
.\.venv\Scripts\python.exe tools/titbit_setup.py --verify build/modern-portable
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
.\.venv\Scripts\python.exe tools/package_portable.py --game-build build/modern-portable
```

Упаковщик собирает **TitbitSetup.exe** и **TitbitStudio.exe** через PyInstaller,
добавляет современную игру, настройки и инструкции. Результат:
`dist/TitbitModern-<version>-windows-x64.zip`.

**QUEST.ezd, AVI, сохранения и образы диска в ZIP не добавляются**, даже если
они есть в папке сборки. Игрок распаковывает ZIP и добавляет свои данные
через TitbitSetup. Python и зависимости Studio уже упакованы в программы.

## Исходная логика без слоя совместимости

Те же современные инструменты, без VC6:

```powershell
.\.venv\Scripts\python.exe tools/build_original.py --fresh --test-runtime
```

Результат — `build/original-portable/titbit.exe`, без `TITBIT_COMPAT`.
Компоновка проверяется отдельно; для игры на современных системах используйте
совместимый `TitbitModern.exe`. Здесь сохраняются исходные требования к диску,
видеокодекам и Win32 API; побайтное совпадение с дисковым EXE не обещается.

## Историческая сборка VC6 (необязательно)

Для исследования прежней сборки сохранён отдельный путь. Только он требует
установленного VC6 SP6 с MASM, `Include`, `ATL/Include`, `LIBC.LIB` и `LIBCP.LIB`:

```powershell
$env:TITBIT_VC6 = 'C:\Program Files (x86)\Microsoft Visual Studio\VC98'
.\.venv\Scripts\python.exe tools/build_original.py --legacy-vc6 --fresh
```

Результат — `build/original/titbit.exe`. Штатный рантайм этого исторического
варианта берётся из установленного VC6. Обычные сборки эту установку не читают.

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

Без архива проверки оригинальных ресурсов пропускаются. Проверенные запуски
без VC6, контрольные суммы и ограничения: [VALIDATION-NO-VC6.md](VALIDATION-NO-VC6.md).
Исторический экспорт VC6: [VALIDATION.md](VALIDATION.md).

Без `--fresh` сборка использует кеш с проверкой команд, объектов, исходников
и заголовков; LINK запускается заново. `/FORCE` не используется.
`config/source-build.json` содержит рецепты сборки и символы,
без машинного кода или данных исходного EXE.

В заголовок EXE записывается время коммита (или `SOURCE_DATE_EPOCH`). Для воспроизведения
бинарника нужны одинаковые исходники, версия Clang, открытые Win32-заголовки и библиотеки
MSVC/Windows SDK; их хэши записываются в `build-info.json`.
Номер версии для всех EXE и пакетов — в файле `VERSION`.

## Linux

Нативная сборка выполняется отдельно из `linux-port/` средствами Clang/LLVM.
Зависимости и проверочные команды (VC6 и Windows SDK не нужны):
[linux-port/docs/BUILD-LINUX.md](../linux-port/docs/BUILD-LINUX.md).
