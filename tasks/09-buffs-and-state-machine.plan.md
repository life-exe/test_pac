# План: задача 09. Возврат `Eaten`-призрака в дом, тесты автомата и отрисовка `Frightened`/`Eaten`

## Контекст
Контракты не трогаю. В ядре уже есть: включение бафа (`activateFrightened`), пауза волн, истечение бафа без разворота, комбо `200 << min(n,3)`, иммунитет `Eaten` к столкновениям и повторному `PowerPellet` (всё в `Game.cpp`). Есть также `chooseRandomDirection`, поле `m_enteringHouse` и `Mover::GhostUsingDoor` для `Eaten` (`currentMover()`). Сейчас `Ghost::update(params)` просто вызывает `update(context)`: призрак не меняет скорость, не выбирает направление случайно и не возвращается в дом.

Развилка: логику `Frightened`/`Eaten` должны разделять все четыре перегрузки `update`/`updateToward`, а у трёх старых нет ни `IRandomSource`, ни `waveMode`. Добавить private-методы в `Ghost.h` нельзя (контракт), поэтому общий цикл живёт в `Ghost.cpp` (шаг 2).

## Шаги

### 1. Сохранить план
Сразу после утверждения записать его в `tasks/09-buffs-and-state-machine.plan.md`.

### 2. `Core/Ghost.cpp`: автомат режимов без изменения `Ghost.h`
- **Общий цикл движения.** Свободная функция в анонимном пространстве имён `advanceMovement(maze, dt, GhostMotion, MotionInputs)`:
  - `GhostMotion` — набор ссылок на поля призрака (`m_mover`, `m_mode`, `m_inHouse`, `m_enteringHouse`, `m_holdReversedDirection`, `m_releaseDelay`) и `m_homeTile`. Ссылки на private-поля законно передавать из тела метода.
  - `MotionInputs` — `const Ghost&` (для публичных `chooseNextDirection`/`chooseRandomDirection`), цель волны, `IRandomSource*` и `waveMode`.
- **Маршруты вызова.** `update(maze, dt)` → `updateToward(m_scatterCorner)`. `update(context)` → `updateToward(resolveTarget(context))`. `updateToward` → `advanceMovement(…, random = nullptr, waveMode = Scatter)`. `update(params)` → `advanceMovement(…, resolveTarget(params.context), &params.random, params.waveMode)`. Сейчас каждый путь двигает призрака по-своему, после правки все идут через один цикл.
- **Скорость.** `speedFor(GhostMode)`: `Frightened` ×`0.5`, `Eaten` ×`2.0`, иначе `c_ghostSpeed`.
- **В центре клетки, по порядку:**
  1. `m_inHouse && tile == houseExit` → `m_inHouse = false` (как сейчас).
  2. `Eaten`: `tile == m_homeTile` → возрождение (`m_enteringHouse = false`, `m_inHouse = true`, `m_releaseDelay = 0.0f`, `m_mode = waveMode`). Остаток пути пересчитываю под новую скорость. Иначе при `tile == houseExit` → `m_enteringHouse = true`.
  3. Выбор направления. Сначала соблюдается `m_holdReversedDirection`. `Frightened`, вне дома, есть `random` → `chooseRandomDirection`. Иначе `chooseNextDirection` к цели: в доме — `houseExit`; в `Eaten` — `m_enteringHouse ? m_homeTile : houseExit`; иначе — цель волны.
- **Разворот.** В `chooseNextDirection` разворот разрешён при `m_inHouse || m_enteringHouse`: фаза входа спускается через дверь.
- **`setMode`.** Переход в `Eaten` сбрасывает `m_holdReversedDirection`, переход в любой другой режим — `m_enteringHouse`.
- **Старые пути без `random`.** `update(maze, dt)`, `update(context)` и `updateToward` в `Frightened` выбирают направление к цели, а при возрождении берут режим `Scatter`. Это описываю комментарием (*why*).

### 3. `Tests/GhostTest.cpp`
Добавляю заглушку `FixedRandomSource{index}`: возвращает `index % count` и запоминает последний `count`.

| TEST | Сценарий → ожидание |
|---|---|
| `ChooseRandomDirectionFollowsPriorityOrder` | `c_crossLevel`, призрак стоит в `{3,4}` (направление `None`). Индексы 0..3 → `Up, Left, Down, Right`, `count == 4` |
| `ChooseRandomDirectionSkipsReverse` | Из `{3,3}` `updateToward(secondsFor(1.5f), {3,8})` → клетка `{3,4}`, движение `Right`. Индекс 1 → `Down` (кандидаты `Up, Down, Right`), `count == 3` |
| `ModeSpeedMultipliers` | Коридор `c_tJunctionLevel`, старт `{1,2}`, `0.1` с к `{1,7}`: обычный режим сдвигается на `0.5` клетки, `Frightened` — на `0.25`, `Eaten` — на `1.0` |
| `EatenGhostRevivesFromEveryCorner` | `c_classicLevel`, углы `{1,1} {1,19} {19,1} {19,19}`, `homeTile = ghostSpawn(Indigo)`, `setMode(Eaten, false)`. Хелпер `stepsUntilRevived` (`update(params)` по `1/64` с, не больше 640 шагов, без `EXPECT` в цикле). Ожидание: шагов меньше лимита, а после этого `isInHouse()`, `mode == Chase` (`waveMode`), `releaseDelay == 0`, и призрак за ограниченное число шагов снова выходит на `houseExitTile()` |

