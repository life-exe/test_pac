# Леса проекта: спецификация

Что должно быть в новом проекте C++ до первой строки предметного кода.
Документ — техническое задание для агента (скилл `new-cpp-project`) и
контрольный список для человека. Критерий приёмки один: каталог проходит
`python checks/scaffold.py <каталог>` из репозитория курса.

Имя проекта задаётся при генерации; ниже оно обозначено `<Name>` (PascalCase,
например `Tetris`). `<name>` — то же имя в нижнем регистре. Проект-эталон,
с которого снята спецификация: этот репозиторий, `<Name>` = `Pacman`.

Всё, что ниже — факты, а не пожелания: версии, имена, ключи конфигурации,
отличия от умолчаний. Где сказано «целиком», файл воспроизводится дословно.

## 1. Структура

```text
<name>/
├── CMakeLists.txt                  # корень: project(CppGameCourse), C++23, find_package, add_subdirectory
├── conanfile.txt                   # nlohmann_json/3.12.0, gtest/1.18.0; generators CMakeDeps, CMakeToolchain
├── config.json                     # {"title": "<Name>", "window": {"width": 800, "height": 600}}
├── README.md                       # по схеме из раздела 9
├── .gitignore                      # целиком, см. ниже
├── .clang-format                   # целиком, раздел 5
├── .clang-tidy                     # целиком, раздел 6
├── Doxyfile                        # отличия от умолчаний, раздел 8
├── mkdocs.yml                      # целиком, раздел 8
├── requirements-docs.txt           # mkdocs==1.6.1, mkdocs-material==9.7.7
├── protection.json                 # целиком, раздел 10
├── docs/
│   ├── index.md                    # обзор, быстрый старт, список разделов
│   ├── architecture.md             # три цели, конфигурация из данных, модули cmake/
│   └── development.md              # скрипты automation/, порядок перед коммитом, документация
├── profiles/
│   ├── windows-msvc-debug          # msvc, cppstd=23, runtime dynamic, версия через detect_api
│   ├── windows-msvc-release
│   ├── linux-gcc-debug             # gcc 14, libstdc++11, cppstd=23, compiler_executables g++-14
│   └── linux-gcc-release
├── cmake/
│   ├── CompilerWarnings.cmake      # set_project_warnings(target)
│   ├── Sanitizers.cmake            # option(ENABLE_ASAN), enable_sanitizers(target)
│   └── ClangFormat.cmake           # цели format и format-check
├── automation/
│   ├── common.py                   # единственное место, где различаются платформы
│   ├── generate_project_files.py
│   ├── build_debug.py
│   ├── build_release.py
│   ├── run_tests.py
│   ├── run_coverage.py
│   ├── run_tidy.py
│   ├── run_asan.py
│   ├── format_files.py
│   ├── build_docs.py
│   └── clean.py
├── src/<Name>/
│   ├── CMakeLists.txt              # project(<Name>); цели <Name>Lib, <Name>, <Name>TestRunner
│   ├── Config.h                    # LifeExe::GameConfig, LifeExe::parseConfig
│   ├── Config.cpp
│   ├── main.cpp
│   └── Tests/
│       └── ConfigTest.cpp          # GoogleTest, четыре теста
└── .github/workflows/
    ├── build.yml                   # работы build-Debug, build-Release, linux-Debug, linux-Release
    ├── tests.yml                   # работы tests, linux-tests
    ├── code-quality.yml            # работы code-quality, linux-tidy
    └── docs.yml                    # работа docs и публикация на Pages
```

`.gitignore` целиком, в этом порядке:

```text
.vs/
build/
CMakeUserPresets.json
__pycache__/
LastCoverageResults.log
build-asan/
docs/api/
```

`docs/api/` игнорируется потому, что справочник Doxygen генерируется в него при
каждой сборке документации; `LastCoverageResults.log` оставляет OpenCppCoverage
в корне проекта.

## 2. Сборка

- CMake 3.25 или новее (`cmake_minimum_required(VERSION 3.25)`),
  `set(CMAKE_CXX_STANDARD 23)`, `set(CMAKE_CXX_STANDARD_REQUIRED ON)`.
  Стандарт C++23 нужен из-за `std::print`/`std::format`.
