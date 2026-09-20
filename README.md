# Pacman

[![build](https://github.com/life-exe/test_pac/actions/workflows/build.yml/badge.svg)](https://github.com/life-exe/test_pac/actions/workflows/build.yml)
[![tests](https://github.com/life-exe/test_pac/actions/workflows/tests.yml/badge.svg)](https://github.com/life-exe/test_pac/actions/workflows/tests.yml)
[![code-quality](https://github.com/life-exe/test_pac/actions/workflows/code-quality.yml/badge.svg)](https://github.com/life-exe/test_pac/actions/workflows/code-quality.yml)
[![docs](https://github.com/life-exe/test_pac/actions/workflows/docs.yml/badge.svg)](https://github.com/life-exe/test_pac/actions/workflows/docs.yml)

Игра Pac-Man на C++ — учебный проект курса по инженерному C++. Логика вынесена
в библиотеку `PacmanLib`, исполняемый файл и тесты линкуют одну и ту же
библиотеку. Настройки окна читаются из `config.json` при запуске.

## Требования

Проект собирается на Windows (MSVC) и на Linux (GCC 14); на Windows Linux
доступен через [WSL](https://learn.microsoft.com/windows/wsl/install).

### Windows

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

### Linux

Ubuntu 24.04 или совместимый дистрибутив. GCC 14 нужен из-за `std::print`:
в Ubuntu 24.04 `g++` — это GCC 13, поэтому профили Conan вызывают `g++-14`
явно.

```bash
sudo apt install -y g++-14 cmake make clang-tidy clang-format pipx
pipx install conan
pipx ensurepath
```

Покрытие на Linux не измеряется: `run_coverage.py` печатает
`coverage is measured on Windows only, skipping` и завершается с кодом `0`.

## Сборка и запуск

```powershell
python automation/generate_project_files.py
python automation/build_debug.py
.\build\bin\Debug\Pacman.exe
```

На Linux — те же скрипты, командой `python3`:

```bash
python3 automation/generate_project_files.py
python3 automation/build_debug.py
./build/Debug/bin/Pacman
```

Запускать из корня проекта: `config.json` читается из текущего каталога, а не
из каталога с исполняемым файлом.

Сборка Release — `python automation/build_release.py`, результат в
`build\bin\Release\Pacman.exe` на Windows и `build/Release/bin/Pacman` на
Linux. Раскладка каталогов различается потому, что генератор Visual Studio
держит обе конфигурации в одном дереве сборки, а Makefile — по одной на
дерево; различие спрятано в `automation/common.py`.

## Тесты

| Команда | Что делает |
|---|---|
| `python automation/run_tests.py` | прогоняет тесты через CTest, сборка Debug выполняется автоматически |
| `python automation/run_coverage.py --min 70` | измеряет покрытие под OpenCppCoverage, возвращает код 1 ниже порога |
| `python automation/run_asan.py` | собирает отдельную сборку `build-asan` и прогоняет тесты под AddressSanitizer; на Linux вместе с ним включается UndefinedBehaviorSanitizer |

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

Опубликованный сайт — <https://life-exe.github.io/test_pac/>: архитектура,
разработка и справочник API.

Собрать локально:

```powershell
python automation/build_docs.py
python automation/build_docs.py --serve   # предпросмотр на http://127.0.0.1:8000
```

Справочник API собирает Doxygen из комментариев `///` в заголовках
`src/Pacman/*.h` по настройкам из `Doxyfile`; страницы о проекте собирает
MkDocs из Markdown в `docs/` по описанию в `mkdocs.yml`. Оба шага выполняет
`build_docs.py`, результат — в `build/docs/site`. Недокументированная
публичная сущность и битая ссылка считаются ошибками, поэтому документация
проверяется в CI работой `docs`.

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
├── profiles/                   # профили Conan: windows-msvc-*, linux-gcc-*: windows-msvc-*, linux-gcc-*
├── src/Pacman/                 # подпроект игры: цели Pacman, PacmanLib, PacmanTestRunner
└── .github/workflows/build.yml # CI
```

## CI

Каждый push в `master` и каждый pull request проверяются на чистых машинах
`windows-latest` и `ubuntu-latest` через GitHub Actions. У каждого workflow
один предмет, платформа — вторая ось: упавшее форматирование больше не
скрывает результат тестов, а ошибка одного компилятора — результат другого.

| Workflow | Работы | Что проверяет | Артефакты |
|---|---|---|---|
| `build.yml` | `build-Debug`, `build-Release`, `linux-Debug`, `linux-Release` | сборка обеих конфигураций матрицей, параллельно, на двух платформах | `game-Debug`, `game-Release`, `game-linux-Debug`, `game-linux-Release` |
| `tests.yml` | `tests`, `linux-tests` | тесты под санитайзерами, тесты, покрытие с порогом 70% (только Windows) | `coverage-report`, `test-report` |
| `code-quality.yml` | `code-quality`, `linux-tidy` | форматирование (`clang-format`), статический анализ (`clang-tidy`) | — |
| `docs.yml` | `docs` | сборка документации и публикация на GitHub Pages | — |

Слияние в `master` требует зелёного результата всех девяти работ; правило
защиты ветки описано в `protection.json`. Работы Linux завершаются заметно
быстрее: образ меньше, диск быстрее, а Makefile легче проектной модели
MSBuild.

Артефакты `game-Debug` и `game-Release` содержат `Pacman.exe` соответствующей
конфигурации и `config.json`, готовые к запуску; `game-linux-*` — то же для
Linux, исполняемый файл без расширения. `coverage-report` — HTML-отчёт
о покрытии, `test-report` — отчёт тестов в формате JUnit; таблица покрытия и
результаты тестов попадают в раздел **Summary** страницы прогона `tests`.

Статус последнего коммита виден в значках выше — каждый ведёт на свой список
прогонов на вкладке **Actions**. Пакеты Conan кэшируются между прогонами по
хэшу `conanfile.txt` и профилей, поэтому повторный прогон не собирает
зависимости заново.
