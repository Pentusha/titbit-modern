# Nix / NixOS

Flake находится в корне Git-репозитория. Нужен Nix с включёнными `nix-command`
и `flakes`. Сборка движка поддерживается на `x86_64-linux`; исполняемый файл
остаётся 32-битным x86 ELF. VC6, Wine и игровые данные для сборки не нужны.
Инструменты LLVM 18 запускаются как x86-64, нативные объекты собираются с `-m32`.

## Сборка и запуск

Из корня репозитория:

```sh
nix build
./result/bin/titbit
```

При первом запуске без аргументов открывается мастер импорта. Нужна своя копия
игры: установленный каталог, диск либо образ ISO / IMG / CUE / CCD. Данные
импортируются в `${XDG_DATA_HOME:-$HOME/.local/share}/titbit/game`. Этот каталог
доступен для записи: движок хранит сохранения рядом с данными. Игровые файлы
в Nix store не добавляются.

Импорт через командную строку:

```sh
nix run .#titbit-setup -- --import /path/to/disc.iso \
  --dest "${XDG_DATA_HOME:-$HOME/.local/share}/titbit/game"
nix run
```

Другой каталог можно выбрать переменной `TITBIT_GAME` при запуске без аргументов
либо явно передать движку:

```sh
nix run . -- --data /path/to/writable/game
nix run .#titbit-studio
```

При запуске мастера с нестандартным каталогом укажите его в окне назначения
или через `--dest`. После импорта повторно запустите игру.

Пакеты:

| Выход flake | Содержимое |
| --- | --- |
| `packages.x86_64-linux.default` | То же, что `titbit` |
| `packages.x86_64-linux.titbit` | Движок, команда запуска, импортёр и Studio |
| `packages.x86_64-linux.titbit-tools` | Импортёр и Studio без движка |

Команды после установки: `titbit`, `titbit-setup`, `titbit-studio`. Движок также
появляется в меню приложений.

## Подключение к NixOS

В `flake.nix` вашей конфигурации:

```nix
{
  inputs.titbit.url = "github:offx366/titbit-modern";

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

`hardware.graphics.enable32Bit` предоставляет 32-битные драйверы OpenGL.
Объедините пример с существующими настройками графики вашей системы.
До слияния PR используйте URL своего форка или ветки вместо upstream URL.

Flake фиксирует nixpkgs из ветки `nixos-26.05` с LLVM 18 и FFmpeg 6.
При желании можно добавить
`inputs.titbit.inputs.nixpkgs.follows = "nixpkgs"`, но выбранный nixpkgs должен
содержать `llvmPackages_18`, `ffmpeg_6` и 32-битные зависимости. Из-за отсутствия
свободной лицензии на восстановленный движок пакеты помечены `unfree`;
flake разрешает только свои пакеты `titbit` и `titbit-tools`.

## Проверки

```sh
nix flake check -L
nix fmt
```

Проверка собирает движок, запускает существующие тесты C++/CRT/ABI и тесты
импортёра, а затем проверяет установленные команды через `--help`. Игровые
данные и графический дисплей для этих проверок не требуются. Проверка игры
и OpenGL с настоящими данными выполняется отдельно.

Вместо режима `--minimal` используются SDL, OpenGL, GLU, FFmpeg 6 и zlib из
nixpkgs. Во время сборки не выполняются скачивания из сборочных скриптов,
зависимостей из `/usr` или запись в пользовательский кэш. Изменения путей и выбор
Clang применяются только внутри Nix derivation.

## Другие платформы

Python-инструменты вынесены в отдельный derivation и не зависят от 32-битного
движка. При добавлении `aarch64-darwin` их можно адаптировать и проверять отдельно.
Сейчас этот flake не объявляет поддержку macOS.

Нативный движок требует x86-инструкций, четырёхбайтовых указателей и Microsoft
C++ ABI. Добавления `aarch64-darwin` в список систем недостаточно: для Apple
Silicon нужен отдельный перенос движка либо окружение, исполняющее x86 Linux.
