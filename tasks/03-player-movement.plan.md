# План: задача 03 — `Player` в `Game`, тесты движения и управление

## Контекст

Контрактные файлы уже добавлены вручную: `Core/Types.h` (`Direction`, `Vec2f`, `opposite`, `directionDelta`), `Core/Command.h` (`MoveUp/Down/Left/Right`), `Core/GameConstants.h` (`c_playerSpeed = 8` тайлов/с), `Core/GridMover.*` (модель «тайл + прогресс», буфер поворота, разворот, туннель) и `Core/Player.*` (`queueDirection`, `update`, `position/direction/facing/isMoving`). Их **не меняем**. Нужно: встроить `Player` в `Game`, отдать его состояние через `GameSnapshot`, покрыть движение тестами и подключить клавиши и отрисовку в `RaylibRenderer`. `GameCore` не зависит от `raylib`.

Важные детали из кода:
- У `Player` нет конструктора по умолчанию → `m_player` обязательно инициализируется в списке инициализации `Game(Maze)` и объявляется **после** `m_maze` (порядок инициализации членов).
- Стартовое состояние игрока: `direction == None`, `facing == Left`, `isMoving == false`, `position == {col + 0.5f, row + 0.5f}`.
- Скорость 8 тайлов/с → `dt = 0.125f` = ровно 1 тайл, `0.0625f` = полтайла. Двоичные дроби дают точные координаты, сравниваем через `EXPECT_FLOAT_EQ`.

## Шаги

### 1. Сохранить план
После утверждения — записать этот план в `tasks/03-player-movement.plan.md`.

### 2. `Core/GameSnapshot.h`
- Перед `GameSnapshot` объявить:
  `struct PlayerSnapshot { Vec2f position; Direction direction{Direction::None}; Direction facing{Direction::Left}; bool isMoving{false}; };` (с Doxygen-комментариями `///<`).
- В `GameSnapshot` добавить поле `PlayerSnapshot player;` (последним).

### 3. `Core/Game.h`
- `#include "Player.h"`; поле `Player m_player;` сразу после `Maze m_maze;`.
- `snapshot()` дополнить `.player = {.position = m_player.position(), .direction = m_player.direction(), .facing = m_player.facing(), .isMoving = m_player.isMoving()}`.
- Правило нуля сохраняется: `Player`/`GridMover` — POD-подобные, `Game` остаётся перемещаемым (существующий `static_assert` в тестах продолжит работать).

### 4. `Core/Game.cpp`
- `Game::Game(Maze maze) : m_maze{std::move(maze)}, m_player{m_maze.playerSpawn()} {}`.
- `update(dt)`: внутри ветки `Playing` добавить `m_player.update(m_maze, dt);`.
- `handleCommand`: заменить четыре пустых `case Move*` на вызов вспомогательной функции в анонимном namespace `Direction toDirection(Command)` (или прямой `switch`) → `if (m_state == GameState::Playing) { m_player.queueDirection(...); }`. В `Paused`/`GameOver` команда игнорируется (не буферизуется).
- `reset()`: `m_player = Player{m_maze.playerSpawn()};`.

### 5. Тесты `src/Game/Tests/PlayerTest.cpp` (GoogleTest)
Стиль как в `GameTest.cpp`/`MazeTest.cpp`: `using namespace LifeExe;`, `{}`-инициализация, суффикс `f`, комментарии на английском. Игра — `Tests::makePlayingGame()` (карта `c_smallLevel`, спавн `{5,5}` → позиция `(5.5, 5.5)`). Локальный хелпер `expectPosition(game, x, y)`.

Карта `c_smallLevel` (строки 0..8, столбцы 0..10): туннели `T` в строке 5; строка 4 открыта только в столбцах 1 и 9; строки 6–7 открыты в столбцах 1..9; строка 8 — стена.

