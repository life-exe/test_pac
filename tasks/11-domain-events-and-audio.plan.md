# План: задача 11. WAV-звуки, RAII `SoundSystem`, подключение в `main.cpp` и тесты событий

## Контекст
- Контракт уже на месте. В `GameEvents.h` есть `enum class GameEvent` и `toString`. `Game::takeEvents()` отдаёт очередь через `swap`. События пишутся в `consumePelletAtPlayer`, `advancePhaseTimer` и `checkGhostCollisions`. `reset()` очищает очередь. `makePlayingGame` вычитывает `RoundStarted`. Файлы `Core/*`, `TestHelpers.h` и `SpriteAtlas.h` не меняю.
- `makeReadyGame` очередь **не** вычитывает. Но `Confirm` вызывает `reset()`, так что Ready-игра стартует с пустой очередью, а `RoundStarted` придёт при выходе из Ready. На этом строится тест `RoundStarted`.
- raylib уже подключён к `GamePresentation`. Модуль `raudio` в рецепте Conan включён по умолчанию. Если линковка `InitAudioDevice` всё же упадёт, проверю опции пакета, а не обходной путь.

## Шаги

### 1. Сохранить план
Сразу после утверждения записать его в `tasks/11-domain-events-and-audio.plan.md`.

### 2. `automation/generate_assets.py`: звуки
- Только `wave`, `struct`, `math`. Формат: моно, 22050 Гц, 16-bit signed PCM. Без случайного шума, поэтому файлы получаются побайтно воспроизводимыми.
- Общие хелперы:
  - `square(phase, duty)` — меандр, «8-битный» тембр;
  - `render_sweep(duration, f_start, f_end, duty, vibrato=...)`. Фаза накапливается интегрированием частоты, поэтому на свипах нет щелчков от скачков фазы;
  - огибающая: атака и затухание по ~5 мс, амплитуда ~0.35 от полной шкалы, без клиппинга;
  - `write_wav(path, samples)` через `wave` + `struct.pack("<h", ...)`.
- Звуки:

| Файл | Длительность | Синтез |
|---|---|---|
| `chomp.wav` | 0.10 с | V-образный свип «ва-ка»: 520→260 Гц, потом 260→520 Гц, duty 50% |
| `power_pellet.wav` | 0.25 с | восходящий свип 200→800 Гц с лёгким вибрато |
| `eat_ghost.wav` | 0.25 с | яркий высокий свип 900→1800 Гц, duty 25% |
| `death.wav` | 0.70 с | нисходящая сирена 800→110 Гц с вибрато ~8 Гц и затуханием к концу |
| `victory.wav` | 0.70 с | арпеджио C5–E5–G5 по ~0.12 с, затем удержание C6 до конца |

- `main()` генерирует и спрайтшит, и звуки. Отчёт печатается по каждому файлу: путь и длительность. Самопроверка: каждый WAV читаю обратно через `wave.open`, длительность должна совпадать с заданной в пределах одного сэмпла, иначе код возврата `1`. Docstring модуля дополняю.
- Запускаю `python automation/generate_assets.py`. 5 WAV остаются в `assets/sounds/` и попадают в Git; в `.gitignore` их нет.

### 3. `src/Game/Presentation/SoundSystem.h/.cpp` + CMake
- `class LifeExe::Presentation::SoundSystem`. Копирование удалено (`= delete`), поэтому перемещение тоже не объявляется. Деструктор свой. Без `[[nodiscard]]` и `noexcept`.
- Поля: `bool m_ready{false};`, `Sound m_chomp{}`, `m_powerPellet{}`, `m_eatGhost{}`, `m_death{}`, `m_victory{}`.
- Конструктор: `InitAudioDevice()`, затем `m_ready = IsAudioDeviceReady()`. Если `false`, выхожу без исключений и логов: система работает вхолостую (WSL, CI). Если `true`, вызываю `LoadSound("assets/sounds/<имя>.wav")` для всех пяти. Путь относительный, как у `spritesheet.png`. Рабочий каталог — корень проекта (`VS_DEBUGGER_WORKING_DIRECTORY`).
- Деструктор: при `m_ready` вызываю `UnloadSound` для каждого звука, затем `CloseAudioDevice()`.
- `processEvents(const std::vector<Core::GameEvent>&)`. При `!m_ready` сразу выхожу. Дальше полный `switch` без `default`: `-Wswitch`/C4062 поймает новое событие.
  - `PelletEaten` → `chomp`, только если `!IsSoundPlaying(m_chomp)`;
  - `PowerPelletEaten` / `GhostEaten` / `PlayerLostLife` / `Victory` → соответствующий звук;
  - `RoundStarted`, `GameOver` → без звука. Комментарий *why*: `death.wav` уже звучит по `PlayerLostLife`.
