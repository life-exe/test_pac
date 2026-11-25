# План: задача 10. `HighScoreStorage`, миграция тестов автомата экранов, `MainMenu` и сохранение рекорда

## Контекст
Контракт уже на месте: в `Types.h` есть `MainMenu` и `Exit`, в `Command.h` есть `Confirm`. Таблица переходов уже реализована в `Game::handleCommand`, а `TestHelpers.h` нажимает `Confirm` внутри `makeReadyGame`/`makePlayingGame`. Эти файлы не трогаю.

**Что ещё сломано и не перечислено в задании:**
1. `PlayerTest.MoveCommandsIgnoredAfterGameOver` ждёт, что после `Quit` будет `GameOver`. Теперь там `MainMenu`, поэтому тест упадёт. Переименовываю его в `MoveCommandsIgnoredAfterQuit` и проверяю `MainMenu`. Смысл теста сохраняется: ввод вне `Playing` игнорируется.
2. В `RaylibRenderer.cpp` в `switch` функции `drawStateOverlay` нет веток `MainMenu`/`Exit`. С `/W4 /WX` (C4062) и `-Wswitch` это ошибка сборки.

## Шаги

### 1. Сохранить план
Сразу после утверждения записать его в `tasks/10-game-screens-and-menu.plan.md`.

### 2. `src/Common/HighScoreStorage.h/.cpp` + CMake
- Объявления в `namespace LifeExe`, у каждого Doxygen-комментарий (`///`, `@param`, `@return`). Само пространство имён повторно не документирую: оно уже описано в `Config.h`.
- `loadHighScore(path)`:
  - `std::ifstream`; если файл не открылся, возвращаю `0`;
  - разбор через `const auto document = nlohmann::json::parse(file, nullptr, false);`, без исключений;
  - `0` получается, если: документ `is_discarded()` (сюда попадают пустой и повреждённый файл), это не объект, нет ключа `highScore`, значение не `is_number_integer()` (строка или дробное число), значение отрицательное;
  - значение больше `INT_MAX` зажимаю до `INT_MAX`.
- `saveHighScore(path, highScore)`: пишу `{"highScore": std::max(0, highScore)}`. Если файл не открылся, просто выхожу без исключения: потерянный рекорд не должен ронять игру. Это поведение описано и в Doxygen.
- `src/Common/CMakeLists.txt`: добавляю `HighScoreStorage.cpp/.h` в `Common`, а `Tests/HighScoreStorageTest.cpp` — в `CommonTests`.

### 3. `src/Common/Tests/HighScoreStorageTest.cpp`
Фикстура `HighScoreStorageTest`. Путь `temp_directory_path() / "maze_highscore_<имя теста>.json"`: имя теста в пути нужно, потому что CTest может гонять тесты параллельно. `TearDown` удаляет файл через `std::filesystem::remove(path, ec)`.

| TEST | Ожидание |
|---|---|
| `MissingFileReturnsZero` | файла нет → `0` |
| `SaveThenLoadRoundTrips` | `save(1234)` → `load == 1234`, повторный `save(50)` перезаписывает |
| `SaveWritesHighScoreObject` | файл разбирается в `{"highScore": 1234}` |
| `SaveClampsNegativeToZero` | `save(-5)` → `load == 0` |
| `EmptyFileReturnsZero` | пустой файл → `0` |
| `CorruptedJsonReturnsZero` | `{"highScore": 12` → `0` |
| `WrongFieldTypeReturnsZero` | `{"highScore": "100"}` → `0` |
| `NegativeValueReturnsZero` | `{"highScore": -7}` → `0` |

### 4. `.gitignore`
Добавляю строку `highscore.json`.

### 5. `src/Game/Tests/GameTest.cpp`
**Миграция 7 тестов `GameCoreTest`:**
- `InitialState`: `Core::Game{}` → `MainMenu`, `totalTime() == 0.0`.
- `AccumulatesTotalTimeWhilePlaying`, `SnapshotCopiesCoreState`, `HandlesPauseAndResumeCommands`, `HandlesTogglePauseCommand`, `HandlesResetCommand`: создаю игру через `Tests::makePlayingGame()`, `totalTime` сравниваю по разности «до/после». Проход Ready уже накопил время.
  - `HandlesTogglePauseCommand`, хвост теста: `Quit` → `MainMenu`, `TogglePause` в меню игнорируется.
  - `HandlesResetCommand`: после `Reset` проверяю, что `totalTime() == 0.0`, потому что `reset()` сам обнуляет время. Затем снова проверяю по разности после `update`.
- `HandlesQuitCommand`: `Playing → MainMenu → Exit`.
- `GameStateToString` дополняю `MainMenu` и `Exit`, `CommandToString` — `Confirm`.

**Хелперы** (анонимное пространство имён, без `EXPECT` в циклах):
- `makeGameIn(GameState)`. `MainMenu` — свежая игра. `Playing`/`Paused` — `c_corridorLevel` с одной съеденной точкой (счёт `10`), чтобы новую игру было видно. `Victory` — съесть все 3 точки коридора. `GameOver` — три раза `loseLife` на `c_collisionLevel`. `Exit` — `MainMenu` + `Quit`.
- `expectNewGame(game, initialPellets)`: `Playing`, `RoundPhase::Ready`, `totalTime == 0`, `score == 0`, `lives == c_startingLives`, все точки на месте.

**Непустые ячейки таблицы (13 TEST, набор `GameScreenTest`):**

