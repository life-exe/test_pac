# План: задача 05 — тесты точек, очков и `Victory`, точки и HUD в `RaylibRenderer`

## Контекст
Эти файлы написаны вручную, их не трогаю: `Core/*` и `Presentation/SpriteAtlas.h`. Вот что в них уже есть:
- **`Maze`:** `remainingPellets()` и `consumeTileAt(pos)`. Для стены, пустой клетки и клетки за картой он возвращает `Empty`, счётчик не меняет.
- **`Game::update`:** режет `dt` на подшаги `c_maxSubstep`. После каждого подшага вызывается `consumePelletAtPlayer()`: он начисляет очки, обновляет `highScore` и переводит игру в `Victory`, когда `remainingPellets() == 0`. В `Victory` `update` больше ничего не делает.
- **`reset()`:** восстанавливает `m_initialMaze`, обнуляет `score` и сохраняет `highScore`.
- **Точка съедается в момент прихода в клетку.** `tile()` меняется в `arriveAtNextCell`, поэтому точка исчезает, когда игрок дошёл до её клетки, а не когда частично в неё зашёл.

## Шаги

### 1. Сохранить план
Сразу после утверждения записать его в `tasks/05-pellets-score-and-hud.plan.md`.

### 2. `Tests/MazeTest.cpp`: раздел `// --- Pellets ---`
| TEST | Сценарий |
|---|---|
| `SmallLevelCountsPellets` | `c_smallLevel` → `remainingPellets() == 40` (38 `.` + 2 `o`) |
| `ClassicLevelCountsPellets` | число `Pellet`/`PowerPellet` в `tiles()` совпадает с `remainingPellets()`, и оно больше `0` |
| `ConsumePelletReturnsPelletAndClearsTile` | `(1,1)` → возвращает `Pellet`, `tileAt == Empty`, счётчик 40 → 39 |
| `ConsumePowerPelletReturnsPowerPellet` | `(6,1)` → возвращает `PowerPellet`, клетка пуста, счётчик −1 |
| `ConsumeClearedTileIsNoOp` | повторный вызов на `(1,1)` → `Empty`, счётчик не меняется |
| `ConsumeWallAndEmptyTileIsNoOp` | стена `(0,0)`, пустая клетка `(3,3)`, туннель `(5,0)` → `Empty`; стена остаётся `Wall`, счётчик тот же |
| `ConsumeOutOfBoundsIsNoOp` | `{-1,0}`, `{0,-1}`, `{rows,0}`, `{0,cols}` → `Empty`, счётчик тот же |

### 3. `Tests/GameTest.cpp`
- **`GameStateToString`:** добавить проверку `toString(GameState::Victory) == "Victory"`.
- **Карта для тестов.** В анонимном пространстве имён завести короткий коридор `c_corridorLevel`: ряд `#P..o#`, плюс призраки 1–4, дверь `=` и стены, которые требует парсер. Итого 3 точки, ход вправо. Время перехода в соседнюю клетку — хелпер `secondsFor(tiles) = tiles / c_playerSpeed`; `0.125f` — степень двойки, поэтому суммы точные.

| TEST | Сценарий |
|---|---|
| `EatingPelletAddsPelletScore` | `MoveRight`, `update(secondsFor(1))` → `score == c_pelletScore`, `remainingPellets == 2`, `highScore == score` |
| `EatingPowerPelletAddsPowerPelletScore` | три клетки → `score == 2*c_pelletScore + c_powerPelletScore` |
| `HighScoreFollowsScore` | после каждой клетки `highScore == score` |
| `SetHighScoreKeepsLargerRecord` | `setHighScore(500)` → 500; `setHighScore(100)` → по-прежнему 500 |
| `SetHighScoreBelowScoreIsOvertaken` | `setHighScore(15)`; первая точка → `score 10`, `highScore 15`; вторая → оба 20 |
| `ResetRestoresPelletsAndKeepsHighScore` | съесть 2 точки, `Reset` → `score 0`, `remainingPellets 3`, клетки снова `Pellet`, `highScore` сохранён |
| `NoPelletsEatenWhilePaused` | `Pause` + `update` → `score 0`, `remainingPellets 3` |
| `EatingLastPelletWinsGame` | пошагово до последней точки → `state == Victory`, в снимке `remainingPellets 0` |
| `VictoryFreezesGame` | после победы `update(1.0f)` не меняет `totalTime` и `score`; `Reset` → `Playing` |
| `LargeStepEatsEveryPelletOnTheWay` | один `update(secondsFor(3))` → все 3 точки съедены, `score 70`, `Victory`. Без подшагов была бы съедена только конечная клетка |

### 4. `Presentation/RaylibRenderer.cpp`
- **`drawMaze`.** Новая сигнатура: `drawMaze(snapshot, layout, spritesheet)`.
  - **`Pellet`:** `DrawTexturePro` с `pelletSprite()` на весь тайл.
  - **`PowerPellet`:** `powerPelletSprite()` с мягкой пульсацией масштаба `0.8–1.0` по `sin(totalTime * 2π * 2 Гц)`. Рисуется от центра тайла (`origin = dest/2`), чтобы спрайт не сползал.
  - Комментарий-заглушку про «later tasks» убрать.
- **HUD сверху.** Слева `SCORE: n`, по центру `HIGH SCORE: n`, справа `STATE: …`. FPS и `Time` уходят в нижнюю полосу, чтобы верх не был перегружен. Все строки собираются через `std::format`.
- **HUD снизу.** Слева `PELLETS: n` (`remainingPellets`), справа — подсказка по клавишам. Ширина считается через `MeasureText`, чтобы тексты не наезжали друг на друга.
- **`drawStateOverlay`.** Для `Victory` рисуется полупрозрачный оверлей на весь экран (`Fade(BLACK, 0.6f)`), по центру `VICTORY!` (крупно, `GOLD`), ниже `PRESS [R] TO RESTART`. Попутно чинится текущая ветка `Victory: break`: сейчас она передаёт `nullptr` в `MeasureText`.
- `GameCore` от `raylib` по-прежнему не зависит: правки только в `Presentation`.

### 5. Форматирование
`python automation/format_files.py`.

## Проверка
1. `python automation/build_debug.py`: сборка без предупреждений.
2. Скрипты из задания с кодами возврата:
   - `python automation/run_tests.py` — все тесты зелёные, включая новые;
   - `python automation/format_files.py --check`;
   - `python automation/run_tidy.py`;
   - `python automation/run_asan.py`.
3. `GameApp` запускается только по просьбе пользователя: точки видны, энерджайзеры пульсируют, счёт растёт, при сборе всех точек появляется `VICTORY!`.
4. Коммит не делаю. Предложу команды `git add` и `git commit`, дальше `/code-review` и PR из `feature/pellets-score-and-hud`.

## Замечания (сообщаю, не правлю)
- `highScore` между запусками не сохраняется: `setHighScore` пока никто не вызывает. В задании загрузка или запись рекорда не требуется.