- Корневой `CMakeLists.txt`, в этом порядке: `project(CppGameCourse LANGUAGES CXX)`,
  стандарт, `find_package(nlohmann_json REQUIRED)`, `find_package(GTest REQUIRED)`,
  `enable_testing()`, `set_property(GLOBAL PROPERTY USE_FOLDERS ON)`,
  `CMAKE_RUNTIME_OUTPUT_DIRECTORY` и `CMAKE_ARCHIVE_OUTPUT_DIRECTORY` =
  `${CMAKE_BINARY_DIR}/bin`, `include(cmake/CompilerWarnings.cmake)` и
  `include(cmake/Sanitizers.cmake)` **до** `add_subdirectory(src/<Name>)`,
  затем `set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT <Name>)` и
  `include(cmake/ClangFormat.cmake)` — после.
- `src/<Name>/CMakeLists.txt`: `project(<Name> LANGUAGES CXX)`; дальше имена
  целей только через `${PROJECT_NAME}`, литерального имени в файле нет.
  - `${PROJECT_NAME}Lib` — `STATIC` из `Config.cpp`, `Config.h`;
    `target_include_directories(... PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})`;
    `target_link_libraries(... PRIVATE nlohmann_json::nlohmann_json)`.
  - `${PROJECT_NAME}` — исполняемый файл только из `main.cpp`, линкует
    `${PROJECT_NAME}Lib` `PRIVATE`; `VS_DEBUGGER_WORKING_DIRECTORY
    "${CMAKE_SOURCE_DIR}"`.
  - `${PROJECT_NAME}TestRunner` — из `Tests/ConfigTest.cpp`, линкует
    `${PROJECT_NAME}Lib` и `GTest::gtest_main` `PRIVATE`.
  - у всех трёх целей `FOLDER ${PROJECT_NAME}`, `set_project_warnings(...)` и
    `enable_sanitizers(...)`.
  - в конце `include(GoogleTest)` и
    `gtest_discover_tests(${PROJECT_NAME}TestRunner DISCOVERY_TIMEOUT 30)`.
- Conan 2. `conan install .` с `--output-folder`, `-pr:h` и `-pr:b` на один и
  тот же профиль, `--build=missing`. Toolchain подключается к CMake ключом
  `-DCMAKE_TOOLCHAIN_FILE=<дерево>/conan_toolchain.cmake`.
- Раскладка деревьев сборки различается по типу генератора и закреплена:
  Windows — одно дерево `build/`, обе конфигурации внутри, бинарные файлы в
  `build/bin/<Config>/`; Linux — по дереву на конфигурацию, `build/<Config>/`,
  бинарные файлы в `build/<Config>/bin/`. На Linux конфигурация фиксируется
  ключом `-DCMAKE_BUILD_TYPE=<Config>` и дополнительно передаётся
  `-DCMAKE_EXPORT_COMPILE_COMMANDS=ON`; на Windows конфигурация выбирается при
  сборке ключом `--config`.
- Профили Conan. Windows (`profiles/windows-msvc-debug`,
  `profiles/windows-msvc-release`) начинаются со строки
  `{% set compiler, version, compiler_exe = detect_api.detect_default_compiler() %}`,
  и `compiler`/`compiler.version` подставляются из неё — версия компилятора не
  зашивается. Настройки: `arch=x86_64`, `build_type=Debug|Release`,
  `compiler.cppstd=23`, `compiler.runtime=dynamic`,
  `compiler.runtime_type=Debug|Release`, `os=Windows`.
  Linux (`profiles/linux-gcc-debug`, `profiles/linux-gcc-release`):
  `arch=x86_64`, `build_type=Debug|Release`, `compiler=gcc`,
  `compiler.cppstd=23`, `compiler.libcxx=libstdc++11`, `compiler.version=14`,
  `os=Linux`, и в секции `[conf]` —
  `tools.build:compiler_executables={"c": "gcc-14", "cpp": "g++-14"}`
  (в Ubuntu 24.04 `g++` — это GCC 13, а `std::println` требует GCC 14).

## 3. Код

Пространство имён `LifeExe`. Типы PascalCase, функции camelCase, поля с
brace-init (`int width{0}`), параметры-строки как `std::string_view`,
форматирование через `std::format` и `std::println`. Комментарии в коде, CMake
и скриптах — на английском, короткие, объясняют причину, а не действие.

- `Config.h`: `#pragma once`; `struct GameConfig` с полями
  `std::string title`, `int width{0}`, `int height{0}`; объявление
  `GameConfig parseConfig(std::istream& input);`. Функция принимает поток, а не
  имя файла, — чтобы тест подавал `std::istringstream`.