| TEST | Переход |
|---|---|
| `MainMenuConfirmStartsNewGame` | `MainMenu` + `Confirm` → новая игра; рекорд из `setHighScore` сохраняется |
| `MainMenuQuitExits` | → `Exit` |
| `PlayingTogglePausePauses` / `PlayingPausePauses` | → `Paused` |
| `PlayingResetStartsNewGame` | → новая игра |
| `PlayingQuitReturnsToMainMenu` | → `MainMenu` |
| `PausedTogglePauseResumes` / `PausedResumeResumes` | → `Playing` |
| `PausedResetStartsNewGame` | → новая игра |
| `PausedQuitReturnsToMainMenu` | → `MainMenu` |
| `EndScreenConfirmStartsNewGame` | `Victory` и `GameOver` + `Confirm` → новая игра |
| `EndScreenResetStartsNewGame` | `Victory` и `GameOver` + `Reset` → новая игра |
| `EndScreenQuitReturnsToMainMenu` | `Victory` и `GameOver` + `Quit` → `MainMenu` |

**Ячейки «—»: `TEST_P(GameIgnoredCommandTest, CommandChangesNothing)`.**
- Параметр — `{GameState from, Command command}`, 20 случаев:
  - `MainMenu`: TogglePause, Pause, Resume, Reset;
  - `Playing`: Confirm, Resume;
  - `Paused`: Confirm, Pause;
  - `Victory`/`GameOver`: TogglePause, Pause, Resume;
  - `Exit`: все 6.
- Проверка: `state`, `score`, `totalTime`, `roundPhase`, `lives`, `remainingPellets` не изменились.
- Имена случаев генерирую из `toString` (`Victory_Pause`), для понятных сообщений об ошибке есть `PrintTo`.

### 6. `src/Game/Tests/PlayerTest.cpp`
Миграция теста из пункта 1 «Контекста».

### 7. `src/Game/Presentation/RaylibRenderer.cpp`
- `processInput()`: `KEY_ENTER` → `Command::Confirm`.
- `render()`:
  - в `MainMenu` рисую только `drawMainMenu(snapshot, screen, spritesheet)`, без лабиринта и HUD;
  - в `Exit` ничего не рисую, цикл всё равно завершается;
  - в остальных состояниях всё как раньше, плюс оверлеи.
- `drawMainMenu`, разбитый на мелкие функции, чтобы не упереться в cognitive complexity:
  - заголовок `MAZE CHOMP`;
  - парад 4 призраков: тело с оттенком (`ghostBodyFrame` + `ghostTint`) и глаза, подписи `Ruby`, `Rosie`, `Indigo`, `Coral` их цветом;
  - таблица очков: спрайт точки `10 PTS`, спрайт энерджайзера `50 PTS`, `ghostFrightenedFrame` + `200 / 400 / 800 / 1600 PTS`;
  - `HIGH SCORE: N` из `snapshot.highScore`;
  - мигающая строка `PRESS [ENTER] TO START` и под ней `[ESC] EXIT`.
- **Развилка:** в `MainMenu` `update()` не идёт, и `snapshot.totalTime` стоит на месте. Поэтому мигание и анимацию юбки в меню считаю от `GetTime()` (часы окна). В игре анимации по-прежнему идут от `totalTime`, чтобы замирать на паузе. Комментарий *why* оставлю в коде.
- Оверлеи (`drawStateOverlay` с полным `switch`):
  - `Paused`: полупрозрачная плашка `PAUSED` и строка `[P] RESUME  [R] RESTART  [ESC] MENU`;
  - `Victory`/`GameOver`: затемнение всего экрана, заголовок и строки `[ENTER] / [R] PLAY AGAIN` и `[ESC] MAIN MENU`;
  - `Playing`/`MainMenu`/`Exit`: без оверлея.
- Строка управления в нижнем HUD: `[ESC] Quit` → `[ESC] Menu`.

### 8. `src/Game/App/main.cpp`
- `constexpr const char* c_highScorePath{"highscore.json"}`. Путь относительный, рабочий каталог — корень проекта (`VS_DEBUGGER_WORKING_DIRECTORY`).
- `int savedHighScore{LifeExe::loadHighScore(c_highScorePath)}` → `game.setHighScore(savedHighScore)`.
- Цикл: `while (!renderer.shouldClose() && game.state() != Core::GameState::Exit)`. Старый комментарий про `GameOver` заменяю на актуальный.
- Вспомогательная функция `saveHighScoreIfImproved(int currentHigh, int& savedHighScore)`: сохраняет, только если `currentHigh > savedHighScore`.
- После `update`, если состояние сменилось на `Victory`/`GameOver` (сравниваю с предыдущим кадром), вызываю функцию.
- Один раз после выхода из цикла тоже вызываю: так рекорд не потеряется при закрытии окна крестиком или выходе через `Esc`.
- `#include "HighScoreStorage.h"`: `GameApp` уже линкует `Common`. `GameCore` не получает ни новых зависимостей, ни работы с файлами.

### 9. Форматирование и проверки
- `format_files.py`, затем `format_files.py --check`, `run_tests.py`, `run_tidy.py`, `run_asan.py`, `build_docs.py`;
- по каждому скрипту отчитываюсь кодом возврата;
- если упрётся в лимит, тайминги или карты подправляю, но проверки не ослабляю.

## Проверка
1. Коды возврата всех скриптов.
2. Меню, оверлеи и запись `highscore.json` в реальном окне тесты не покрывают. `GameApp` запускается только по просьбе.
3. Коммит не делаю. Предлагаю команды `git add`/`git commit`, затем `/code-review` и PR из `feature/game-screens-and-menu`.
4. Приёмочный тест курса `GameScreensAndMenuAcceptanceTest.cpp` не добавляю: это следующий шаг 4 спринта.
