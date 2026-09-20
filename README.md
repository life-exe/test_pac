# Pacman

[![build](https://github.com/life-exe/test_pac/actions/workflows/build.yml/badge.svg)](https://github.com/life-exe/test_pac/actions/workflows/build.yml)

Игра Pac-Man на C++ — учебный проект курса по инженерному C++. Логика вынесена
в библиотеку `PacmanLib`, исполняемый файл и тесты линкуют одну и ту же
библиотеку. Настройки окна читаются из `config.json` при запуске.

## Требования

| Компонент | Версия | Примечание |
|---|---|---|
| Windows | 10 или 11 | |
| Visual Studio | 2022 | компоненты: **Разработка классических приложений на C++**, **C++ AddressSanitizer**; в комплекте идут clang-format и clang-tidy |
| CMake | 3.25 или новее | |
| Python | 3.10 или новее | |
| Conan | 2.x | `pip install conan` |
| OpenCppCoverage | любая | только для измерения покрытия: `winget install OpenCppCoverage.OpenCppCoverage` |
| Doxygen | любая | только для сборки документации: `winget install --id DimitriVanHeesch.Doxygen` |
| MkDocs | закреплена | только для сборки документации: `pip install -r requirements-docs.txt` |

## Сборка и запуск

```powershell
python automation/generate_project_files.py
python automation/build_debug.py
.\build\bin\Debug\Pacman.exe
```

Запускать из корня проекта: `config.json` читается из текущего каталога, а не
из каталога с исполняемым файлом.

Сборка Release — `python automation/build_release.py`, результат в
`build\bin\Release\Pacman.exe`.

## Тесты

| Команда | Что делает |
|---|---|
| `python automation/run_tests.py` | прогоняет тесты через CTest, сборка Debug выполняется автоматически |
| `python automation/run_coverage.py --min 70` | измеряет покрытие под OpenCppCoverage, возвращает код 1 ниже порога |
| `python automation/run_asan.py` | собирает отдельную сборку `build-asan` и прогоняет тесты под AddressSanitizer |

Тесты лежат в `src/Pacman/Tests/` и собираются в цель `PacmanTestRunner`.
Отчёты покрытия — `build/coverage/html/index.html` и
`build/coverage/coverage.xml`.

## Качество кода

| Команда | Что делает | Правила |
|---|---|---|
| `python automation/format_files.py --check` | проверяет форматирование, без `--check` исправляет | `.clang-format` |
| `python automation/run_tidy.py` | статический анализ clang-tidy, любая находка — ошибка | `.clang-tidy` |

Предупреждения компилятора включены на полный уровень и считаются ошибками;
настройка — `cmake/CompilerWarnings.cmake`.

## Документация

## Структура проекта

```text
project/
├── CMakeLists.txt              # корень: проект, стандарт, find_package, подпроекты
├── conanfile.txt               # зависимости: nlohmann_json, gtest
├── config.json                 # настройки окна, читаются при запуске
├── .clang-format               # правила форматирования
├── .clang-tidy                 # правила статического анализа
├── automation/                 # скрипты сборки, тестов и проверок
├── cmake/                      # модули: предупреждения, санитайзеры, формат
├── docs/                       # страницы сайта документации
├── profiles/                   # профили Conan
├── src/Pacman/                 # подпроект игры: цели Pacman, PacmanLib, PacmanTestRunner
└── .github/workflows/build.yml # CI
```

## CI

Каждый push и pull request собирается на чистой машине `windows-latest` через
GitHub Actions; описание — `.github/workflows/build.yml`. Прогон выполняет
генерацию файлов проекта, сборку Debug, тесты под AddressSanitizer, обычные
тесты, измерение покрытия с порогом 70%, сборку Release, проверку
форматирования и статический анализ.

Статус последнего коммита виден в значке выше, подробности прогона — на вкладке
**Actions**. Таблица покрытия и результаты тестов попадают в раздел
**Summary** страницы прогона.

Артефакты страницы прогона:

- `game-release` — `Pacman.exe` и `config.json`, готовые к запуску;
- `coverage-report` — HTML-отчёт о покрытии;
- `test-report` — отчёт тестов в формате JUnit.