- `Config.cpp`: разбор через `nlohmann::json`. Внутренняя функция в анонимном
  пространстве имён проверяет наличие ключа и бросает `std::runtime_error` с
  текстом `config: key '<ключ>' is missing`; `nlohmann::json::parse_error`
  перехватывается и превращается в `config: invalid JSON: ...`,
  `nlohmann::json::type_error` — в `config: wrong value type: ...`. Значения
  извлекаются явным `.get<T>()`.
- `main.cpp`: `int main()` без аргументов; всё тело в `try`; открывает
  `config.json` из текущего каталога через `std::ifstream`, при отсутствии
  файла печатает сообщение в `stderr` и возвращает `EXIT_FAILURE`; при успехе
  печатает `Hello, {}! Window: {}x{}`. Обработчики
  `catch (const std::exception&)` и `catch (...)` используют только
  `std::fputs` — он не бросает, поэтому исключение не выходит из `main`. Коды
  возврата — `EXIT_SUCCESS`/`EXIT_FAILURE`.
- `Tests/ConfigTest.cpp`: GoogleTest, набор `ConfigTest`, четыре теста —
  `ShouldReadTitleAndWindow`, `ShouldNameMissingKey` (проверяет, что имя
  пропущенного ключа есть в сообщении), `ShouldRejectInvalidJson`,
  `ShouldRejectWrongValueType`. Входные документы — сырые строковые литералы
  в `std::istringstream`.
- Публичный заголовок документирован комментариями `///`: пространство имён,
  структура, каждое поле через `///<`, функция с `@param`, `@return`,
  `@throws`. Недокументированная публичная сущность роняет Doxygen (раздел 8).

## 4. Скрипты `automation/` (Python 3.10+, без сторонних пакетов)

Скрипты запускаются из любого каталога: пути вычисляются от файла `common.py`,
а не от текущего каталога. На Linux вызываются командой `python3`.

`common.py` — единственное место, где различаются платформы:

- `PROJECT_ROOT`, `PROFILES_DIR`, `IS_WINDOWS = sys.platform == "win32"`,
  `CONFIGURATIONS = ("Debug", "Release")`;
- `run(command)` — печатает команду с префиксом `+`, запускает из корня
  проекта и завершает скрипт с её кодом при ошибке;
- `profile(configuration)` → `profiles/{windows-msvc|linux-gcc}-{debug|release}`;
- `build_dir(configuration, root="build")` → `build` на Windows,
  `build/<Config>` на Linux; параметр `root` нужен сборке с санитайзером;
- `bin_dir(configuration)`, `toolchain(configuration)`, `relative(path)`
  (путь от корня проекта с прямыми слэшами);
- `conan_install(configuration)`,
  `configure(configuration, build_tree=None, extra=())`,
  `ensure_generated(configuration="Debug")` (генерирует проект, если в дереве
  нет `CMakeCache.txt`), `cmake_build(build_tree, configuration)`,
  `build(configuration)`.

| Скрипт | Что делает |
|---|---|
| `generate_project_files.py` | `conan install` для Debug и Release; конфигурация CMake — одна на Windows, по одной на конфигурацию на Linux |
| `build_debug.py` | `build("Debug")` |
| `build_release.py` | `build("Release")` |
| `run_tests.py` | собирает Debug, затем `ctest --output-on-failure --output-junit test-report.xml` |
| `run_coverage.py` | Windows: OpenCppCoverage, HTML в `build/coverage/html` и Cobertura в `build/coverage/coverage.xml`, таблица по файлам, дозапись в `GITHUB_STEP_SUMMARY`, ключ `--min N` роняет запуск ниже порога; Linux: печатает `coverage is measured on Windows only, skipping` и выходит с кодом 0 |
| `run_tidy.py` | Windows: `cmake --build build --config Debug -- /p:RunCodeAnalysis=true /p:EnableClangTidyCodeAnalysis=true /p:EnableMicrosoftCodeAnalysis=false` — всё после `--` CMake передаёт MSBuild, которого находит сам по генератору Visual Studio, поэтому искать `MSBuild.exe` в `PATH` или в каталогах Visual Studio не нужно; Linux: `clang-tidy -p build/Debug` по всем `.cpp` из `src/` |
| `run_asan.py` | отдельное дерево через `build_dir("Debug", root="build-asan")`, конфигурация с `-DENABLE_ASAN=ON`, сборка, `ctest --output-on-failure` |
| `format_files.py` | цель `format`; с ключом `--check` — цель `format-check` |
| `build_docs.py` | `doxygen Doxyfile`, затем `python -m mkdocs build --strict`; с ключом `--serve` — `mkdocs serve` вместо сборки |
| `clean.py` | `git clean -Xdn` для показа и `git clean -Xdf` для удаления; с `--dry-run` только показывает |

