# Pacman

![build](https://github.com/life-exe/test_pac/actions/workflows/build.yml/badge.svg)

Игра Pac-Man на C++ — учебный проект курса по инженерному C++


## Сборка
```powershell
python automation/generate_project_files.py
python automation/build_debug.py
python automation/build_release.py
```

## Запуск
Запускать из корня проекта: config.json читается из текущего каталога, а не из каталога с исполняемым файлом.

```powershell
.\build\bin\Debug\Pacman.exe
```

## Очистка проекта

```powershell
python automation/clean.py --dry-run
python automation/clean.py
```

## Стиль кода

стиль задан файлом .clang-format

проверка

```powershell
python automation/format_files.py --check
```

исправление

```powershell
python automation/format_files.py
```

## CI

Каждый push и pull request собирается на чистой машине windows-latest через GitHub Actions,
описание — `.github/workflows/build.yml`

что выполняется:

- генерация файлов проекта (`generate_project_files.py`)
- сборка Debug (`build_debug.py`)
- сборка Release (`build_release.py`)
- проверка стиля (`format_files.py --check`)

результат каждого запуска — на вкладке **Actions**, статус последнего коммита виден в бейдже выше

собранный Release можно скачать без локальной сборки: страница запуска → раздел **Artifacts** →
архив `game-release` с `Pacman.exe` и `config.json`; распаковать и запустить из каталога с `config.json`

## Структура проекта

```text
project/
├── CMakeLists.txt                 # корень: проект, настройки, find_package, подпроекты
├── cmake/ClangFormat.cmake
└── src/
    └── Pacman/                    # подпроект игры
        ├── CMakeLists.txt         # project(Pacman): цели Pacman и PacmanLib
        ├── main.cpp               # тонкий: открыть файл, вызвать, напечатать
        ├── Config.h               # публичный интерфейс библиотеки
        └── Config.cpp             # разбор JSON — внутри библиотеки
```

Логика вынесена в PacmanLib: из теста можно вызвать функцию библиотеки, а main — нельзя