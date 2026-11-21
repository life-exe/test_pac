# План: задача 02 — лабиринт `Maze` в `Game`, тесты и отрисовка

## Контекст

В `GameCore` уже добавлены контрактные файлы: `Core/Types.h` (`TileType`, `Mover`, `GhostId`), `Core/Level.h` (`c_classicLevel`, 21×21), `Core/Maze.h/.cpp` (парсер `Maze::fromAscii` → `std::expected<Maze, MazeError>`) и `Tests/TestHelpers.h` (`c_smallLevel` 9×11, `makePlayingGame`). Их **не меняем**. Нужно: встроить `Maze` в `Game`, отдать сетку наружу через `GameSnapshot`, покрыть парсер тестами и нарисовать лабиринт в `RaylibRenderer`. `GameCore` не должен зависеть от `raylib`.

Важная деталь: конструктор `Maze()` приватный, поэтому `m_maze` нельзя создать по умолчанию — его обязательно инициализирует конструктор `Game`.

## Шаги

### 1. Сохранить план
После утверждения — записать этот план в `tasks/02-maze-and-level.plan.md`.

### 2. `Core/GameSnapshot.h`
- `#include <span>`; добавить поля `int mazeRows{0};`, `int mazeCols{0};`, `std::span<const TileType> tiles;`.
- Комментарий над структурой (англ.): срез `tiles` указывает во внутренний буфер `Maze` и валиден до следующего вызова `update`, `handleCommand` или `reset`.

### 3. `Core/Game.h`
- `#include "Maze.h"`; поле `Maze m_maze;` (первым, до `m_state`).
- `Game();` и `explicit Game(Maze maze);` вместо `Game() = default;`.
- `snapshot()` дополнить `.mazeRows = m_maze.rows(), .mazeCols = m_maze.cols(), .tiles = m_maze.tiles()` (designated initializers в порядке полей).
- `Level.h` в заголовок не тянем — он нужен только в `Game.cpp`.
- Перемещаемость: никаких пользовательских деструктора/копирования — правило нуля; `Maze` (vector + POD) перемещаем, значит и `Game` тоже. Закрепить `static_assert(std::is_move_constructible_v<Game> && std::is_move_assignable_v<Game>)` в тесте.

### 4. `Core/Game.cpp`
- `#include "Level.h"`, `<utility>`.
- `Game::Game() : Game(Maze::fromAscii(c_classicLevel).value()) {}`
- `Game::Game(Maze maze) : m_maze{std::move(maze)} {}`
- `update` / `handleCommand` / `reset` без изменений (`reset` лабиринт не трогает).

### 5. Тесты `src/Game/Tests/MazeTest.cpp` (GoogleTest)
Стиль как в `GameTest.cpp`: `{}`-инициализация, комментарии на английском, `using namespace LifeExe;`. Карты — `Tests::c_smallLevel`, `Core::c_classicLevel` и маленькие локальные `constexpr std::string_view` для ошибок.

| TEST | Что проверяет |
|---|---|
| `MazeTest.SmallLevelHasExpectedSize` | `rows()==9`, `cols()==11`, `tiles().size()==99`; то же для classic: 21×21, 441 |
| `MazeTest.PerimeterIsWallExceptTunnels` | обход первой/последней строки и первого/последнего столбца обеих карт: `Wall`, а клетки `T` (строка 5 малой, строка 9 classic) — `Tunnel` и только они |
| `MazeTest.PlayerCannotWalkThroughGhostDoor` | на клетке `=` (`{2,5}`): `Player` и `Ghost` — false, `GhostUsingDoor` — true |
| `MazeTest.WallIsNotWalkableForAnyMover` | стена `{0,0}` непроходима для всех трёх `Mover` |
| `MazeTest.OpenTilesAreWalkableForAllMovers` | `Pellet`, `PowerPellet`, `Empty`, `Tunnel` — проходимы для всех |
| `MazeTest.SpawnPositionsMatchMap` | `playerSpawn()=={5,5}`, `ghostSpawn(Ruby)=={1,5}`, `Rosie {3,4}`, `Indigo {3,5}`, `Coral {3,6}` |
| `MazeTest.SpawnTilesAreEmpty` | `tileAt` каждого спавна == `TileType::Empty` |
| `MazeTest.OutOfBoundsIsWall` | `{-1,0}`, `{0,-1}`, `{rows,0}`, `{0,cols}`, большие значения → `Wall`, и `isWalkable` → false |
| `MazeErrorTest.EmptyMap` | `""` и `"\n"` → `EmptyMap` |
| `MazeErrorTest.RaggedRows` | строки разной длины |
| `MazeErrorTest.UnknownSymbol` | символ `X` в корректной иначе карте |
| `MazeErrorTest.MissingPlayerSpawn` | нет `P` |
| `MazeErrorTest.MissingGhostSpawn` | нет одной из цифр `1..4` |
| `MazeErrorTest.DuplicatePlayerSpawn` | два `P` → `DuplicateSpawn` |
| `MazeErrorTest.DuplicateGhostSpawn` | две `2` → `DuplicateSpawn` |
| `MazeErrorTest.MissingGhostDoor` | нет `=` |
| `MazeErrorTest.UnpairedTunnel` | один `T` в строке (и отдельно `T` не на краю) |
| `GameMazeTest.DefaultGameUsesClassicLevel` | `Game{}.snapshot()`: 21×21, `tiles.size()==441` |
| `GameMazeTest.SnapshotExposesInjectedMaze` | `Tests::makePlayingGame()`: 9×11, `tiles[r*cols+c]` совпадает с `Maze::tileAt` |
| `GameMazeTest.GameIsMovable` | `static_assert` на move-конструирование/присваивание + перемещённая игра отдаёт тот же лабиринт |