Исполняемый файл тест-раннера в `run_coverage.py` — `<Name>TestRunner.exe` из
`bin_dir("Debug")`; измеряются только исходники `src/<Name>`, каталог `Tests`
исключён.

## 5. Форматирование

`.clang-format` целиком:

```yaml
BasedOnStyle: Microsoft
IndentWidth: 4
UseTab: Never
TabWidth: 4
BreakBeforeBraces: Allman
ColumnLimit: 140
AccessModifierOffset: -4
SortIncludes: false
AllowShortBlocksOnASingleLine: false
AlignAfterOpenBracket: DontAlign
AllowShortFunctionsOnASingleLine: Inline
PointerAlignment: Left
AllowShortIfStatementsOnASingleLine: true
SpacesBeforeTrailingComments: 2
AllowShortCaseLabelsOnASingleLine: true
IndentCaseLabels: true
AlwaysBreakTemplateDeclarations: Yes
```

`cmake/ClangFormat.cmake`: ищет `clang-format` в `PATH` и дополнительно в
каталогах установленной Visual Studio
(`C:/Program Files/Microsoft Visual Studio/*/*/VC/Tools/Llvm/bin` и
`.../Llvm/x64/bin`) — он поставляется с Visual Studio и обычно не лежит в
`PATH`. Если не найден — `message(WARNING ...)` и `return()`, конфигурация не
падает. Список файлов — `file(GLOB_RECURSE ... CONFIGURE_DEPENDS)` по
`src/*.cpp` и `src/*.h`. Цели: `format` (`-i`) и `format-check`
(`--dry-run --Werror`), обе с `WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}`.

`cmake/CompilerWarnings.cmake`: функция `set_project_warnings(target)`,
`target_compile_options(... PRIVATE)`; MSVC — `/W4 /WX /permissive-`,
остальные — `-Wall -Wextra -Wpedantic -Werror`.

## 6. Статический анализ

`.clang-tidy` целиком, с подстановкой имени в `HeaderFilterRegex`:

```yaml
Checks: '-*,bugprone-*,performance-*,modernize-*,-modernize-use-trailing-return-type,readability-*,-readability-magic-numbers,-readability-identifier-length,misc-const-correctness'
WarningsAsErrors: '*'
HeaderFilterRegex: 'src[/\\]<Name>[/\\].*'
FormatStyle: file
```

Любая находка — ошибка, поэтому запуск `run_tidy.py` падает на первом
замечании. Отключены три проверки: `modernize-use-trailing-return-type`,
`readability-magic-numbers`, `readability-identifier-length`.

## 7. Санитайзеры

`cmake/Sanitizers.cmake`: `option(ENABLE_ASAN "Build with AddressSanitizer" OFF)` —
по умолчанию выключено, инструментированная сборка живёт в отдельном дереве
`build-asan/`.

- На уровне файла, при `ENABLE_ASAN AND MSVC`: путь до
  `clang_rt.asan_dynamic-x86_64.dll` рядом с компилятором, `FATAL_ERROR` с
  подсказкой про компонент **C++ AddressSanitizer**, если файла нет; из
  `CMAKE_CXX_FLAGS_DEBUG` удаляется `/RTC1` — он несовместим с ASan.
- `enable_sanitizers(target)` при выключенной опции возвращается сразу.
  MSVC: `/fsanitize=address`, определение `_DISABLE_STL_ANNOTATION`
  (gtest из Conan собран без ASan, иначе `LNK2038`), `/INCREMENTAL:NO`, и для
  целей типа `EXECUTABLE` — `POST_BUILD` копирование рантайма ASan рядом с
  исполняемым файлом.
  GCC/Clang: `-fsanitize=address,undefined -fno-sanitize-recover=all
  -fno-omit-frame-pointer` при компиляции и `-fsanitize=address,undefined` при
  линковке. `-fno-sanitize-recover=all` обязателен: иначе UBSan сообщает и
  продолжает, и тест с неопределённым поведением всё равно проходит.

## 8. Документация

Два инструмента: Doxygen делает справочник API из комментариев в заголовках,
MkDocs — сайт из Markdown. Оба считают проблему ошибкой.

`Doxyfile` — умолчания Doxygen, кроме:

