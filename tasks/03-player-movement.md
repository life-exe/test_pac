==================== TASK FOR AGENT: project/tasks/03-player-movement.md ====================
# Задача 03. Интеграция `Player` в `Game`, модульные тесты движения и управление

## Контракты (менять нельзя)
- Файлы `Core/Types.h`, `Core/Command.h`, `Core/GameConstants.h`, `Core/Maze.*`, `Core/GridMover.*` и `Core/Player.*` написаны вручную — не изменяй их.
- В `Core/GameSnapshot.h` объяви структуру `struct PlayerSnapshot { Vec2f position; Direction direction{Direction::None}; Direction facing{Direction::Left}; bool isMoving{false}; };` и добавь поле `PlayerSnapshot player;` в `GameSnapshot`.
- В `Core::Game` храни поле `Player m_player`, инициализируемое клеткой `m_maze.playerSpawn()`:
  - в `Game::update(float dt)` обновляй `m_player.update(m_maze, dt)` только в состоянии `GameState::Playing`;
  - в `Game::handleCommand(Command)` обрабатывай `MoveUp`, `MoveDown`, `MoveLeft`, `MoveRight` вызовом `m_player.queueDirection(...)` только при `m_state == GameState::Playing` (в `Paused` и `GameOver` команды движения игнорируются);
  - в `Game::reset()` возвращай `m_player` на стартовую клетку `m_maze.playerSpawn()`.

## Что нужно сделать агенту
1. Сохрани краткий пошаговый план реализации в файл `tasks/03-player-movement.plan.md` (в `Plan Mode` покажи план на согласование, а сразу после утверждения запиши его в этот файл).
2. Обнови `Core/GameSnapshot.h`, `Core/Game.h` и `Core/Game.cpp` по контракту выше.
3. Создай файл `src/Game/Tests/PlayerTest.cpp` (GoogleTest, создавай игру через `Tests::makePlayingGame()` из `TestHelpers.h`) и подключи его к цели `GameTests` в `src/Game/CMakeLists.txt`. Покрой сценарии:
   - движение по прямому коридору;
   - остановка ровно в центре клетки перед стеной с сохранением `facing`;
   - буферизация поворота (нажатие поворота заранее и срабатывание на первом открытом перекрёстке);
   - мгновенный разворот на 180° посреди клетки без скачка `position`;
   - проход через боковой туннель на противоположную сторону строки;
   - большой шаг `dt`, проходящий несколько клеток подряд с поворотом по пути;
   - игнорирование команд `Move*` на паузе.
4. В `RaylibRenderer::processInput()` добавь опрос клавиш `WASD` (`KEY_W`, `KEY_S`, `KEY_A`, `KEY_D`) и стрелок (`KEY_UP`, `KEY_DOWN`, `KEY_LEFT`, `KEY_RIGHT`) через `IsKeyPressed` с отправкой соответствующих команд `MoveUp`/`MoveDown`/`MoveLeft`/`MoveRight`. В `RaylibRenderer::render()` временно рисуй Чомпера жёлтым кругом в точке `snapshot.player.position` (переведи координаты из тайлов в экранные пиксели с учётом масштаба и смещения лабиринта; в следующем спринте заменим круг на анимированный спрайт).

## Ограничения и приёмка
- `GameCore` не должен зависеть от `raylib`.
- Соблюдай `AGENTS.md`; после реализации сам прогони сборку, тесты, `format_files.py --check`, `run_tidy.py` и `run_asan.py`.
=============================================================================================