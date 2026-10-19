# Права и лицензии

**Код игры «Ядерный Титбит»** (движок, скрипты квестов, ресурсы интерфейса) — © VZlab / Бука.
Восстановленный исходный код публикуется с разрешения правообладателей.
Разрешение на публикацию сообщено владельцем проекта. Точный текст разрешения и
отдельная лицензия к игровому коду пока не приложены; этот NOTICE не заменяет их
и не объявляет игровой код MIT/BSD. Условия сторонних библиотек сохраняются.

**Не распространяются** и в репозиторий не входят: игровые данные (`QUEST.ezd`, видео
`*.avi`), оригинальный `titbit.exe`, образ диска, компилятор Microsoft Visual C++ 6.0,
исходники и библиотеки C/C++-рантайма Microsoft (CRT, STL). Их пользователь берёт из своих
копий.

## Сторонние библиотеки в репозитории

| Библиотека | Где | Лицензия |
|---|---|---|
| SeeR 0.94a, П. Подсядлы (1998–99) | `src/seer` | бесплатная библиотека, условия в `src/seer/README.TXT` |
| IJG JPEG 6a | `src/jpeg` | IJG License (README в каталоге) |
| Audiere | `src/audiere` | [GNU LGPL 2.1](docs/licenses/audiere-LGPL-2.1.txt) |
| Ogg / Vorbis (Xiph.Org) | `src/lanes/codex/ogg_*`, `vorbis_*`, `third_party/audiere-reference` | [Ogg](docs/licenses/libogg-1.0-COPYING), [Vorbis](docs/licenses/libvorbis-1.0-COPYING); оригинальные тексты из архивов 1.0 |
| WTL 7.5 (фрагменты) | `src/native/app_module*`, `archive_*` | Common Public License 1.0 (`src/native/app_module-CPL.TXT`) |
| three.js (редактор) | `editor/web/vendor` | MIT |
| zlib 1.1.3 | `third_party/zlib-1.1.3` | условия в её README / zlib.h |
| CenterPoint/XML | `src/lanes/engine41/cpxml43`, `engine42/cpxml` | CenterPoint Public License 1.0 (текст в каждом каталоге) |

Источники и контрольные суммы архивов Xiph: [provenance.json](docs/licenses/provenance.json).
