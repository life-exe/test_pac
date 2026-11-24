# План: задача 08. Фазы раунда, коллизии, жизни, `RaylibRenderer` и цикл `main.cpp`

## Контекст
Контракты не трогаю. Они уже в индексе: `Core/*` и `Tests/TestHelpers.h`. Приёмочные тесты `MazeAcceptanceTest.cpp` и `GhostNavigationAcceptanceTest.cpp` лежат в рабочем дереве и в индекс не добавлены, их я тоже не меняю. Что уже есть в ядре:
- **Фазы.** `updateSubstep` в фазах `Ready` и `PlayerDying` только уменьшает `m_phaseTimer`, а сущности не обновляются. При этом `m_totalTime` растёт в `update()` до подшагов, то есть идёт и в паузе `Ready`.
- **Таймеры выхода из дома и таймер волн** тикают только в фазе `Active`. Поэтому отсчёт идёт от начала `Active`, а не от `totalTime`.
- **Смерть.** `checkGhostCollisions` → `--m_lives`, `PlayerDying`, `m_phaseTimer = c_dyingDuration`. Когда фаза кончается: при `lives > 0` вызывается `respawnRound()` (спавны, `Ready`, волна `Scatter`, карта и счёт сохраняются), иначе наступает `GameOver`. Фаза при этом остаётся `PlayerDying`.
- **Команды движения** игнорируются только в `PlayerDying`.
- **`Tests::makePlayingGame`** заканчивает работу ровно в момент перехода в `Active`.

Почему падают 3 теста (`GameWaveTest.SwitchesScatterChaseScatter`, `GameWaveTest.ResetRestartsScatterWave`, `GhostTest.GameReleasesGhostsInTurn`): призраки на `c_smallLevel` за 10–27 с ловят стоящего Чомпера, `respawnRound` сбрасывает волну и таймеры дома, а `advanceTo` вызывает `update(отрицательное dt)`, потому что `totalTime` уже равен `1.5`.

## Шаги

### 1. Сохранить план
Сразу после утверждения записать его в `tasks/08-collisions-and-lives.plan.md`.

### 2. Общие локальные хелперы в `GameTest.cpp` и `GhostTest.cpp` (в анонимном пространстве имён)
- `c_walledOffLevel` — копия `c_releaseMap` из приёмочного теста. Чомпер заперт в ряду туннеля, призракам к нему не пройти. `TestHelpers.h` менять нельзя, поэтому карту дублирую.
- `void skipReadyPhase(Core::Game&)` — тот же цикл `update(c_maxSubstep)`, что в `makePlayingGame`, но для уже существующей игры, например после `Command::Reset`.

### 3. Адаптация старых тестов
| Тест | Изменение |
|---|---|
| `GameWaveTest.SwitchesScatterChaseScatter` | карта `c_walledOffLevel`; в конце добавляю `EXPECT_EQ(lives, c_startingLives)` как доказательство, что коллизий не было |
| `GameWaveTest.ResetRestartsScatterWave` | `c_walledOffLevel`; после `Reset` проверяю `Scatter`, потом вызываю `skipReadyPhase` и прежние отметки времени |
| `GhostTest.GameReleasesGhostsInTurn` | `c_walledOffLevel`; вместо абсолютного времени — относительные шаги: `update(0.5f)` → `update(rosie − 1.0f)` → `update(1.0f)` → `update(indigo − rosie)` → `update(coral − indigo)` |
| `GhostTest.ResetReturnsGhostsToSpawns` | `c_walledOffLevel`, `update(c_coralReleaseDelay + 1.0f)` |
| `GhostTest.ResetRestartsReleaseTimers` | сейчас проходит формально: призраки неподвижны просто из-за `Ready`. Добавлю `skipReadyPhase` после `Reset` и проверку, что после ещё `update(1.0f)` Rosie покинула спавн |
| хелпер `advanceTo` | удаляю: он больше не используется, и clang-tidy предупредил бы о неиспользуемой функции |

Тесты `GameCoreTest` на классической карте и `GameScoreTest`/`GameVictoryTest` на коридоре продолжат проходить. В коридоре призраки не могут попасть в ряд игрока, а выход из дома ведёт в ряд 1.

### 4. Новые тесты в `GameTest.cpp` (набор `GameRoundTest`)
Карта для коллизий: Ruby стоит в тупике и может идти только навстречу Чомперу.
```
"#######\n"
"#.P..1#\n"
"###=###\n"
"# 234 #\n"
"#######\n"
```
Точка `{1,1}` лежит позади Чомпера. В каждой жизни он идёт только вправо, поэтому никогда её не съест, и `Victory` невозможна, когда бы ни съедались точки впереди. Опора на тайминг не нужна: `Player::tile()` теперь берёт клетку фактической позиции (`std::floor`), точка съедается на середине ребра, и прежняя карта `"#P...1#"` успевала очиститься до столкновения. Точка `{1,3}` впереди съедается раньше встречи с Ruby, поэтому счёт к моменту смерти больше нуля. Хелпер `runWhilePhase(game, phase)` делает шаги по `c_maxSubstep`, пока `state == Playing && roundPhase == phase`, но не больше заданного числа шагов. `EXPECT_*` в циклах не использую, чтобы не превысить порог cognitive complexity. Хелпер `loseLife(game)` делает `skipReadyPhase` → `MoveRight` → `runWhilePhase(Active)` → `runWhilePhase(PlayerDying)`.

