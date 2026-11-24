==================== TASK FOR AGENT: project/tasks/09-buffs-and-state-machine.md ====================
# Задача 09. Возврат `Eaten`-призрака в дом, модульные тесты автомата и отрисовка `Frightened`/`Eaten`

## Контракты (менять нельзя)
- Файлы `Core/Types.h`, `Core/Command.h`, `Core/GameConstants.h`, `Core/RandomSource.h`, `Core/Ghost.h`, `Core/Game.h`, `Core/Game.cpp`, `Core/GameSnapshot.h`, `Presentation/SpriteAtlas.h` и `Tests/TestHelpers.h` написаны вручную или поставляются курсом — не изменяй их.
- `GhostMode` содержит `Scatter`, `Chase`, `Frightened`, `Eaten`.
- Константы в `Core/GameConstants.h`: `c_frightenedSpeedMultiplier` (`0.5f`), `c_eatenSpeedMultiplier` (`2.0f`), `c_frightenedDuration` (`7.0f`), `c_frightenedFlashTime` (`2.0f`), `c_baseGhostEatScore` (`200`).
- В `Presentation/SpriteAtlas.h` уже готовы `ghostFrightenedFrame(bool flashing)` и `ghostEyesFrame(Core::Direction)`.

## Что нужно сделать агенту
1. Сохрани краткий пошаговый план реализации в файл `tasks/09-buffs-and-state-machine.plan.md` (в `Plan Mode` покажи план на согласование, а сразу после утверждения запиши его в этот файл).
2. Допиши в `src/Game/Core/Ghost.cpp` поведение режимов `Frightened` и `Eaten`:
   - скорость призрака: `c_ghostSpeed * c_frightenedSpeedMultiplier` в `Frightened`, `c_ghostSpeed * c_eatenSpeedMultiplier` в `Eaten`, иначе `c_ghostSpeed`;
   - в `Frightened` вне дома призрак выбирает направление в центре клетки через `chooseRandomDirection(maze, random)`;
   - в `Eaten` призрак движется как `Mover::GhostUsingDoor` в две фазы: сначала к `maze.houseExitTile()`, дойдя до него — включает вход в дом (без запрета разворота) и спускается к `m_homeTile`; достигнув `m_homeTile`, возрождается (`m_inHouse = true`, `m_releaseDelay = 0.0f`, `m_mode = waveMode`) и выходит из дома обычным порядком.
3. Дополни `src/Game/Tests/GhostTest.cpp`:
   - проверь `chooseRandomDirection` с детерминированной заглушкой `IRandomSource` (порядок кандидатов `Up, Left, Down, Right`);
   - проверь, что в `c_classicLevel` призрак в режиме `GhostMode::Eaten` возвращается в дом и возрождается из всех 4 углов карты за ограниченное число шагов (не зацикливается).
4. Дополни `src/Game/Tests/GameTest.cpp`:
   - проверь `toString(GhostMode::Frightened)` и `toString(GhostMode::Eaten)`;
   - проверь включение `Frightened` от `PowerPellet` (разворот призраков в лабиринте, без разворота в доме), паузу волнового таймера на время бафа, истечение бафа с возвратом в текущий режим волны без разворота;
   - проверь поедание `Frightened`-призрака (переход в `Eaten`, комбо `200/400/...`, безопасность `Eaten` для Чомпера и игнорирование повторного `PowerPellet` для `Eaten`).
5. Обнови `src/Game/Presentation/RaylibRenderer.cpp`:
   - для `GhostMode::Frightened` рисуй `Presentation::ghostFrightenedFrame(flashing)` без отдельных глаз, где `flashing` мигает (например, каждые `0.2` с по `snapshot.totalTime`), когда `snapshot.frightenedTimer > 0.0f && snapshot.frightenedTimer < Core::c_frightenedFlashTime`;
   - для `GhostMode::Eaten` рисуй **только глаза** (`Presentation::ghostEyesFrame(ghost.direction)`) без тела;
   - для `Scatter` и `Chase` оставь прежнюю двухслойную отрисовку тела + глаз.

## Ограничения и приёмка
- `GameCore` не должен зависеть от `raylib`.
- Соблюдай `AGENTS.md`; после реализации сам прогони `format_files.py --check`, `run_tests.py`, `run_tidy.py` и `run_asan.py`.
===================================================================================================