```text
PROJECT_NAME           = <Name>
PROJECT_BRIEF          = "Game logic library and executable"
OUTPUT_DIRECTORY       = docs
INPUT                  = src/<Name>
FILE_PATTERNS          = *.h
RECURSIVE              = NO
EXCLUDE                = src/<Name>/Tests
EXTRACT_ALL            = NO
WARN_IF_UNDOCUMENTED   = YES
WARN_AS_ERROR          = FAIL_ON_WARNINGS
QUIET                  = YES
GENERATE_HTML          = YES
HTML_OUTPUT            = api
GENERATE_LATEX         = NO
HAVE_DOT               = NO
JAVADOC_AUTOBRIEF      = YES
```

`EXTRACT_ALL = NO` вместе с `WARN_IF_UNDOCUMENTED` и `WARN_AS_ERROR` и даёт
правило «публичная сущность без комментария роняет сборку».

`mkdocs.yml` целиком:

```yaml
site_name: <Name>
site_description: Учебный проект курса «Инженерный C++ на реальной игре»
docs_dir: docs
site_dir: build/docs/site
theme:
  name: material
  language: ru
nav:
  - Обзор: index.md
  - Архитектура: architecture.md
  - Разработка: development.md
  - Справочник API: api/index.html
markdown_extensions:
  - admonition
  - pymdownx.superfences
```

`requirements-docs.txt` — закреплённые версии, `mkdocs==1.6.1` и
`mkdocs-material==9.7.7`.

Страницы `docs/`: `index.md` — что за проект, быстрый старт, список разделов;
`architecture.md` — три цели CMake и что из этого следует для тестов,
конфигурация из данных, модули `cmake/`; `development.md` — таблица скриптов
`automation/`, различия платформ, порядок проверок перед коммитом. Сайт
отвечает на «почему так устроено» и «как здесь работать»; как собрать в три
команды — дело README.

## 9. README

`README.md` на русском, в этом порядке:

1. заголовок `# <Name>`;
2. четыре значка состояния — по одному на workflow (`build`, `tests`,
   `code-quality`, `docs`), каждый ссылкой на свой список прогонов;
3. абзац о проекте: логика в библиотеке `<Name>Lib`, исполняемый файл и тесты
   линкуют одну и ту же библиотеку, настройки читаются из `config.json`;
4. **Требования** — таблица для Windows (Windows 10 или 11, Visual Studio 2022
   с компонентами «Разработка классических приложений на C++» и
   «C++ AddressSanitizer», CMake 3.25 или новее, Python 3.10 или новее,
   Conan 2.x, OpenCppCoverage, Doxygen, MkDocs) и блок для Linux (Ubuntu 24.04,
   GCC 14, команда установки, оговорка про покрытие);
5. **Сборка и запуск** — команды для Windows и Linux, оговорка «запускать из
   корня проекта», путь к Release, полная пересборка через `clean.py`;
6. **Тесты** — таблица `run_tests.py`, `run_coverage.py --min 70`,
   `run_asan.py`; где лежат тесты и отчёты;
7. **Качество кода** — таблица `format_files.py --check` и `run_tidy.py` со
   ссылками на `.clang-format` и `.clang-tidy`; строка про предупреждения
   компилятора и `cmake/CompilerWarnings.cmake`;
8. **Документация** — ссылка на опубликованный сайт, команды `build_docs.py` и
   `--serve`, что делает Doxygen и что MkDocs;
9. **Структура проекта** — дерево верхнего уровня с однострочными пояснениями;
10. **CI** — таблица «workflow → работы → что проверяет → артефакты», правило
    защиты ветки со ссылкой на `protection.json`, что лежит в артефактах, где
    смотреть состояние.

Каждый скрипт из `automation/` назван в README хотя бы один раз:
`generate_project_files.py`, `build_debug.py`, `build_release.py`,
`run_tests.py`, `run_coverage.py`, `run_tidy.py`, `run_asan.py`,
`format_files.py`, `build_docs.py`, `clean.py`.

## 10. CI

GitHub Actions, четыре файла в `.github/workflows/`. У каждого workflow один
предмет, платформа — вторая ось: упавшее форматирование не скрывает результат
тестов. Общее у всех: `on: push` в основную ветку и `on: pull_request`;
`concurrency` с группой `${{ github.workflow }}-${{ github.ref }}` и
`cancel-in-progress: true` (у `docs.yml` — `false`); `actions/checkout@v4`;
кэш `actions/cache@v4` по пути `~/.conan2/p` с ключом
`conan-${{ runner.os }}-${{ hashFiles('conanfile.txt', 'profiles/**') }}`.
Conan ставится через `pip install conan` на Windows и `pipx install conan` на
Linux; на Linux перед этим `sudo apt-get install -y g++-14`.

