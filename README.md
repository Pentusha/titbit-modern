<h1 align="center">Ядерный Титбит</h1>

<p align="center"><b>Восстановленные исходники игры 2003 года — и версия для Windows 10/11</b></p>
<p align="center">VZlab · Бука · 2003</p>
<p align="center"><a href="#как-играть">Как играть</a> · <a href="#titbit-studio">Редактор сцен</a> · <a href="#для-разработчиков">Сборка</a></p>

![Ядерный Титбит — первая улица](docs/images/game-street.png)

Исходный код «Ядерного Титбита», восстановленный по оригинальному EXE. Игра работает
на современных Windows: полный экран, без диска в приводе и без старых кодеков.
Игровая логика оригинальная, всё «современное» лежит отдельно в `src/compat/`.

> [!IMPORTANT]
> **Для игры нужна ваша копия «Ядерного Титбита»** — оригинальный диск, образ диска
> (ISO / IMG / CUE / CCD) или уже установленная игра. Уровни, модели, музыка, озвучка
> и видео берутся оттуда: в этот репозиторий и в готовые сборки они **не входят**.

## Как играть

1. Скачайте `TitbitModern-*-windows-x64.zip` из [Releases](https://github.com/offx366/titbit-modern/releases/latest) и распакуйте, например в `C:\Games\TitbitModern`.
2. Запустите **TitbitSetup.exe** и укажите диск, образ или папку установленной игры — данные скопируются и проверятся.
3. Играйте: **TitbitModern.exe**. Настройки экрана — **TitbitSettings.exe**.

Python, компилятор и интернет не нужны. После импорта диск можно убрать.
Подробно и что делать, если не получилось: [docs/GETTING_STARTED.md](docs/GETTING_STARTED.md).

## Что нового по сравнению с диском

- **Полный экран** в родном разрешении монитора, без смены видеорежима; **Alt+Enter / F11** — окно ↔ экран.
- Пропорции **4:3** или растяжение, правильная мышь при любом размере окна и масштабе Windows.
- Видео работают без установки старых кодеков.
- Не нужен диск в приводе.

![Главное меню](docs/images/game-menu.png)

## Titbit Studio

Редактор сцен: запустите **TitbitStudio.exe** — откроется браузер со всеми 70 локациями игры.
Камера, выбор и перемещение объектов, анимации, экспорт в Blender (OBJ / GLB) и обратно,
сборка своего мода. Архив игры только читается.

![Titbit Studio](docs/images/studio.png)

Возможности и управление: [editor/README.md](editor/README.md).

## Для разработчиков

Нужны Python 3.11 и своя установка **Visual C++ 6.0 SP6** (с MASM) + Resource Compiler из Windows SDK.
Инструменты Microsoft в репозиторий не входят.

```powershell
$env:TITBIT_VC6 = 'C:\Program Files (x86)\Microsoft Visual Studio\VC98'
py -3.11 -m venv .venv
.\.venv\Scripts\python.exe -m pip install -r requirements.txt
.\.venv\Scripts\python.exe tools/build_modern.py --fresh
```

Готовые EXE — в `build/modern/`. Всё остальное (версия без совместимости, Studio,
portable-пакет): [docs/BUILD.md](docs/BUILD.md).

| Папка | Что внутри |
|---|---|
| `src/native`, `src/lanes` | движок игры |
| `src/compat` | совместимость с современными Windows |
| `editor` | Titbit Studio |
| `tools`, `config` | сборка и импорт данных |

## Состояние

Проверено: сборка из этого репозитория, импорт данных из образа диска, игра от меню до
первой улицы. Полное прохождение и все сочетания Windows/видеокарт пока не проверены —
сообщайте о проблемах в Issues. Подробнее: [docs/VALIDATION.md](docs/VALIDATION.md).

Игра и её материалы — © VZlab / Бука. Данные игры, образы диска и инструменты Microsoft
не распространяются. Сторонние библиотеки: [THIRD_PARTY_NOTICES.md](THIRD_PARTY_NOTICES.md),
игровые данные: [docs/ASSETS.md](docs/ASSETS.md).
