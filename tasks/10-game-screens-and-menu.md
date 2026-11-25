==================== TASK FOR AGENT: project/tasks/10-game-screens-and-menu.md ====================
# Задача 10. `HighScoreStorage`, миграция и тесты автомата экранов, экран `MainMenu` и сохранение рекорда

## Контракты (менять нельзя)
- Файлы `src/Game/Core/*`, `src/Game/Presentation/SpriteAtlas.h` и `src/Game/Tests/TestHelpers.h` написаны вручную или поставляются курсом — не изменяй их.
- `GameState` содержит `Playing`, `Paused`, `GameOver`, `Victory`, `MainMenu`, `Exit`; начальное состояние `Game` — `MainMenu`.
- `Command` содержит `Confirm`.

## Что нужно сделать агенту
1. Сохрани краткий пошаговый план реализации в файл `tasks/10-game-screens-and-menu.plan.md` (в `Plan Mode` покажи план на согласование, а сразу после утверждения запиши его в этот файл).
2. Создай `src/Common/HighScoreStorage.h` и `src/Common/HighScoreStorage.cpp` (подключи в `src/Common/CMakeLists.txt`):
   - в пространстве имён `LifeExe` объяви и реализуй:
     - `int loadHighScore(const std::filesystem::path& path);`
     - `void saveHighScore(const std::filesystem::path& path, int highScore);`
   - все публичные объявления в `HighScoreStorage.h` снабди Doxygen-комментариями (`///`, `@param`, `@return`), потому что `Doxyfile` проверяет `src/Common` с `WARN_AS_ERROR = FAIL_ON_WARNINGS`;
   - используй `nlohmann::json`, инициализируя переменные через `=` (никогда через `{}` — ловушка `std::initializer_list` под Linux);
   - отсутствующий, пустой, повреждённый JSON или отрицательное значение дают `0` и не бросают исключений;
   - `saveHighScore` сохраняет `{"highScore": highScore}` (отрицательные значения зажимает к `0`).
3. Создай `src/Common/Tests/HighScoreStorageTest.cpp` и добавь его в `CommonTests` в `src/Common/CMakeLists.txt`:
   - все временные файлы создавай строго в `std::filesystem::temp_directory_path()` и убирай за собой;
   - проверь: отсутствующий файл -> `0`, сохранение и чтение обратно, повреждённый JSON -> `0`, неверный тип поля -> `0`.
4. Добавь `highscore.json` в `.gitignore`.
5. Обнови `src/Game/Tests/GameTest.cpp`:
   - мигрируй 7 старых тестов `GameCoreTest` под новый контракт:
     - `InitialState` проверяет `GameState::MainMenu` и `totalTime() == 0.0`;
     - `HandlesQuitCommand` проверяет `Playing -> MainMenu` и `MainMenu -> Exit`;
     - `AccumulatesTotalTimeWhilePlaying`, `SnapshotCopiesCoreState`, `HandlesPauseAndResumeCommands`, `HandlesTogglePauseCommand` и `HandlesResetCommand` создают игру через `Tests::makePlayingGame()` и проверяют `totalTime` по разности до и после обновления;
     - дополни `GameStateToString` (`MainMenu`, `Exit`) и `CommandToString` (`Confirm`);
   - добавь тесты на каждую непустую ячейку таблицы переходов экранов и параметризованный тест (`TEST_P`) на игнорируемые команды («—»).
6. Обнови `src/Game/Presentation/RaylibRenderer.cpp`:
   - в `processInput()` привяжи клавишу `KEY_ENTER` к `Core::Command::Confirm`;
   - в `render()` для `GameState::MainMenu` сверстай аркадный экран: заголовок `MAZE CHOMP`, парад всех 4 призраков из `spritesheet.png` с именами `Ruby`, `Rosie`, `Indigo`, `Coral`, таблицу очков (`10 PTS`, `50 PTS`, `200 / 400 / 800 / 1600 PTS`), текущий `HIGH SCORE` и мигающую подсказку `PRESS [ENTER] TO START` (и `[ESC] EXIT`);
   - для `Paused`, `Victory` и `GameOver` рисуй полупрозрачные оверлеи с понятными подсказками клавиш (`[ENTER]` / `[R]` / `[P]` / `[ESC]`).
7. Обнови `src/Game/App/main.cpp`:
   - при старте загружай рекорд через `LifeExe::loadHighScore("highscore.json")` и передавай в `game.setHighScore(savedHighScore)`;
   - условие главного цикла: `while (!renderer.shouldClose() && game.state() != Core::GameState::Exit)`;
   - сохраняй рекорд через `LifeExe::saveHighScore("highscore.json", currentHigh)`, когда он вырос: при переходе в `Victory` или `GameOver`, а также один раз после выхода из цикла (чтобы закрытие окна крестиком тоже не теряло новый рекорд).

## Ограничения и приёмка
- `GameCore` не должен зависеть от `raylib` и файловой системы.
- Соблюдай `AGENTS.md`; после реализации сам прогони `format_files.py --check`, `run_tests.py`, `run_tidy.py`, `run_asan.py` и `build_docs.py`.
===================================================================================================
