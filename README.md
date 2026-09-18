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
- запуск тестов (`run_tests.py`)
- измерение покрытия с порогом 70% (`run_coverage.py --min 70`)
- сборка Release (`build_release.py`)
- проверка стиля (`format_files.py --check`)

результат каждого запуска — на вкладке **Actions**, статус последнего коммита виден в бейдже выше

HTML-отчёт о покрытии загружается артефактом `coverage-report`, таблица покрытия видна
в разделе **Summary** страницы запуска

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
        ├── Config.cpp             # разбор JSON — внутри библиотеки
        └── Tests/ConfigTest.cpp   # тесты разбора конфига
```

Логика вынесена в PacmanLib: из теста можно вызвать функцию библиотеки, а main — нельзя

## Тесты

тесты лежат в `src/Pacman/Tests/`, собираются в цель `PacmanTestRunner` и линкуют `PacmanLib` —
ту же библиотеку, что игра

запуск через CTest (сборка Debug выполняется автоматически)

```powershell
python automation/run_tests.py
```

или напрямую, если проект уже собран

```powershell
.\build\bin\Debug\PacmanTestRunner.exe
```

что покрыто: разбор `config.json` — чтение заголовка и размеров окна, отсутствующий ключ
(сообщение об ошибке должно называть ключ), синтаксически битый JSON, неверный тип значения

## Покрытие

покрытие измеряет [OpenCppCoverage](https://github.com/OpenCppCoverage/OpenCppCoverage) —
он запускает тесты под отладчиком и по `.pdb` сопоставляет выполненные инструкции со строками
исходников, пересборка с особыми флагами не нужна

установка

```powershell
winget install OpenCppCoverage.OpenCppCoverage
```

измерение (сборка Debug выполняется автоматически)

```powershell
python automation/run_coverage.py
python automation/run_coverage.py --min 70
```

скрипт печатает таблицу покрытия по файлам и пишет два отчёта в `build/coverage/`:
`html/index.html` — для чтения человеком, `coverage.xml` в формате Cobertura — для CI

с флагом `--min N` скрипт возвращает код 1, если покрытие ниже порога; в CI порог — 70%,
он держится с запасом ниже текущего покрытия: его задача — не дать покрытию незаметно упасть,
когда в проект добавят непроверенный код

измеряется только Debug: в Release оптимизатор переставляет и выбрасывает строки, и отчёт
по нему вводит в заблуждение