- **Развилка:** если WAV-файла нет, `LoadSound` вернёт пустой `Sound`. Воспроизведение идёт через маленький хелпер `playIfValid(const Sound&)` с проверкой `IsSoundValid`. Пропавший ассет даёт тишину, а не падение. Так же устроена «работа вхолостую».
- `src/Game/CMakeLists.txt`: `Presentation/SoundSystem.h` и `Presentation/SoundSystem.cpp` добавляю в `GamePresentation`. `GameCore` не меняется и не получает raylib или аудио.

### 4. `src/Game/App/main.cpp`
- `#include "SoundSystem.h"`.
- `Presentation::SoundSystem soundSystem;` ставлю сразу после `renderer.init(...)`.
- `soundSystem.processEvents(game.takeEvents());` вызываю один раз за кадр сразу после цикла `while (accumulator >= fixedDt)`. События всех фиксированных шагов кадра уходят одной пачкой. Комментарий *why*: очередь копится между шагами, поэтому вычитывать её внутри цикла не нужно.
- Порядок разрушения: `renderer.close()` явный, а `soundSystem` уничтожается в конце блока `try`. Аудиоустройство raylib не зависит от окна, так что закрывать его после окна безопасно.

### 5. `src/Game/Tests/GameTest.cpp`: набор `GameEventTest`
- Для понятных сообщений об ошибке добавляю `PrintTo(Core::GameEvent, std::ostream*)` в `namespace LifeExe::Core`, чтобы его нашёл ADL. Через него печатается `toString`, иначе gtest выведет байты. Очередь сравниваю целиком: `EXPECT_EQ(events, (std::vector{...}))`.
- Тесты переиспользуют существующие карты и хелперы (`c_corridorLevel`, `c_collisionLevel`, `c_frightenedRingLevel`, `c_eatGhostLevel`, `runIntoGhost`, `loseLife`, `stepUntil`, `skipReadyPhase`):

| TEST | Сценарий → ожидание |
|---|---|
| `GameEventToString` | все 7 значений |
| `RoundStartedOnReadyToActiveAndTakeClearsQueue` | `makeReadyGame` → очередь пуста → `skipReadyPhase` → `{RoundStarted}` → повторный `takeEvents()` пуст |
| `EventsOfConsecutiveUpdatesAccumulate` | коридор, два `update(secondsFor(1))` без `takeEvents` → `{PelletEaten, PelletEaten}` |
| `PelletEaten` | коридор, один тайл → `{PelletEaten}` |
| `PowerPelletEaten` | кольцевая карта, `stepUntil(isFrightenedActive)` → `{PowerPelletEaten}` |
| `VictoryAfterLastPellet` | коридор, 3 тайла → `{PelletEaten, PelletEaten, PowerPelletEaten, Victory}` |
| `GhostEaten` | сценарий `c_eatGhostLevel` до `isRubyEaten` → последнее событие `GhostEaten`, ровно одно |
| `PlayerLostLifeImmediatelyOnCollision` | `runIntoGhost` → последнее событие `PlayerLostLife`, ровно одно, `GameOver` нет; ещё `update(0.5f)` в `PlayerDying` → новых событий нет |
| `GameOverAfterDyingTimer` | 2× `loseLife`, затем `takeEvents`; `runIntoGhost` → `PlayerLostLife` без `GameOver`; `runWhilePhase(PlayerDying)` → `{GameOver}` |
| `ResetClearsUntakenEvents` | коридор, съесть точку, `Reset` → `takeEvents()` пуст |

- Счётчики через `std::ranges::count`, без `EXPECT` в циклах: это порог cognitive complexity в `clang-tidy`.
- `SoundSystem` модульными тестами не покрываю. Ему нужен raylib и реальное аудиоустройство, а `GameTests` намеренно не линкуют raylib.

### 6. Форматирование и проверки
- `python automation/format_files.py`, затем `format_files.py --check`, `run_tests.py`, `run_tidy.py`, `run_asan.py`. По каждому скрипту отчитываюсь кодом возврата.
- Если какой-то тест окажется хрупким по таймингам, правлю сценарий или карту, но проверки не ослабляю.

## Проверка
1. Коды возврата всех скриптов и вывод `generate_assets.py`: 5 WAV с нужными длительностями.
2. Звук в реальном окне тесты не покрывают. `GameApp` запускается только по просьбе.
3. Коммит не делаю. Предлагаю команды `git add` (включая `assets/sounds/*.wav`) и `git commit`, затем `/code-review` и PR из `feature/domain-events-and-audio`.
4. Приёмочный тест курса для спринта 11 не добавляю: это отдельный шаг спринта.
