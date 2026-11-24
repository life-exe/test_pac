==================== TASK FOR AGENT: project/tasks/08-collisions-and-lives.md ====================
# Задача 08. Тесты фаз раунда и коллизий, отображение жизней/смерти в `RaylibRenderer` и цикл `main.cpp`

## Контракты (менять нельзя)
- Файлы `Core/*`, `Presentation/SpriteAtlas.h` и `Tests/TestHelpers.h` написаны вручную или поставляются курсом — не изменяй их.
- В `Core/Types.h` добавлены `RoundPhase { Ready, Active, PlayerDying }` и `toString(RoundPhase)`.
- В `Core/GameConstants.h` добавлены `c_startingLives` (`3`), `c_readyDuration` (`1.5f`), `c_dyingDuration` (`1.0f`).
- В `Core/GameSnapshot.h` добавлены поля `lives`, `roundPhase`, `phaseTimeLeft`.
- В `Presentation/SpriteAtlas.h` уже готовы `playerDeathFrame(int)` (4 кадра `0..3`) и `lifeIconSprite()`.

## Что нужно сделать агенту
1. Сохрани краткий пошаговый план реализации в файл `tasks/08-collisions-and-lives.plan.md` (в `Plan Mode` покажи план на согласование, а сразу после утверждения запиши его в этот файл).
2. Обнови `src/Game/Tests/GameTest.cpp` (и при необходимости `src/Game/Tests/GhostTest.cpp`):
   - адаптируй прежние модульные тесты из спринтов 06–07 под новые правила (после `Command::Reset` проматывай паузу `Ready` до `RoundPhase::Active`, двигай время относительными шагами `update(dt)` вместо абсолютного `totalTime`, а в длинных проверках волн и выхода из дома используй карту, где Чомпер отгорожен стеной от призраков);
   - добавь в `GameTest.cpp` модульные тесты:
   - `toString(RoundPhase)` для `Ready`, `Active`, `PlayerDying`;
   - стартовая фаза `Ready` блокирует движение Чомпера и призраков на `c_readyDuration`, при этом `totalTime` растёт;
   - столкновение Чомпера с призраком сразу уменьшает `lives`, переводит в `RoundPhase::PlayerDying` и замораживает сущности;
   - после завершения `PlayerDying` при `lives > 0` сущности возвращаются на спавны в `Ready`, а уже съеденные точки и счёт сохраняются;
   - потеря последней (3-й) жизни после `PlayerDying` переводит игру в `GameState::GameOver`.
3. Обнови `src/Game/Presentation/RaylibRenderer.cpp`:
   - в нижней панели HUD рисуй `snapshot.lives` значков жизней из `spritesheet.png` с помощью `Presentation::lifeIconSprite()`;
   - когда `snapshot.state == GameState::Playing && snapshot.roundPhase == RoundPhase::Ready`, рисуй надпись `READY!` жёлтым цветом под домом призраков (или по центру лабиринта);
   - когда `snapshot.roundPhase == RoundPhase::PlayerDying`, рисуй Чомпера через `Presentation::playerDeathFrame(frameIndex)`, вычисляя `frameIndex` из прогресса фазы `1.0f - (snapshot.phaseTimeLeft / Core::c_dyingDuration)` (от `0` до `3`), призраков на время схлопывания можно скрыть или оставить неподвижными;
   - когда `snapshot.state == GameState::GameOver`, рисуй полупрозрачный оверлей `GAME OVER` с подсказкой `PRESS [R] TO RESTART`.
4. Обнови `src/Game/App/main.cpp`:
   - убери из условия `while` проверку `game.state() != Core::GameState::GameOver` — цикл должен крутиться, пока `!renderer.shouldClose()`, иначе окно закроется в момент потери последней жизни и экран `GAME OVER` никто не увидит (клавиша `R` по-прежнему отправляет `Command::Reset`, а `Esc` — `Command::Quit`, это временно до спринта 10).

## Ограничения и приёмка
- `GameCore` не должен зависеть от `raylib`.
- Соблюдай `AGENTS.md`; после реализации сам прогони `format_files.py --check`, `run_tests.py`, `run_tidy.py` и `run_asan.py`.
===================================================================================================