Ошибочные карты строятся так, чтобы срабатывала именно проверяемая ошибка с учётом порядка проверок в `Maze::fromAscii` (размеры → символы/дубликаты → P → призраки → дверь → туннели). Проверка: `ASSERT_FALSE(result.has_value()); EXPECT_EQ(result.error(), MazeError::X);`.

### 6. `src/Game/CMakeLists.txt`
- Добавить `Tests/TestHelpers.h` и `Tests/MazeTest.cpp` в `add_executable(GameTests ...)`.
- `target_include_directories(GameTests PRIVATE Tests)`.

### 7. `Presentation/RaylibRenderer.cpp` — отрисовка лабиринта
- `#include <algorithm>`; константа `constexpr int c_hudMargin{48};` (в анонимном namespace).
- Масштаб: `tileSize = std::max(1, std::min(screenWidth / cols, (screenHeight - 2 * c_hudMargin) / rows))` (при 800×800 и 21×21 → 33 px).
- Центрирование: `originX = (screenWidth - cols * tileSize) / 2`, `originY = c_hudMargin + (screenHeight - 2 * c_hudMargin - rows * tileSize) / 2`.
- Фон `BLACK` (неон лучше читается на чёрном).
- Обход `snapshot.tiles` (индекс `row * mazeCols + col`):
  - `Wall` → `DrawRectangleRounded` неоново-синим (`Color{33, 33, 255, 255}`), небольшой отступ внутри тайла, `roundness ≈ 0.4f`;
  - `GhostDoor` → горизонтальная розовая перекладина по центру тайла (`Color{255, 184, 222, 255}`, высота ≈ `tileSize / 4`, минимум 1 px);
  - остальные тайлы пока не рисуем (пеллеты — будущие задачи).
- Защита: при `mazeRows == 0 || mazeCols == 0` лабиринт не рисуется.
- HUD ужать в верхнюю полосу 48 px (FPS, время и состояние в одну строку), подсказку управления — в нижнюю полосу; убрать демонстрационные круг/квадрат из центра, оставив текст `PAUSED` / `GAME OVER` поверх лабиринта.
- Вынести расчёт раскладки и отрисовку сетки в вспомогательные функции (`computeLayout`, `drawMaze`) в анонимном namespace, чтобы `render()` остался коротким.

### 8. Форматирование
`python automation/format_files.py`.

## Проверка
1. `python automation/build_debug.py` — сборка без предупреждений (`/W4 /WX`).
2. `python automation/run_tests.py` — все тесты зелёные, включая старые `GameTest`/`KickoffAcceptanceTest` (они используют `Game()` → classic level) и новые `MazeTest.*`, `MazeErrorTest.*` (8 ошибок, 9 тестов), `GameMazeTest.*`.
3. `python automation/format_files.py --check` — без замечаний.
4. `python automation/run_tidy.py` — без замечаний.
5. `python automation/run_asan.py` — тесты под ASan/UBSan чистые.
6. Запуск `GameApp` — визуально: лабиринт по центру, синие скруглённые стены, розовая дверь, HUD не перекрывает поле.
7. Отчёт кодами возврата. Коммит не делаю — предложу команды `git add` / `git commit`; доставка через ветку `feature/maze-and-level`, `/code-review` и PR.

## Замечания (не правлю, только сообщаю)
- `config.json` уже изменён на 800×800 — учтено в расчёте масштаба.
- Контрактный `Types.h` без перевода строки в конце — косметика.
