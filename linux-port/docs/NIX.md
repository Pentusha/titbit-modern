# Nix / NixOS / macOS

Flake находится в корне Git-репозитория. Нужен Nix с включёнными `nix-command`
и `flakes`. nixpkgs зафиксирован на `nixos-26.05`.

## Платформы

| Система | `nix run` / пакет `titbit` | `titbit-tools` |
| --- | --- | --- |
| `x86_64-linux` | Нативный Linux-порт, 32-битный x86 ELF | Нативные Python-инструменты |
| `aarch64-linux` | Тот же ELF через `qemu-i386`, программный OpenGL | Нативные Python-инструменты |
| `aarch64-darwin` | x86 Linux VM в QEMU, программный OpenGL | Нативные Python-инструменты |

Это **не нативный ARM-порт движка**. Его структуры данных и Microsoft C++ ABI
требуют четырёхбайтовых указателей и x86. На Apple Silicon эмулируется также
Linux; Rosetta для запуска этого ELF не используется. Производительность
эмуляции и программного OpenGL будет ниже нативного запуска.

Варианты с эмуляцией пока экспериментальные. Для macOS проверено вычисление
derivation; сборка и запуск на реальном Mac требуют отдельной проверки.

На ARM и macOS для сборки игры нужен builder `x86_64-linux`, умеющий запускать
32-битные Linux-программы. Например, удалённая x86-64 NixOS-машина. Другой вариант
на ARM Linux — `boot.binfmt.emulatedSystems = [ "x86_64-linux" "i686-linux" ];`;
сборка LLVM/движка под эмуляцией может быть медленной. Нативные `titbit-tools`
такого builder не требуют. Обычный ARM Linux builder на macOS без x86-эмуляции
для движка недостаточен.

## Сборка и запуск

Из корня репозитория:

```sh
nix build
./result/bin/titbit
```

При первом запуске без аргументов открывается мастер импорта. Нужна своя копия
игры: установленный каталог, диск либо образ ISO / IMG / CUE / CCD. Данные
импортируются в `${XDG_DATA_HOME:-$HOME/.local/share}/titbit/game` (также на macOS).
Каталог доступен для записи: движок хранит сохранения рядом с данными. Игровые
файлы в Nix store не добавляются.

Импорт через командную строку и запуск:

```sh
nix run .#titbit-setup -- --import /path/to/disc.iso \
  --dest "${XDG_DATA_HOME:-$HOME/.local/share}/titbit/game"
nix run
```

Другой каталог можно выбрать переменной `TITBIT_GAME` при запуске без аргументов
либо явно передать:

```sh
nix run . -- --data /path/to/writable/game
nix run .#titbit-studio
```

При запуске мастера с нестандартным каталогом укажите его в окне назначения
или через `--dest`. После импорта повторно запустите игру.

Пакеты и приложения:

| Имя | Содержимое |
| --- | --- |
| `packages.<system>.default`, `titbit` | Запуск игры способом из таблицы платформ, импортёр и Studio |
| `packages.<system>.titbit-tools` | Только нативные импортёр и Studio |
| `packages.<linux-system>.titbit-emulated` | Явный запуск через `qemu-i386`, в том числе на x86-64 для проверки |
| `packages.<system>.titbit-vm` | Явный запуск в x86 Linux VM на любой объявленной системе |
| `apps.<system>.titbit-setup`, `titbit-studio` | Запуск нативных инструментов через `nix run` |

После установки доступны `titbit`, `titbit-setup`, `titbit-studio`. На Linux
движок также появляется в меню приложений.

## Автоматическая подготовка из своего magnet

Для установки без мастера импорта используйте функцию `lib.withMagnet`:

```nix
{ pkgs, inputs, ... }:
{
  environment.systemPackages = [
    (inputs.titbit.lib.withMagnet {
      system = pkgs.stdenv.hostPlatform.system;
      magnet = "magnet:?xt=urn:btih:YOUR_INFO_HASH";
      hash = "sha256-...";
    })
  ];
}
```

Magnet задаёт пользователь; конкретная раздача в flake не зашита. Nix скачивает
её через `fetchtorrent`, затем импортёр извлекает данные и проверяет все 19
файлов версии 1.00 по SHA256. Если в каталоге раздачи один подходящий ISO / IMG /
CUE / CCD, он выбирается автоматически. Установленная папка игры тоже подходит.
Архивы RAR / 7z и другие форматы диска нужно предварительно преобразовать в
источнике Nix; установщики и EXE не запускаются.

`hash` — рекурсивный SHA256 результата `fetchtorrent`, **не** `btih` из magnet.
Для первого получения хэша можно временно указать `hash = pkgs.lib.fakeHash;`:
после загрузки Nix сообщит ожидаемый и фактический хэш. Перенесите значение
`got: sha256-...` в конфигурацию. Скачивание зависит от доступности раздачи.

При первом запуске подготовленные файлы автоматически копируются из Nix store
в `${XDG_DATA_HOME:-$HOME/.local/share}/titbit/game` либо в каталог из
`TITBIT_GAME` / `--data`. Мастер импорта не открывается. Сохранения и посторонние
файлы остаются на месте. Если `QUEST.ezd` уже есть, повторный запуск использует
существующую игру, включая пользовательские изменения. Ошибка импорта прерывает
запуск и не заменяет несовместимые файлы.

Можно передать произвольный источник, включая конкретный образ внутри раздачи,
через `lib.withGameData`:

