==================== TASK FOR AGENT: project/tasks/05-pellets-score-and-hud.md ====================
# Задача 05. Тесты сбора точек, очков и `GameState::Victory`, отрисовка точек и HUD в `RaylibRenderer`

## Контракты (менять нельзя)
- Все файлы `src/Game/Core/*` (`Types.h`, `Command.h`, `GameConstants.h`, `Maze.*`, `GridMover.*`, `Player.*`, `GameSnapshot.h`, `Game.*`) и `src/Game/Presentation/SpriteAtlas.h` написаны вручную — не изменяй их.
- В `Core::Maze` добавлены `int remainingPellets() const` и `TileType consumeTileAt(GridPos pos)`.
- В `Core::Game` добавлены:
  - деление шага `update(float dt)` на подшаги не длиннее `c_maxSubstep` (`1.0f / 64.0f`);
  - начисление `+c_pelletScore` (`10`) за `TileType::Pellet` и `+c_powerPelletScore` (`50`) за `TileType::PowerPellet`;
  - обновление `highScore`, метод `void setHighScore(int highScore)` и сохранение `highScore` при `reset()`;
  - переход в `GameState::Victory` в момент, когда съедена последняя точка и `remainingPellets()` стал равен `0`.
- В `Core::GameSnapshot` добавлены поля `int score{0};`, `int highScore{0};` и `int remainingPellets{0};`.

## Что нужно сделать агенту
1. Сохрани краткий пошаговый план реализации в файл `tasks/05-pellets-score-and-hud.plan.md` (в `Plan Mode` покажи план на согласование, а сразу после утверждения запиши его в этот файл).
2. Дополни `src/Game/Tests/MazeTest.cpp` модульными тестами учёта и сбора точек:
   - подсчёт `remainingPellets()` при разборе карты `Maze::fromAscii`;
   - `consumeTileAt(pos)` для `Pellet` и `PowerPellet` (замена клетки на `TileType::Empty`, уменьшение `remainingPellets()` и возврат прежнего типа);
   - повторный вызов `consumeTileAt` на уже очищенной клетке, вызов на стене и за пределами карты (возвращает `TileType::Empty`, счётчик не меняется).
3. Дополни `src/Game/Tests/GameTest.cpp` модульными тестами очков, рекорда и победы:
   - обнови тест `GameStateToString` проверкой `Core::toString(Core::GameState::Victory) == "Victory"`;
   - проверь начисление `+c_pelletScore` и `+c_powerPelletScore` и рост `highScore`;
   - проверь `setHighScore(int)` (не уменьшает уже набранный больший рекорд) и то, что `Reset` сбрасывает `score` и восстанавливает все точки лабиринта (`remainingPellets`), но сохраняет `highScore`;
   - проверь переход в `GameState::Victory` при сборе последней точки и то, что один большой шаг `dt` (через несколько клеток с точками) съедает все промежуточные точки благодаря подшагам `c_maxSubstep`.
4. Обнови `src/Game/Presentation/RaylibRenderer.cpp`:
   - в `drawMaze` рисуй `TileType::Pellet` кадром `SpriteAtlas::pelletSprite()` и `TileType::PowerPellet` кадром `SpriteAtlas::powerPelletSprite()` из `m_spritesheet` (для энерджайзера добавь мягкую пульсацию или мигание по `snapshot.totalTime`);
   - в верхней полосе HUD выводи `SCORE` (`snapshot.score`) и `HIGH SCORE` (`snapshot.highScore`);
   - в нижней полосе HUD выводи остаток точек (`snapshot.remainingPellets`);
   - в `drawStateOverlay` для `Core::GameState::Victory` рисуй полупрозрачный оверлей с текстом `VICTORY!` (и подсказкой нажать `[R]` для перезапуска).

## Ограничения и приёмка
- `GameCore` не должен зависеть от `raylib`.
- Соблюдай `AGENTS.md`; после реализации сам прогони `format_files.py --check`, `run_tests.py`, `run_tidy.py` и `run_asan.py`.
===================================================================================================
