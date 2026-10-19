<h1 align="center">Ядерный Титбит</h1>

<p align="center"><b>Восстановленные исходники игры. Windows 10/11 и нативный Linux-порт.</b></p>
<p align="center">VZlab · Бука · 2003</p>

![Первая улица — сборка из этих исходников](docs/images/game-street.png)

Код игры восстановлен по оригинальному `titbit.exe`. Сборка Windows работает на
Windows 10/11; Linux-порт собирается в нативный ELF32 и использует SDL2, OpenGL и FFmpeg.
Для современных Windows добавлены полный экран
(**Alt+Enter / F11**), воспроизведение видео без старых кодеков и запуск без диска.

> [!IMPORTANT]
> **Для игры нужна ваша копия «Ядерного Титбита»** — оригинальный CD, образ диска
> (ISO / IMG / CUE / CCD) или установленная игра. Уровни, модели, музыка, озвучка и видео
> берутся оттуда: в репозиторий и готовые сборки они **не входят**.

<details>
<summary>Где взять данные игры?</summary>

Права на игру, музыку и другие оригинальные материалы принадлежат их правообладателям, поэтому мы их не распространяем. ISO или установщик игры ищите на всем известных сайтах ;)

</details>

## Установка Windows

1. Скачайте `TitbitModern-*-windows-x64.zip` из [Releases](https://github.com/offx366/titbit-modern/releases)
   и распакуйте в любую папку, например `C:\Games\TitbitModern`.
2. Запустите **TitbitSetup.exe** и укажите, откуда взять данные игры:
   - **диск** — вставьте CD и выберите привод;
   - **образ** — выберите файл ISO, IMG, CUE или CCD (монтировать не нужно);
   - **установленная игра** — выберите её папку.

   Нажмите **«Добавить файлы игры»**. Программа сама найдёт и скопирует `QUEST.ezd`
   и 18 видеороликов и проверит их. Старый установщик запускать не нужно.
3. Запустите **TitbitModern.exe** — игра. **TitbitSettings.exe** — размер окна,
   полный экран, пропорции 4:3 или растяжение.

После импорта диск больше не нужен. Python, компилятор и интернет не требуются.
Если что-то не получилось: [docs/GETTING_STARTED.md](docs/GETTING_STARTED.md).

## Установка Linux

Нативная сборка (ELF32), без Wine. Python и компилятор не нужны. Два варианта:

**Flatpak — любой дистрибутив.** Скачайте `Titbit-*-x86_64.flatpak` из
[Releases](https://github.com/offx366/titbit-modern/releases):

```bash
flatpak remote-add --user --if-not-exists flathub https://dl.flathub.org/repo/flathub.flatpakrepo
flatpak install --user flathub org.freedesktop.Platform.Compat.i386//25.08 org.freedesktop.Platform.GL32.default//25.08
flatpak install --user Titbit-*-x86_64.flatpak
flatpak run io.github.offx366.Titbit
```

Вторая строка — 32-битные библиотеки с Flathub (у кого стоит Steam из Flathub, они уже есть).
Игра появится и в меню приложений.

**Архив — Ubuntu 24.04+, Debian 13+, Fedora 39+ и другие системы с glibc 2.38+.**

1. Установите 32-битные OpenGL, X11 и звук (Ubuntu / Debian):

   ```bash
   sudo dpkg --add-architecture i386 && sudo apt update
   sudo apt install libgl1:i386 libgl1-mesa-dri:i386 libx11-6:i386 libxext6:i386 libxcursor1:i386 libxi6:i386 libxrandr2:i386 libpulse0:i386
   ```

   Если у вас стоит Steam, они, скорее всего, уже есть.

2. Скачайте `Titbit-*-linux-x86.tar.gz` из [Releases](https://github.com/offx366/titbit-modern/releases)
   и запустите:

   ```bash
   tar -xzf Titbit-*-linux-x86.tar.gz && cd Titbit-*-linux-x86
   sh start-game.sh
   ```

При первом запуске откроется мастер импорта: укажите ISO / IMG / CUE / CCD, папку
установщика (`data1.hdr`, `data*.cab`) или установленную игру — `QUEST.ezd` и 18 роликов
скопируются и проверятся. Дальше игра запускается сразу. Studio: `sh tools/start_studio.sh`
(во Flatpak — `flatpak run io.github.offx366.Titbit --studio`).

![Начальная улица — нативная Linux-сборка](docs/images/game-linux.png)

Сборка из исходников: [linux-port/docs/BUILD-LINUX.md](linux-port/docs/BUILD-LINUX.md)
(`python3 tools/build_linux.py --minimal`). Проверки: [VALIDATION-LINUX.md](docs/VALIDATION-LINUX.md).

## Titbit Studio

Редактор сцен в комплекте. В Windows запустите **TitbitStudio.exe**, в Linux —
`sh tools/start_studio.sh`. В браузере откроются все 70 локаций игры:
свободная камера, выбор и перемещение объектов, анимации, экспорт в Blender (OBJ / GLB)
и обратно, сборка своего мода. Файлы игры при этом не меняются.

![Titbit Studio](docs/images/studio.png)

Управление: [editor/README.md](editor/README.md).

## Насколько это оригинал

Каждая из **4 955** функций игры переписана на C/C++, собрана тем же компилятором, что и
в 2003 году (Visual C++ 6.0), и сверена с оригинальным EXE.

| | Байт кода | |
|---|---|---|
| Совпадает с оригиналом **байт в байт** | 633 954 | **80,6 %** |
| Поведение совпадает, байты отличаются | 152 527 | **19,4 %** |
| **Итого соответствует оригиналу** | **786 481** | **100 %** |

```
байт в байт   ████████████████████████████████████████░░░░░░░░░░   80,6 %
поведение     ██████████████████████████████████████████████████  100,0 %
```

```mermaid
flowchart LR
    A[titbit.exe] --> B[функция → C/C++]
    B --> C[Visual C++ 6.0]
    C --> D{байты совпали?}
    D -- да, 80,6 % --> F[byte-exact]
    D -- нет, 19,4 % --> E[сверка поведения<br>в эмуляторе x86] --> G[поведение совпало]
    F --> H[EXE из исходников]
    G --> H
```

## Сборка из исходников

Для обеих платформ нужен **Visual C++ 6.0 SP6** — в Windows им собирается игра, в Linux берутся
только его заголовки (`VC98/Include`, `VC98/ATL/Include`). Инструменты Microsoft и данные игры
в репозиторий не входят.

<details>
<summary><b>Windows</b></summary>

Нужны только Python 3.11 и VC6 SP6 с MASM (`ML.EXE`). Windows SDK и современная Visual Studio не нужны.

```powershell
git clone https://github.com/offx366/titbit-modern.git
cd titbit-modern
$env:TITBIT_VC6 = 'C:\Program Files (x86)\Microsoft Visual Studio\VC98'
py -3.11 -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
.\.venv\Scripts\python.exe tools/build_modern.py --check   # проверка инструментов
.\.venv\Scripts\python.exe tools/build_modern.py --fresh
```

Результат — `build/modern/TitbitModern.exe` и `TitbitSettings.exe`. Данные игры добавьте
`python tools/titbit_setup.py`. Portable-пакет как в Releases (с TitbitSetup и TitbitStudio):

```powershell
.\.venv\Scripts\python.exe -m pip install -r requirements-build.txt
.\.venv\Scripts\python.exe tools/package_portable.py --game-build build/modern
```

Подробно: [docs/BUILD.md](docs/BUILD.md).

</details>

<details>
<summary><b>Linux</b></summary>

Ubuntu 24.04 x86-64 (или WSL2). Сборка: Clang/LLVM 18 → ELF32, без Wine.

```bash
sudo dpkg --add-architecture i386 && sudo apt update
sudo apt install git curl python3 clang-18 llvm-18 lld-18 gcc-multilib g++-multilib pkg-config libsdl2-dev:i386 libgl1-mesa-dev:i386 libglu1-mesa-dev:i386 zlib1g-dev:i386

git clone https://github.com/offx366/titbit-modern.git
cd titbit-modern/linux-port
export TITBIT_VC6="/path/to/VC98"   # в WSL: "/mnt/c/Program Files (x86)/Microsoft Visual Studio/VC98"
python3 tools/build_linux.py --check
python3 tools/build_linux.py --minimal --jobs "$(nproc)" --test-runtime
```

Результат — `build/titbit`. `--minimal` при первой сборке скачивает официальные архивы
FFmpeg и SDL2 (с проверкой SHA-256), собирает их урезанными в `~/.cache/titbit/` и вшивает
в игру: готовому файлу нужны только 32-битные glibc и OpenGL. Запуск:

```bash
python3 tools/titbit_setup.py --import /path/to/disc.iso --dest ~/.local/share/titbit/game
./build/titbit --data ~/.local/share/titbit/game
```

Пакеты как в Releases (нужны ещё `python3-venv python3-tk`, для Flatpak — `flatpak-builder`):

```bash
sh tools/build_tools.sh            # build/titbit-tools: импорт + Studio одним файлом
python3 tools/package_linux.py --version dev --commit "$(git rev-parse --short HEAD)" --out dist/Titbit-dev-linux-x86.tar.gz
sh flatpak/build_flatpak.sh dev    # dist/Titbit-dev-x86_64.flatpak
```

Подробно: [linux-port/docs/BUILD-LINUX.md](linux-port/docs/BUILD-LINUX.md).

</details>

---

Проверено: сборка из этого репозитория, импорт с образа диска, игра от меню до первой улицы.
Полное прохождение и разные видеокарты ещё не проверены — пишите о проблемах в Issues.

Игра и её материалы — © VZlab / Бука. Данные игры и инструменты Microsoft не распространяются.
[Сторонние библиотеки](THIRD_PARTY_NOTICES.md) · [Данные игры](docs/ASSETS.md)
