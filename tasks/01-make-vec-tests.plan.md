# План: задача 01 — модульные тесты `TypesTest.cpp`

## Контекст

В `src/Game/Core/Types.h` вручную добавлены базовые типы координат лабиринта: `GridPos` (адрес клетки: `row`, `col`, `operator==`, `+`, `-`, `int * GridPos`, всё `constexpr`) и `Vec2f` (непрерывная позиция в тайлах: `x`, `y`). Тестов для них нет. Задача (`tasks/01-make-vec-tests.md`) — закрепить их поведение тестами GoogleTest в цели `GameTests`, **не меняя `Types.h`**.

## Шаги

1. Создать `src/Game/Tests/TypesTest.cpp` в стиле `GameTest.cpp`:
   - `#include <gtest/gtest.h>`, `#include "Types.h"` (путь `Core` уже PUBLIC у `GameCore`), `using namespace LifeExe;`;
   - инициализация фигурными скобками `{}`, литералы `float` со строчным `f`, комментарии на английском (только «почему»);
   - сравнение `float` — через `EXPECT_FLOAT_EQ`, не `==`.
2. Тестовые сценарии (группы `GridPosTest`, `Vec2fTest`, всего 8 ≥ 6):

   | TEST | Что проверяет |
   |---|---|
   | `GridPosTest.DefaultConstructedIsZero` | `GridPos{}` → `row == 0`, `col == 0` |
   | `GridPosTest.EqualityComparesBothFields` | равные позиции равны; отличие только в `row` или только в `col` → не равны (`==` и выведенный `!=`) |
   | `GridPosTest.AdditionSumsComponentwise` | `{1, 2} + {3, -4} == {4, -2}`, нейтральность `GridPos{}` |
   | `GridPosTest.SubtractionSubtractsComponentwise` | `{5, 1} - {2, 3} == {3, -2}`, `p - p == GridPos{}` |
   | `GridPosTest.ScalarMultiplicationScalesBothFields` | `3 * {1, -2} == {3, -6}`, `0 * p == GridPos{}`, `-1 * p` |
   | `GridPosTest.OperationsAreEvaluatedAtCompileTime` | `static_assert` для `GridPos{}`, `==`, `+`, `-`, `*` (плюс `SUCCEED()`, чтобы сценарий отображался в отчёте) |
   | `Vec2fTest.DefaultConstructedIsZero` | `Vec2f{}` → `x`, `y` равны `0.0f` |
   | `Vec2fTest.FieldsCanBeAssigned` | запись `x`, `y` по одному и через `Vec2f{.x = 1.5f, .y = -2.25f}`; поля независимы |

3. Подключить файл к цели в `src/Game/CMakeLists.txt`:
   ```cmake
   add_executable(GameTests
       Tests/GameTest.cpp
       Tests/TypesTest.cpp
   )
   ```
   `gtest_discover_tests` подхватит новые тесты автоматически.
4. Отформатировать: `python automation/format_files.py`.

## Проверка

- `python automation/run_tests.py` — сборка Debug (`/W4 /WX`) и CTest: все тесты зелёные, в выводе 8 новых `GridPosTest.*` / `Vec2fTest.*`.
- `python automation/format_files.py --check` — без замечаний.
- Отчитаться кодами возврата и списком тестов; коммит не делать — предложить команды `git add` / `git commit` (доставка через ветку, `/code-review` и PR по `AGENTS.md`).

## Замечания к `Types.h` (не править, только сообщить)

- В конце файла нет перевода строки (`\ No newline at end of file` в `git diff`) — косметика, сборку не ломает.
- Есть только `int * GridPos`; `GridPos * int` не компилируется. Если это осознанно — ок, тесты используют только существующую форму.
