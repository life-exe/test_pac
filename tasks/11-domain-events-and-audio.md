==================== TASK FOR AGENT: project/tasks/11-domain-events-and-audio.md ====================
# Задача 11. Генерация WAV-звуков, RAII `SoundSystem`, подключение в `main.cpp` и тесты событий

## Контракты (менять нельзя)
- Файлы `src/Game/Core/*`, `src/Game/Presentation/SpriteAtlas.h` и `src/Game/Tests/TestHelpers.h` написаны вручную или поставляются курсом — не изменяй их.
- В `Core/GameEvents.h` объявлено `enum class GameEvent { PelletEaten, PowerPelletEaten, GhostEaten, PlayerLostLife, RoundStarted, Victory, GameOver }`.
- В `Core/Game.h` объявлен метод `std::vector<GameEvent> takeEvents();`.

## Что нужно сделать агенту
1. Сохрани краткий пошаговый план реализации в файл `tasks/11-domain-events-and-audio.plan.md` (в `Plan Mode` покажи план на согласование, а сразу после утверждения запиши его в этот файл).
2. Расширь `automation/generate_assets.py` генерацией 5 классических 8-битных звуков (PCM WAV, только стандартные модули `wave`, `struct`, `math`, без pip-зависимостей) в `assets/sounds/`:
   - `assets/sounds/chomp.wav` — короткий аркадный «вака» при съедении точки (~0.10 с);
   - `assets/sounds/power_pellet.wav` — восходящий импульс энерджайзера (~0.25 с);
   - `assets/sounds/eat_ghost.wav` — яркий высокий звук поедания призрака (~0.25 с);
   - `assets/sounds/death.wav` — нисходящая сирена потери жизни (~0.70 с);
   - `assets/sounds/victory.wav` — победная аркадная фанфара (~0.70 с).
   Запусти `python automation/generate_assets.py` и оставь сгенерированные WAV в репозитории (в Git).
3. Создай `src/Game/Presentation/SoundSystem.h` и `src/Game/Presentation/SoundSystem.cpp` (подключи их к `GamePresentation` в `src/Game/CMakeLists.txt`):
   - RAII-класс `LifeExe::Presentation::SoundSystem` (некопируемый):
     - конструктор вызывает `InitAudioDevice()`; если `IsAudioDeviceReady()` вернул `false` (например, в WSL без звука), система запоминает `m_ready = false` и работает вхолостую без ошибок;
     - при `m_ready == true` загружает все 5 звуков из `assets/sounds/*.wav` через `LoadSound(...)`;
     - деструктор выгружает загруженные звуки (`UnloadSound`) и вызывает `CloseAudioDevice()`, если устройство было открыто;
     - метод `void processEvents(const std::vector<Core::GameEvent>& events)`:
       - `PelletEaten` -> `chomp.wav`, но только если `!IsSoundPlaying(m_chomp)` (чтобы частые точки не давали треск);
       - `PowerPelletEaten` -> `power_pellet.wav`;
       - `GhostEaten` -> `eat_ghost.wav`;
       - `PlayerLostLife` -> `death.wav`;
       - `Victory` -> `victory.wav`;
       - `GameOver` и `RoundStarted` -> без звука (`death.wav` уже звучит по `PlayerLostLife`).
4. Подключи `Presentation::SoundSystem` в `src/Game/App/main.cpp`:
   - создай `Presentation::SoundSystem soundSystem;` после `renderer.init(...)`;
   - вызывай `soundSystem.processEvents(game.takeEvents());` один раз за кадр сразу после цикла `while (accumulator >= fixedDt)`.
5. Дополни `src/Game/Tests/GameTest.cpp` модульными тестами событий:
   - `toString(GameEvent)` для всех 7 значений;
   - `RoundStarted` при переходе `Ready -> Active` и очистка очереди после `takeEvents()`;
   - два `update` подряд без `takeEvents()` между ними — события обоих шагов на месте;
   - `PelletEaten`, `PowerPelletEaten`, `GhostEaten`, `PlayerLostLife` (сразу при столкновении), `Victory` и `GameOver` (после таймера смерти);
   - `Reset` очищает невычитанные события.

## Ограничения и приёмка
- `GameCore` не должен зависеть от `raylib` и аудио.
- Соблюдай `AGENTS.md`; после реализации сам прогони `format_files.py --check`, `run_tests.py`, `run_tidy.py` и `run_asan.py`.
===================================================================================================
