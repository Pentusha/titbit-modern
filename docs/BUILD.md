# Сборка из публичных исходников

Команды выполняются из корня клонированного репозитория. Оригинальный EXE и
исходники Microsoft/OpenNT для сборки не нужны.

## Что нужно

1. **Windows 10/11.**
2. **Microsoft Visual C++ 6.0 + Service Pack 6**, x86.
   Нужны `Bin/CL.EXE`, `LINK.EXE`, `LIB.EXE`, `CVTRES.EXE`, `ML.EXE` (MASM `/coff`),
   `Include`, `ATL/Include`, `Lib` (LIBC.LIB, LIBCP.LIB, штатные Win32/OpenGL/DirectInput/VfW
   импортные библиотеки). Если MASM отсутствует, установите его отдельно.
   В репозиторий эти инструменты не входят — укажите путь переменной
   `TITBIT_VC6=C:\путь\к\VC98`.
3. **Python 3.11**, зависимости из `requirements.txt`.
4. Resource Compiler **rc.exe** из Windows SDK 10/11, в `PATH` либо в
   `Windows Kits/10/bin`. Достаточно установить инструменты SDK, Visual Studio
   для этого не обязательна.

```powershell
$env:TITBIT_VC6 = 'C:\Program Files (x86)\Microsoft Visual Studio\VC98'
py -3.11 -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
```

Путь указывает именно на **VC98**. C-рантайм берётся из **LIBC.LIB**,
C++-рантайм/STL — из **LIBCP.LIB** и штатных заголовков установленного VC6.

## Сборка игры

```powershell
.\.venv\Scripts\python.exe tools/build_modern.py --check
.\.venv\Scripts\python.exe tools/build_modern.py --fresh
```

Результат: `build/modern/TitbitModern.exe`, `TitbitSettings.exe`, `titbit_compat.ini`,
`build-info.json` и журналы. Положите рядом `QUEST.ezd` и 18 роликов `*.avi` из своей
игры. Сохранения автоматически не копируются и не перезаписываются.

## Оригинальное поведение

```powershell
.\.venv\Scripts\python.exe tools/build_original.py --fresh
```

Результат — `build/original/titbit.exe`, без `TITBIT_COMPAT`. Сохраняются исходные
требования игры к диску, видео и Win32 API. Эта сборка использует штатный рантайм
установленного VC6; побайтное совпадение с дисковым EXE не обещается.

## Редактор

```
.\.venv\Scripts\python.exe editor/server.py --archive 'D:\Games\Titbit\QUEST.ezd'
```

Откройте http://127.0.0.1:4318/. Архив читается без записи. Проект сохраняется
в `editor/projects/`, мод скачивается отдельно. Three.js включён в репозиторий,
интернет редактору не нужен.

## Повторная сборка

Без `--fresh` допускается кеш компиляторных объектов с проверкой команд, хешей
объектов и исходных файлов/заголовков. Объекты для адаптации COFF хранятся отдельно
от неизменённых результатов компилятора. LINK всегда выполняется заново;
`/FORCE` не используется.

`config/source-build.json` содержит относительные рецепты, имена символов,
адреса для сопоставления и порядок конструкторов. Машинного кода и данных
оригинального EXE в нём нет. OpenNT, Microsoft reference trees и переписанные
CRT/STL этой сборкой не используются.

Сведения о проверенном запуске и ограничения: [VALIDATION.md](VALIDATION.md).