| Файл | Работы | Что делает | Артефакты |
|---|---|---|---|
| `build.yml` | `build-Debug`, `build-Release` (матрица, `windows-latest`), `linux-Debug`, `linux-Release` (матрица, `ubuntu-latest`) | генерация проекта и сборка конфигурации, `fail-fast: false` | `game-Debug`, `game-Release`, `game-linux-Debug`, `game-linux-Release` — исполняемый файл и `config.json` |
| `tests.yml` | `tests` (`windows-latest`), `linux-tests` (`ubuntu-latest`) | Windows: `run_asan.py`, `run_tests.py`, установка OpenCppCoverage, `run_coverage.py --min 70`; Linux: `run_asan.py`, `run_tests.py` | `coverage-report`, `test-report` (оба `if: always()`) |
| `code-quality.yml` | `code-quality` (`windows-latest`), `linux-tidy` (`ubuntu-latest`) | Windows: `format_files.py --check`, затем `run_tidy.py`; Linux: `run_tidy.py` (в установку добавлен `clang-tidy`) | — |
| `docs.yml` | `docs` (`windows-latest`) и `deploy` | `choco install doxygen.install`, `pip install -r requirements-docs.txt`, `build_docs.py`, `actions/upload-pages-artifact@v3`; `deploy` с `needs: docs` публикует через `actions/deploy-pages@v4` | — |

В `tests.yml` отчёт тестов дополнительно публикуется в Summary шагом
`phoenix-actions/test-reporting@v10` с `reporter: java-junit`. Работа `deploy`
выполняется только при `github.event_name == 'push'` и
`github.ref_name == github.event.repository.default_branch`, то есть на pull
request сайт собирается, но не публикуется; её права — `pages: write` и
`id-token: write`, окружение `github-pages`.

Правило защиты основной ветки — `protection.json`, целиком:

```json
{
  "required_status_checks": {
    "strict": false,
    "contexts": [
      "build-Debug",
      "build-Release",
      "tests",
      "code-quality",
      "docs",
      "linux-Debug",
      "linux-Release",
      "linux-tests",
      "linux-tidy"
    ]
  },
  "enforce_admins": true,
  "required_pull_request_reviews": null,
  "restrictions": null
}
```

Девять обязательных работ; `enforce_admins: true` — правило действует и на
владельца репозитория, иначе оно необязательное.

## 11. Приёмка

Единственный критерий: из корня репозитория курса

```powershell
python checks/scaffold.py <каталог>
```

Все строки `[OK  ]`. Скрипт прогоняет по указанному каталогу проверки спринтов
04–14, пропуская разделы про git и GitHub, которых у свежесозданного проекта
нет.

Самопроверка до приёмки, из каталога проекта, в этом порядке — на первой
ошибке остановиться:

```powershell
python automation/generate_project_files.py
python automation/build_debug.py
python automation/run_tests.py
python automation/format_files.py --check
python automation/run_tidy.py
python automation/run_asan.py
python automation/build_docs.py
```

Красный пункт приёмки — дефект этой спецификации, а не сгенерированного файла:
правка файла исправляет один экземпляр, правка спецификации — все следующие.

## Решено непоследовательно

Расхождения проекта-эталона, которые спецификация описывает как есть, но
которые сознательным решением не являются:

- имя проекта в корневом `CMakeLists.txt` — `CppGameCourse`, а не `<Name>`:
  оно осталось от первого спринта курса и не зависит от имени игры; подпроект
  `src/<Name>` назван правильно;
- `config.json` в эталоне содержит `"title": "Pac-Man"` — с дефисом, в отличие
  от имени проекта `Pacman`; для нового проекта `title` равен `<Name>`;
- каталог тестов назван `Tests` с большой буквы, остальные каталоги проекта —
  со строчной (`automation`, `cmake`, `docs`, `profiles`, `src`);
- в разделе «Структура проекта» файла README эталона строка про `profiles/`
  содержит дублированный фрагмент — опечатка, воспроизводить её не нужно;
- работа `docs` выполняется на `windows-latest`, хотя Doxygen и MkDocs
  одинаково работают на Linux и там дешевле;
- порядок строк в `.gitignore` сложился по ходу спринтов и смысла не несёт.
