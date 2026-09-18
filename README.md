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
.\build\Debug\game.exe
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
архив `game-release` с `game.exe` и `config.json`; распаковать и запустить из каталога с `config.json`

