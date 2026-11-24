# План: задача 07. Стратегии `Rosie`, `Indigo`, `Coral`, тесты `GhostStrategyTest.cpp` и волны `Scatter`/`Chase`

## Контекст
Файлы-контракты не трогаю. Их изменения уже лежат в рабочем дереве, но ещё не закоммичены: `Types.h`, `GameConstants.h`, `GridMover.*`, `Ghost.*`, `GhostStrategy.h`, `GameSnapshot.h`. В них уже есть всё нужное:
- **Арифметика `GridPos`.** Есть `operator+`, `operator-`, `operator*(int, GridPos)` и `directionDelta()`, причём `None` даёт `{0, 0}`.
- **`Ghost::resolveTarget`.** Стратегия спрашивается только в режиме `Chase`. В режиме `Scatter` цель — `m_scatterCorner`.
- **Волны в `Game`.** Режим переключает `updateWaveTimer`: таймер идёт только в состоянии `Playing`, режим через `setMode` получают все 4 призрака. Режим виден снаружи через `snapshot().ghosts[i].mode`.
- **Подшаг `c_maxSubstep = 1/64`.** Это точная степень двойки, поэтому `7.0` и `20.0` набираются без ошибки округления. Граница волны приходится точно на `t = 7.0` и `t = 27.0`, но в тестах всё равно беру запас ±0.25 с.

## Шаги

### 1. Сохранить план
Сразу после утверждения записать его в `tasks/07-ghost-ai-strategy.plan.md`.

### 2. `Core/GhostStrategy.cpp`
`RubyStrategy` оставляю как есть. В анонимном пространстве имён будет два элемента:
- `constexpr int c_coralChaseRadiusSq{64};` — порог 8 клеток. В `GameConstants.h` его не кладу, это контракт;
- `int squaredDistance(GridPos lhs, GridPos rhs)`. Такой же хелпер есть в `Ghost.cpp`, но он закрыт в его анонимном пространстве имён, а общий заголовок менять нельзя.

Реализации:
- **`RosieStrategy`:** `return context.playerPos + (4 * directionDelta(context.playerDir));`
- **`IndigoStrategy`:** `const GridPos pivot{context.playerPos + (2 * directionDelta(context.playerDir))};`, затем `return context.rubyPos + (2 * (pivot - context.rubyPos));`
- **`CoralStrategy`:** при `squaredDistance(ghostPos, playerPos) > c_coralChaseRadiusSq` возвращает `playerPos`, иначе `scatterCorner`.

### 3. `Core/Game.cpp` → `makeGhosts`
Меняю только три `nullptr` на `std::make_unique<RosieStrategy>()`, `std::make_unique<IndigoStrategy>()` и `std::make_unique<CoralStrategy>()`. Остальную логику и порядок `#include` не трогаю.

### 4. `Tests/GhostStrategyTest.cpp` и запись в `GameTests` в `CMakeLists.txt`
- Хелпер `makeContext(...)` собирает `GhostContext` через designated initializers.
- Проверки в стиле проекта: `EXPECT_EQ(target, (GridPos{r, c}))`.
- Без `EXPECT_*` в циклах: так не превысим порог `readability-function-cognitive-complexity`.

| TEST | Сценарий → ожидание |
|---|---|
| `RubyStrategyTest.TargetsPlayerTile` | `playerPos {10,10}`, `{1,1}`, `{-2,30}`; прочие поля заполнены «шумом» → цель всегда `playerPos` |
| `RosieStrategyTest.TargetsFourTilesAhead` | `playerPos {10,10}`: `Up → {6,10}`, `Down → {14,10}`, `Left → {10,6}`, `Right → {10,14}` |
| `RosieStrategyTest.NoneDirectionTargetsPlayer` | `None → {10,10}` |
| `RosieStrategyTest.TargetMayLeaveMap` | `playerPos {1,1}`: `Up → {-3,1}`, `Left → {1,-3}` |
| `IndigoStrategyTest.DoublesVectorFromRubyToPivot` | `playerPos {10,10}`, `rubyPos {8,8}`: `Up → {8,12}`, `Down → {16,12}`, `Left → {12,8}`, `Right → {12,16}` |
| `IndigoStrategyTest.NoneDirectionUsesPlayerAsPivot` | `None`, `rubyPos {8,8}` → `{12,12}` |
| `IndigoStrategyTest.RubyOnPivotTargetsPivot` | `rubyPos` совпадает с `pivot` → цель `pivot` |
| `IndigoStrategyTest.TargetMayLeaveMap` | `playerPos {1,1}`, `Up`, `rubyPos {5,5}` → `{-7,-3}` |
| `CoralStrategyTest.ChasesWhenFartherThanEightTiles` | `ghostPos {0,0}`, `playerPos {8,1}` (`distSq = 65`) → `playerPos` |
| `CoralStrategyTest.ExactBoundaryRetreatsToCorner` | `playerPos {8,0}` (`distSq = 64`) → `scatterCorner` |
| `CoralStrategyTest.RetreatsWhenClose` | `playerPos {3,4}` (`distSq = 25`) → `scatterCorner` |

Все ветки `GhostStrategy.cpp` покрыты, обе ветки `Coral` тоже.

### 5. `Tests/GameTest.cpp`
Хелпер `expectAllGhostsInMode(const Core::Game&, Core::GhostMode)` проходит по `snapshot().ghosts`. Игра — `Tests::makePlayingGame()`: игрок стоит на месте, поэтому `Victory` в тесте не наступит.

| TEST | Сценарий |
|---|---|
| `GameWaveTest.GhostModeToString` | `toString(Scatter) == "Scatter"`, `toString(Chase) == "Chase"` |
| `GameWaveTest.SwitchesScatterChaseScatter` | Старт: `Scatter`. Затем `update(c_scatterDuration - 0.25f)` → `Scatter`; `update(0.5f)` → `Chase`; `update(c_chaseDuration - 0.5f)` → `Chase`; `update(0.5f)` → `Scatter` |
| `GameWaveTest.ResetRestartsScatterWave` | Переходим в `Chase`, затем `Command::Reset` → `Scatter`. Таймер перезапущен: `update(c_scatterDuration - 0.25f)` → `Scatter`, `update(0.5f)` → `Chase` |

### 6. Форматирование и проверки
`python automation/format_files.py`, затем по коду возврата `format_files.py --check`, `run_tests.py`, `run_tidy.py` и `run_asan.py`.

## Проверка
1. Отчитаюсь кодом возврата каждого скрипта.
2. `GameApp` запускаю только по просьбе пользователя.
3. Коммит не делаю. Предложу команды `git add` и `git commit`, затем `/code-review` и PR из `feature/ghost-ai-strategy`.

## Замечания (сообщаю, не правлю)
- **Призрак в доме.** `setMode` меняет режим и у призраков в доме, без разворота. Поэтому в снимке у всех четырёх один режим, и проверка «все в волне» корректна.
- **Цель `Rosie` и `Indigo` бывает за картой.** Это ожидаемо: `chooseNextDirection` сравнивает только квадраты расстояний и в лабиринт по этой цели не обращается.