| TEST | Сценарий → ожидание |
|---|---|
| `RoundPhaseToString` | `"Ready"`, `"Active"`, `"PlayerDying"` |
| `ReadyPhaseFreezesEntitiesWhileTimeRuns` | `makeReadyGame()`: `Ready`, `lives == 3`, `phaseTimeLeft == c_readyDuration`. Затем `MoveRight` и `update(c_readyDuration − 0.25f)`: Чомпер и все призраки на местах, `totalTime` вырос, фаза всё ещё `Ready`. После `update(0.5f)`: фаза `Active`, Ruby и Чомпер сдвинулись |
| `GhostCollisionCostsLifeAndFreezesEntities` | Сразу после контакта: `lives == 2`, `PlayerDying`, `phaseTimeLeft == c_dyingDuration`, `state == Playing`. Затем `MoveLeft` и `update(0.5f)`: позиции и направления не изменились, `totalTime` вырос |
| `RespawnAfterDyingKeepsPelletsAndScore` | Запоминаю `score` и `remainingPellets` в момент смерти (`score > 0`). Когда `PlayerDying` закончится: `Ready`, `phaseTimeLeft == c_readyDuration`, `lives == 2`, счёт и число точек те же, `{1,3}` пустая, `{1,1}` позади Чомпера ещё на месте, Чомпер и призраки в центрах своих спавнов |
| `LosingLastLifeEndsGame` | `loseLife` ×2 (`lives` 2, затем 1, состояние `Playing`). Третий контакт: `lives == 0`, ещё `Playing`. Когда `PlayerDying` закончится: `GameOver`, после этого `update(1.0f)` не меняет `totalTime` |
| `ResetRestoresLivesAndReadyPhase` | После одной смерти `Reset` → `lives == 3`, `Ready`, `phaseTimeLeft == c_readyDuration` |

### 5. `Presentation/RaylibRenderer.cpp`
- **Жизни.** `drawHud` и `drawBottomHud` теперь принимают `const Texture2D&`. На первой строке нижней панели, по центру, рисую `std::max(0, snapshot.lives)` значков `lifeIconSprite()`: 16×16, то есть 1:1 с атласом, с зазором 4 px.
- **`READY!`** при `Playing && Ready`. Отдельная функция `drawReadyText(snapshot, layout)`: текст жёлтый (`YELLOW`), по центру лабиринта по X, высота шрифта примерно `tileSize`. По Y — на 2 клетки ниже самого нижнего призрака: в `Ready` все призраки стоят на спавнах, так что это ряд сразу под стенкой дома (на классической карте ряд 11). Причину поясню комментарием.
- **Смерть.** При `PlayerDying` Чомпер рисуется кадром `playerDeathFrame(static_cast<int>(progress * 4.0f))`, где `progress = 1.0f − phaseTimeLeft / Core::c_dyingDuration`; значение `1.0` зажимается к кадру 3 внутри `playerDeathFrame`. Угол 0, без поворота. Призраков в этой фазе скрываю, как в классике. Нужен `#include "GameConstants.h"`.
- **`GAME OVER`.** `drawVictoryOverlay` обобщаю в `drawEndOverlay(screen, title, titleColor)`: полупрозрачный оверлей и подсказка `PRESS [R] TO RESTART`. Его вызывают `Victory` (`GOLD`) и `GameOver` (`RED`). Ветка `Paused` не меняется.

### 6. `App/main.cpp`
Условие цикла `while (!renderer.shouldClose())`, остальной код без изменений.

### 7. Форматирование и проверки
`python automation/format_files.py`, затем `format_files.py --check`, `run_tests.py`, `run_tidy.py` и `run_asan.py`. По каждому скрипту отчитаюсь кодом возврата.

## Проверка
1. Отчитаюсь кодом возврата каждого скрипта.
2. `GameApp` запускаю только по просьбе пользователя. Визуальную часть (жизни, `READY!`, анимация смерти, `GAME OVER`) модульные тесты не покрывают.
3. Коммит не делаю. Предложу команды `git add` и `git commit` (включая непроиндексированные приёмочные тесты), затем `/code-review` и PR из `feature/collisions-and-lives`.

## Замечания (сообщаю, не правлю)
- **`Esc` больше не закрывает окно.** Из-за `SetExitKey(KEY_NULL)` клавиша `Esc` теперь показывает экран `GAME OVER` вместо закрытия. Окно закрывается только крестиком. По заданию это временно, до спринта 10.
- **После смерти на последней жизни фаза остаётся `PlayerDying`.** Поэтому под оверлеем `GAME OVER` останется последний кадр смерти, а призраки будут скрыты. После `Quit` фаза `Active`, и сцена рисуется как обычно.