| TEST | Сценарий и ожидания |
|---|---|
| `PlayerTest.SnapshotStartsAtSpawn` | без команд: `(5.5, 5.5)`, `None`, `facing Left`, не движется |
| `PlayerTest.MovesAlongStraightCorridor` | `MoveLeft`, `update(0.125f)` → `(4.5, 5.5)`, `direction Left`, `isMoving`; ещё `update(0.0625f)` → `x = 4.0` |
| `PlayerTest.StopsAtCellCentreBeforeWall` | `MoveDown`, `update(0.5f)` (4 тайла, а до стены 2) → ровно `(5.5, 7.5)`, `isMoving == false`, `direction None`, `facing Down` |
| `PlayerTest.BufferedTurnFiresAtFirstOpenJunction` | `MoveLeft`, `update(0.0625f)`; `MoveUp` (вверх стена) → после `update(0.125f)` ещё `Left`, `y = 5.5`; продолжать до центра `(1.5, 5.5)` и ещё тайл → `direction Up`, `(1.5, 4.5)`, `facing Up` |
| `PlayerTest.ReversesInstantlyMidCellWithoutJump` | `MoveLeft`, `update(0.0625f)` → `x = 5.0`; `MoveRight`, `update(0.0f)` → позиция та же `(5.0, 5.5)`, `direction Right`, `facing Right`; `update(0.0625f)` → `x = 5.5` |
| `PlayerTest.TunnelWrapsToOppositeSideOfRow` | `MoveRight`, кадрами по `0.0625f` 12 раз (6 тайлов: до `{5,10}` и через край) → `(0.5, 5.5)`, `direction Right`; на каждом кадре `y == 5.5` |
| `PlayerTest.LargeStepCrossesSeveralCellsAndTurns` | `MoveLeft`, `update(0.0f)` (направление принято), `MoveUp`, один `update(0.75f)` (6 тайлов: 4 влево + 2 вверх) → `(1.5, 3.5)`, `direction Up`, `facing Up`, `isMoving` |
| `PlayerTest.MoveCommandsIgnoredWhilePaused` | `Pause`, `MoveLeft`, `update(0.5f)` → на спавне; `Resume`, `update(0.125f)` → всё ещё на спавне и `None` (команда не была забуферизована) |
| `PlayerTest.ResetReturnsPlayerToSpawn` | `MoveLeft`, `update(0.25f)`, `Reset` → спавн, `None`, `facing Left` |

### 6. `src/Game/CMakeLists.txt`
- Добавить `Tests/PlayerTest.cpp` в `add_executable(GameTests ...)`.

### 7. `Presentation/RaylibRenderer.cpp`
- `processInput()`: таблица пар «клавиша → команда» (`constexpr std::array` в анонимном namespace): `KEY_W`/`KEY_UP` → `MoveUp`, `KEY_S`/`KEY_DOWN` → `MoveDown`, `KEY_A`/`KEY_LEFT` → `MoveLeft`, `KEY_D`/`KEY_RIGHT` → `MoveRight`; опрос через `IsKeyPressed` в цикле, чтобы `processInput` остался коротким.
- `render()`: вычислить `MazeLayout` один раз, передать в `drawMaze` и новый `drawPlayer(snapshot.player, layout)`:
  центр `originX + position.x * tileSize`, `originY + position.y * tileSize`, радиус `tileSize * 0.4f`, `DrawCircleV(..., YELLOW)`. Комментарий: временная заглушка до спрайта.
- Подсказку управления дополнить: `[WASD/Arrows] Move`.

### 8. Форматирование
`python automation/format_files.py`.

## Проверка
1. `python automation/build_debug.py` — сборка без предупреждений (`/W4 /WX`).
2. `python automation/run_tests.py` — все тесты зелёные, включая старые и новые `PlayerTest.*`.
3. `python automation/format_files.py --check` — без замечаний.
4. `python automation/run_tidy.py` — без замечаний.
5. `python automation/run_asan.py` — чисто.
6. Запуск `GameApp` (по желанию пользователя): жёлтый круг на спавне, двигается WASD/стрелками, проходит туннель, стоит на паузе.
7. Отчёт кодами возврата. Коммит не делаю — предложу команды `git add` / `git commit`; доставка через ветку `feature/player-movement`, `/code-review` и PR.

## Замечания (не правлю, только сообщаю)
- Контрактный `GameConstants.h` без перевода строки в конце — косметика.
- На кадре выхода из туннеля круг частично рисуется за краем лабиринта (`x` до `cols`) — ожидаемо для заглушки.
