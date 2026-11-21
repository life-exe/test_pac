==================== TASK FOR AGENT: project/tasks/02-maze-and-level.md ====================
# Задача 02. Интеграция лабиринта `Maze` в `Game`, тесты и отрисовка

## Контракты (менять нельзя)
- Файлы `Core/Types.h`, `Core/Level.h`, `Core/Maze.h`, `Core/Maze.cpp` и `Tests/TestHelpers.h` уже написаны — не изменяй их.
- В классе `LifeExe::Core::Game` объяви два конструктора:
  1) `explicit Game(Maze maze);` — принимает готовый `Maze` по значению и перемещает в поле `m_maze`;
  2) `Game();` — делегирует в `Game(Maze::fromAscii(c_classicLevel).value())`.
  Класс `Game` должен оставаться перемещаемым (move-constructible и move-assignable).
- В структуру `LifeExe::Core::GameSnapshot` добавь поля:
  - `int mazeRows{0};`
  - `int mazeCols{0};`
  - `std::span<const TileType> tiles;`
  Заполняй их в `Game::snapshot()` из `m_maze`. В комментарии над `GameSnapshot` укажи время жизни `span`: срез `tiles` остаётся валидным до следующего вызова `update`, `handleCommand` или `reset`.

## Что нужно сделать агенту
1. Сохрани краткий пошаговый план реализации в файл `tasks/02-maze-and-level.plan.md` (в `Plan Mode` покажи план на согласование, а сразу после утверждения запиши его в этот файл).
2. Реализуй обновления в `Core/GameSnapshot.h`, `Core/Game.h` и `Core/Game.cpp`.
3. Напиши модульные тесты в `src/Game/Tests/MazeTest.cpp` (GoogleTest) и подключи файл к цели `GameTests` в `src/Game/CMakeLists.txt` (также добавь `target_include_directories(GameTests PRIVATE Tests)` для `TestHelpers.h`). Покрой сценарии:
   - размеры сетки и общая длина `tiles()`;
   - внешний периметр закрыт стенами везде, кроме клеток туннеля `T`;
   - `isWalkable` для `Player`, `Ghost` и `GhostUsingDoor` (особенно на клетке `GhostDoor`);
   - координаты спавнов `P` и `1..4` и то, что сами эти клетки имеют тип `TileType::Empty`;
   - любая координата за границей карты возвращает `TileType::Wall`;
   - отдельные тесты на каждое из 8 значений `MazeError` (для `DuplicateSpawn` проверь и дубликат `P`, и дубликат цифры призрака).
4. В `RaylibRenderer::render()` рассчитай целочисленный масштаб тайла (`std::max(1, ...)`) с запасом по 48 пикселей сверху и снизу под будущий HUD, отцентрируй лабиринт в окне и отрисуй стены неоново-синим цветом со скруглениями, а дверь дома призраков (`GhostDoor`) — горизонтальной розовой перекладиной.

## Ограничения и приёмка
- `GameCore` не должен зависеть от `raylib`.
- Соблюдай `AGENTS.md`; после реализации сам прогони сборку, тесты, `format_files.py --check`, `run_tidy.py` и `run_asan.py`.
============================================================================================
