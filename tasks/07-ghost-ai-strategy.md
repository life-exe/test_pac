==================== TASK FOR AGENT: project/tasks/07-ghost-ai-strategy.md ====================
# Задача 07. Реализация стратегий `Rosie`, `Indigo`, `Coral`, `GhostStrategyTest.cpp` и тесты волн `Scatter`/`Chase`

## Контракты (менять нельзя)
- Файлы `Core/Types.h`, `Core/Command.h`, `Core/GameConstants.h`, `Core/Level.h`, `Core/Maze.*`, `Core/GridMover.*`, `Core/Player.*`, `Core/Ghost.*`, `Core/GhostStrategy.h`, `Core/GameSnapshot.h` и `Presentation/SpriteAtlas.h` написаны вручную — не изменяй их.
- В `Core/GhostStrategy.h` объявлены `GhostContext`, интерфейс `IGhostStrategy` и классы `RubyStrategy`, `RosieStrategy`, `IndigoStrategy`, `CoralStrategy`.
- В `Core/GhostStrategy.cpp` уже реализована `RubyStrategy::targetTile`.
- В `Core/Game.*` уже реализован волновой таймер (`c_scatterDuration`, `c_chaseDuration`) и передача `GhostContext`.

## Что нужно сделать агенту
1. Сохрани краткий пошаговый план реализации в файл `tasks/07-ghost-ai-strategy.plan.md` (в `Plan Mode` покажи план на согласование, а сразу после утверждения запиши его в этот файл).
2. Реализуй три стратегии в `src/Game/Core/GhostStrategy.cpp` по точной спецификации:
   - `RosieStrategy::targetTile(context)` — на 4 клетки впереди Чомпера: `context.playerPos + (4 * directionDelta(context.playerDir))` (при `Direction::None` смещение `{0, 0}`, исторический баг переполнения при `Direction::Up` не воспроизводится);
   - `IndigoStrategy::targetTile(context)` — точка опоры `pivot = context.playerPos + (2 * directionDelta(context.playerDir))`, затем удвоение вектора от `rubyPos` к `pivot`: `context.rubyPos + (2 * (pivot - context.rubyPos))`;
   - `CoralStrategy::targetTile(context)` — квадрат расстояния от `context.ghostPos` до `context.playerPos`: если строго больше `64` (`> 8` клеток) — вернуть `context.playerPos`, иначе (включая ровно `64`) — вернуть `context.scatterCorner`.
3. В `src/Game/Core/Game.cpp` в `Game::createGhosts` подключи все 4 стратегии: `RubyStrategy`, `RosieStrategy`, `IndigoStrategy`, `CoralStrategy` (не меняя остальную логику `Game.cpp`).
4. Создай `src/Game/Tests/GhostStrategyTest.cpp` и добавь его в `GameTests` в `src/Game/CMakeLists.txt`. Покрой все 4 стратегии изолированными тестами (100% покрытие `GhostStrategy.cpp`):
   - `RubyStrategy`: прямое преследование при разных позициях;
   - `RosieStrategy`: все 4 направления (`Up`, `Down`, `Left`, `Right`) и `Direction::None`, включая цель за границами карты;
   - `IndigoStrategy`: расчёт по паре `playerPos` + `rubyPos` при разных направлениях, при `Direction::None` и с выходом за границы карты;
   - `CoralStrategy`: дистанция больше 8 клеток (`distSq = 65`), точная граница (`distSq = 64` → `scatterCorner`) и ближняя дистанция (`distSq < 64`).
5. Дополни `src/Game/Tests/GameTest.cpp`:
   - проверь `Core::toString(Core::GhostMode::Scatter)` и `Core::toString(Core::GhostMode::Chase)`;
   - проверь переключение волн `Scatter` (`c_scatterDuration`) → `Chase` (`c_chaseDuration`) → `Scatter` и сброс волны в `Scatter` при `Command::Reset`.

## Ограничения и приёмка
- `GameCore` не должен зависеть от `raylib`.
- Соблюдай `AGENTS.md`; после реализации сам прогони `format_files.py --check`, `run_tests.py`, `run_tidy.py` и `run_asan.py`.
===================================================================================================