### 4. `Tests/GameTest.cpp`
Хелперы: `FixedRandomSource`, `runUntil(game, predicate, maxSteps)` (подшаги, возвращает `bool`, без `EXPECT` внутри), `ghostOf`.

**Карта A** — кольцо призраков без тупиков, Чомпер в изолированном ряду:
```
"###########\n"
"#....1....#\n"
"#.###=###.#\n"
"#.# 234 #.#\n"
"#.#######.#\n"
"#.........#\n"
"###########\n"
"#.Po......#\n"
"###########\n"
```
**Карта B** — коридор: Ruby идёт навстречу, второй `o` правее:
```
"############\n"
"#.Po....o.1#\n"
"##=#########\n"
"#234########\n"
"############\n"
```
**Карта C** — комбо: тупик для Чомпера слева от выхода, кольцо справа для возрождённой Ruby (в `Scatter` она уходит направо и кружит там):
```
"##########\n"
"#1 oP....#\n"
"###=# ## #\n"
"#423#    #\n"
"##########\n"
```

| TEST | Сценарий → ожидание |
|---|---|
| `GhostModeToStringBuffs` | `"Frightened"`, `"Eaten"` |
| `PowerPelletFrightensAndReversesOnlyOutsideHouse` | A: `update(c_rosieReleaseDelay + 0.1f)`, Rosie уже идёт `Right` внутри дома. `MoveRight`, подшаги до `frightenedTimer > 0` с запоминанием снимка до подшага. Ожидание: все `Frightened`, таймер ≈ `c_frightenedDuration`, Ruby = `opposite(до)`, Rosie без разворота |
| `FrightenedPausesWaveAndExpiresWithoutReverse` | A: съесть `o`, запомнить прошедшее время волны. Подшаги до `frightenedTimer == 0`: все в `Scatter` (без паузы был бы уже `Chase`), Ruby ≠ `opposite(до)` (в кольце нет тупиков). Затем `update(scatter − elapsed − margin)` → `Scatter`, `update(2·margin)` → `Chase` |
| `EatingFrightenedGhostAndEatenSafety` | B: стою, пока Ruby не дойдёт до `x < 7`, затем `MoveRight`. Подшаги до `Eaten`: прирост счёта `== 200`, `lives == 3`, `Active`. Второй `o` съедается, пока Ruby в `Eaten`: Ruby остаётся `Eaten`, остальные `Frightened`, таймер снова ≈ 7 с. `Eaten`-Ruby на обратном пути проходит сквозь Чомпера — `lives == 3`. После возрождения у Ruby `mode == Scatter` |
| `ComboDoublesWithinOneFrightened` | C, `FixedRandomSource{0}`: `MoveLeft` → `o`. Ruby съедена: `+200`. Rosie выходит в 3 с, на развилке берёт `Left` (индекс 0) и идёт в тупик к Чомперу: `+400`, `lives == 3`, всё до истечения бафа |

Тайминги посчитаны вручную. Если какая-то карта поведёт себя не так, подправляю тайминг или карту и описываю это в отчёте. Проверки не ослабляю.

### 5. `Presentation/RaylibRenderer.cpp`
- `drawGhost` принимает `const Core::GameSnapshot&` вместо `totalTime` и делает `switch (ghost.mode)`:
  - `Frightened` → только `ghostFrightenedFrame(flashing)`, `WHITE`;
  - `Eaten` → только `ghostEyesFrame(ghost.direction)`;
  - `Scatter`/`Chase` → прежние два слоя.
- `isFrightenedFlashing(snapshot)`: `0 < frightenedTimer < c_frightenedFlashTime` и нечётный полупериод `totalTime / 0.2`. Константа `c_flashPeriod{0.2}`, с комментарием *why*: время замирает на паузе, мигание тоже.

### 6. Форматирование и проверки
`format_files.py`, затем `format_files.py --check`, `run_tests.py`, `run_tidy.py`, `run_asan.py`. По каждому скрипту отчитываюсь кодом возврата.

## Проверка
1. Коды возврата всех скриптов.
2. Визуальную часть (мигание, глаза) тесты не покрывают. `GameApp` запускаю только по просьбе.
3. Коммит не делаю. Предлагаю `git add`/`git commit`, затем `/code-review` и PR из `feature/buffs-and-state-machine`.

## Замечания для автора курса (сообщаю, не правлю)
- **Чекпоинт 09 расходится с контрактом.** `games/maze/checkpoints/09-…/Core/Ghost.h` добавляет четыре private-метода. Задание запрещает менять `Ghost.h`, а подсказка шага 3 в `SPRINT.md` даёт только `m_enteringHouse`, так что агент в проекте ученика повторить эталон не может.
- **Приёмочный тест 09 не скомпилируется.** `BuffsAndStateMachineAcceptanceTest.cpp:120` вызывает `ruby.update(maze, c_maxSubstep, context, random, GhostMode::Chase)`, а перегрузки с пятью аргументами нет ни в чекпоинте, ни в контракте. Нужно `update(maze, dt, {.context = context, .random = random, .waveMode = GhostMode::Chase})`.
