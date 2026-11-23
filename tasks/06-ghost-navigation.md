==================== TASK FOR AGENT: project/tasks/06-ghost-navigation.md ====================
# Задача 06. Подключение четырёх призраков в `Game`, модульные тесты `GhostTest.cpp` и двухслойная отрисовка в `RaylibRenderer`

## Контракты (менять нельзя)
- Файлы `Core/Types.h`, `Core/Command.h`, `Core/GameConstants.h`, `Core/Level.h`, `Core/Maze.*`, `Core/GridMover.*`, `Core/Player.*`, `Core/Ghost.*`, `Core/GameSnapshot.h` и `Presentation/SpriteAtlas.h` написаны вручную — не изменяй их.
- В `Core/GameConstants.h` заданы `c_ghostSpeed`, `c_rosieReleaseDelay`, `c_indigoReleaseDelay`, `c_coralReleaseDelay`.
- В `Core/Ghost.h` объявлены `GhostConfig` и класс `Ghost` с методами `update(maze, dt)`, `updateToward(maze, dt, targetTile)` и `chooseNextDirection(maze, targetTile)`.
- В `Core/GameSnapshot.h` объявлены `GhostSnapshot { GhostId id; Vec2f position; Direction direction; }` и поле `std::array<GhostSnapshot, 4> ghosts;` в `GameSnapshot`.

## Что нужно сделать агенту
1. Сохрани краткий пошаговый план реализации в файл `tasks/06-ghost-navigation.plan.md` (в `Plan Mode` покажи план на согласование, а сразу после утверждения запиши его в этот файл).
2. Подключи 4 призраков в `src/Game/Core/Game.h` и `src/Game/Core/Game.cpp`:
   - храни `std::array<Ghost, 4> m_ghosts` в порядке `Ruby` (индекс 0), `Rosie` (1), `Indigo` (2), `Coral` (3);
   - инициализируй их из `m_maze.ghostSpawn(id)` и `m_maze.scatterCorner(id)`:
     - `Ruby`: `inHouse = false`, `releaseDelay = 0.0f`;
     - `Rosie`: `inHouse = true`, `releaseDelay = c_rosieReleaseDelay`;
     - `Indigo`: `inHouse = true`, `releaseDelay = c_indigoReleaseDelay`;
     - `Coral`: `inHouse = true`, `releaseDelay = c_coralReleaseDelay`;
   - в `Game::updateSubstep(float stepDt)` обновляй всех призраков `ghost.update(m_maze, stepDt)` сразу после `m_player.update(m_maze, stepDt)` и до `consumePelletAtPlayer()`;
   - в `Game::reset()` пересоздавай `m_ghosts` в исходное состояние (позиции спавнов, `inHouse` и исходные таймеры выхода);
   - в `Game::snapshot()` заполняй массив `ghosts` (`id`, `position`, `direction`).
3. Создай `src/Game/Tests/GhostTest.cpp` и добавь его в `GameTests` в `src/Game/CMakeLists.txt`. Покрой модульными тестами:
   - поочерёдный выход из дома через дверь `=` по таймеру `releaseDelay` и сброс `isInHouse()` на `houseExitTile()`;
   - запрет разворота на 180° в коридоре;
   - разворот в тупике, когда других выходов нет;
   - выбор кратчайшего поворота к `targetTile` на Т-образном перекрёстке;
   - приоритет `Up > Left > Down > Right` при равной дистанции;
   - призрак в лабиринте не заходит в дверь дома (`=`);
   - прохождение туннеля без разворота.
4. Обнови `src/Game/Presentation/RaylibRenderer.cpp`:
   - рисуй каждого призрака из `snapshot.ghosts` в два слоя из `m_spritesheet` через `DrawTexturePro`:
     - нижний слой — тело `Presentation::ghostBodyFrame(phase)` с переключением двух фаз юбки по `snapshot.totalTime` (например, 6 раз в секунду) и окраской `tint` по `GhostId`: `Ruby` → `RED`, `Rosie` → `PINK`, `Indigo` → `SKYBLUE`, `Coral` → `ORANGE`;
     - верхний слой — глаза `Presentation::ghostEyesFrame(ghost.direction)` с `WHITE` (без окраски).

## Ограничения и приёмка
- `GameCore` не должен зависеть от `raylib`.
- Соблюдай `AGENTS.md`; после реализации сам прогони `format_files.py --check`, `run_tests.py`, `run_tidy.py` и `run_asan.py`.
===================================================================================================
