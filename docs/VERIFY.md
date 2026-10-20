# Как проверить файлы релиза

## 1. Архив не повреждён и не подменён

Рядом с каждым файлом в [Releases](https://github.com/offx366/titbit-modern/releases) лежит `.sha256`.

```powershell
Get-FileHash .\TitbitModern-0.3.1-windows-x64.zip   # сравните с TitbitModern-0.3.1-windows-x64.zip.sha256
```

```bash
sha256sum -c Titbit-0.3.1-linux-x86.tar.gz.sha256
```

Внутри Windows-пакета `SHA256SUMS.txt` перечисляет хэш каждого файла, а `package.json` —
коммит (`sourceCommit`), из которого собран пакет, и хэши всех четырёх EXE.

## 2. EXE собран именно из этого кода

`TitbitModern.exe` и `TitbitSettings.exe` воспроизводимы побайтно: та же версия исходников и
те же версии Clang, Win32-заголовков, MSVC Build Tools и Windows SDK дают тот же SHA-256.
Время в заголовке EXE берётся из времени коммита,
а не из часов вашего компьютера.

```powershell
git clone https://github.com/offx366/titbit-modern.git
cd titbit-modern
git checkout <sourceCommit из package.json>
$env:TITBIT_LLVM = 'C:\Tools\llvm-mingw-20240619-ucrt-x86_64'
py -3.11 -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
.\.venv\Scripts\python.exe tools/build_modern.py --fresh
.\.venv\Scripts\python.exe tools/verify_release.py C:\Downloads\TitbitModern-0.3.1-windows-x64.zip
```

`verify_release.py` проверяет архив по `SHA256SUMS.txt` и сравнивает оба EXE с вашей сборкой.
Если хэши разошлись, скрипт сравнит хэши инструментов и библиотек с
`package.json` → `reproducible`. Для старых VC6-релизов поддерживаются прежние метаданные;
их нужно проверять сборкой из соответствующего исторического коммита.

Linux-игра (`build/titbit`) тоже собирается побайтно одинаково в любой папке: сравните
`sha256sum build/titbit` двух сборок одного коммита с одинаковыми Clang 18 и пакетами Ubuntu 24.04.

`TitbitSetup.exe` и `TitbitStudio.exe` — упакованный Python (PyInstaller), побайтно они не
воспроизводятся. Их исходники — [`tools/titbit_setup.py`](../tools/titbit_setup.py) и
[`editor/`](../editor/); вместо EXE можно запускать их из исходников (см. [BUILD.md](BUILD.md)).

## 3. Что программы делают в системе

| Программа | Сеть | Что пишет |
|---|---|---|
| TitbitModern.exe | нет | сохранения и `titbit_compat.ini` в своей папке; параметры окна настроек — `HKCU\Software\VZlab\Titbit`, как оригинал |
| TitbitSettings.exe | нет | `titbit_compat.ini` |
| TitbitSetup.exe | нет | копирует `QUEST.ezd` и 18 AVI в выбранную папку; исходник не меняет |
| TitbitStudio.exe | только `127.0.0.1` (локальный сервер для браузера) | проекты в `TitbitStudio-projects` рядом с EXE |

Окно SmartScreen «Windows защитила ваш компьютер» появляется потому, что EXE не подписаны
цифровой подписью. Это не значит, что в них что-то найдено; нажмите «Подробнее» →
«Выполнить в любом случае».