```nix
let
  torrent = pkgs.fetchtorrent {
    url = "magnet:?xt=urn:btih:YOUR_INFO_HASH";
    hash = "sha256-...";
  };
in
inputs.titbit.lib.withGameData {
  system = pkgs.stdenv.hostPlatform.system;
  source = "${torrent}/path/to/disc.iso";
}
```

Это также позволяет использовать `pkgs.fetchurl`, `pkgs.fetchzip` или локальный
источник, добавленный через `pkgs.requireFile`. `package.gameData` предоставляет
пакет только с проверенными данными. При смене magnet движок не пересобирается.
API одинаков для Linux и macOS; на macOS данные получает и подготавливает хост,
а VM видит уже доступный для записи каталог.

## VM на macOS и Linux

```sh
nix run .#titbit-vm -- --data /path/to/writable/game
```

VM автоматически входит в графическую сессию и запускает игру. Каталог данных
передаётся через 9P; bindfs согласует владельца файлов с пользователем гостя,
поэтому сохранения остаются в исходном каталоге на хосте, в том числе при macOS
UID 501. Корневая файловая система гостя временная; отдельный постоянный диск
не создаётся. Nix store подключён для чтения, сеть гостю не предоставляется.

По умолчанию VM получает 2 CPU и 2 ГиБ RAM. На Apple Silicon используется
программная эмуляция x86 (TCG). Звук выводится через CoreAudio на macOS или
PulseAudio на Linux. Для запуска без звука:

```sh
TITBIT_VM_AUDIO=none nix run .#titbit-vm
```

`QEMU_OPTS` добавляет параметры QEMU. Параметры игры, кроме обрабатываемых
обёрткой `--data` и `--help`, передаются гостю без преобразования через shell.
После нормального завершения игры VM выключается; при ошибке окно терминала
сохраняет сообщение до нажатия Enter.

## Подключение к конфигурации

В `flake.nix` вашей конфигурации:

```nix
{
  inputs.titbit.url = "github:Pentusha/titbit-modern/nix-flake";

  outputs = { nixpkgs, titbit, ... }: {
    nixosConfigurations.my-host = nixpkgs.lib.nixosSystem {
      system = "x86_64-linux";
      modules = [
        ./configuration.nix
        ({ pkgs, ... }: {
          environment.systemPackages = [
            titbit.packages.${pkgs.stdenv.hostPlatform.system}.default
          ];
          hardware.graphics = {
            enable = true;
            enable32Bit = true;
          };
        })
      ];
    };
  };
}
```

На x86-64 Linux `hardware.graphics.enable32Bit` предоставляет 32-битные драйверы
OpenGL. На ARM Linux используется программный рендеринг внутри эмулируемого
процесса; включать `enable32Bit` на ARM-хосте не нужно. На nix-darwin достаточно
добавить пакет из `packages.aarch64-darwin` в `environment.systemPackages`
и предоставить Linux builder для сборки гостя.

После слияния изменений upstream URL можно заменить на
`github:offx366/titbit-modern`. Можно добавить
`inputs.titbit.inputs.nixpkgs.follows = "nixpkgs"`, но выбранный nixpkgs должен
содержать LLVM 18, FFmpeg 6 и 32-битные зависимости. VC6, Wine и игровые данные
для сборки не нужны. Из-за отсутствия свободной лицензии на восстановленный
движок пакеты помечены `unfree`; flake разрешает только `titbit`, `titbit-tools` и `titbit-data`.

## Структура flake

Используется dendritic pattern, как в CEX: корневой `flake.nix` содержит inputs
и вызов `flake-parts`, а `import-tree` автоматически подключает модули из `nix/`.
Каждая возможность определена отдельно:

- `nix/flake-parts/` — системы, nixpkgs и форматирование;
- `nix/tools/` — нативные импортёр и Studio;
- `nix/engine/` — сборка 32-битного Linux-движка;
- `nix/emulation/` — запуск через `qemu-i386`;
- `nix/game-data/` — настраиваемый источник данных и автоматическая подготовка;
- `nix/vm/` — гостевая NixOS-система и запуск VM на разных хостах.

Все автоматически импортируемые `.nix` — модули flake-parts. Файлы
`_package.nix` — функции сборки, исключённые из автоматического импорта.
Гость VM оформлен как `flake.modules.nixos.titbit-vm`.

## Проверки

```sh
nix flake check -L
nix flake check --all-systems --no-build
nix build .#titbit-vm -L
nix fmt
```

Проверки запускают существующие тесты C++/CRT/ABI на x86 Linux, тесты импортёра
на каждой нативной системе и `--help` установленного движка, включая запуск
через QEMU на Linux. Проверка `titbit-game-data` использует синтетический ISO
без оригинальных игровых файлов: проверяет извлечение, автоматическую подготовку,
передачу путей и сохранность сохранений / изменённых архивов. Она работает без сети
и не скачивает magnet. `--all-systems --no-build` проверяет вычисление всех
платформ без попытки выполнить Darwin-программы на Linux. Сборка пакета VM
проверяет её конфигурацию и команду запуска; загрузка гостя и графический запуск
проверяются отдельно. Игровые данные для сборочных проверок не требуются.

Вместо `--minimal` используются SDL, OpenGL, GLU, FFmpeg 6 и zlib из nixpkgs.
Сборочные скрипты не скачивают зависимости и не используют библиотеки из `/usr`.

Подробности режимов эмуляции: [QEMU User Mode](https://www.qemu.org/docs/master/user/),
[QEMU System Emulation](https://www.qemu.org/docs/master/system/introduction.html).
