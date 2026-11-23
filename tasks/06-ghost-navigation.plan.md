# План: задача 06. Четыре призрака в `Game`, тесты `GhostTest.cpp` и отрисовка в два слоя

## Контекст
Файлы-контракты не трогаю: `Core/Types.h`, `Command.h`, `GameConstants.h`, `Level.h`, `Maze.*`, `GridMover.*`, `Player.*`, `Ghost.*`, `GameSnapshot.h` и `Presentation/SpriteAtlas.h`. В них уже есть всё нужное:
- **`Ghost`.** Пока не истёк `releaseDelay`, призрак стоит. Внутри дома он идёт к `houseExitTile()` в режиме `Mover::GhostUsingDoor`. В центре этой клетки сбрасывается `inHouse`, дальше призрак идёт к цели: `update()` ведёт к `scatterCorner`, `updateToward()` к заданной клетке. Разворот запрещён, кроме тупика. При равной дистанции приоритет `Up > Left > Down > Right`.
- **Направление в начале.** У `GridMover` текущее направление сначала `None`, поэтому на первом решении запрета разворота нет.
- **Скорость.** `c_ghostSpeed = 7.5`, а это не степень двойки. Поэтому в тестах конечные точки выбираю посередине клетки, а не на границе, и сравниваю через `EXPECT_FLOAT_EQ` или по `tile()`.

## Шаги

### 1. Сохранить план
Сразу после утверждения записать его в `tasks/06-ghost-navigation.plan.md`.

### 2. `Core/Game.h` и `Core/Game.cpp`
- Подключить `#include "Ghost.h"` и `<array>`. Добавить поле `std::array<Ghost, 4> m_ghosts;` после `m_player`: порядок объявления важен, ведь `m_ghosts` строится из уже готового `m_maze`.
- **`makeGhosts(const Maze&)`** в анонимном пространстве имён возвращает `std::array<Ghost, 4>` в порядке `Ruby`, `Rosie`, `Indigo`, `Coral`. Значения берутся из `ghostSpawn(id)` и `scatterCorner(id)`:
  - `Ruby`: `inHouse = false`, `releaseDelay = 0.0f`;
  - остальные: `inHouse = true` и `c_rosieReleaseDelay`, `c_indigoReleaseDelay`, `c_coralReleaseDelay` соответственно.
- **Конструктор:** `m_ghosts{makeGhosts(m_maze)}`. Конструктора по умолчанию у `Ghost` нет, поэтому инициализация только в списке инициализации.
- **`updateSubstep`:** `m_player.update(...)`, затем `for (Ghost& ghost : m_ghosts) { ghost.update(m_maze, stepDt); }`, затем `consumePelletAtPlayer()`.
- **`reset()`:** `m_ghosts = makeGhosts(m_maze);` после восстановления лабиринта.
- **`snapshot()`:** поле `.ghosts` идёт после `.player`, в порядке объявления. Его заполняет `ghostSnapshots()`: цикл по `m_ghosts` → `{id(), position(), direction()}`.

### 3. `Tests/GhostTest.cpp` и запись в `GameTests` в `CMakeLists.txt`
Тестирую `Ghost` напрямую через `GhostConfig` на `Tests::c_smallLevel` и небольших картах в анонимном пространстве имён. Каждая карта содержит то, что требует парсер: `P`, `1–4` и `=`. Хелпер `secondsFor(tiles) = tiles / c_ghostSpeed`, плюс прогон мелкими шагами с проверкой условия на каждом шаге.

| TEST | Сценарий |
|---|---|
| `WaitsInHouseUntilReleaseDelay` | `Rosie` на `c_smallLevel`: `update(delay - 0.5)` → стоит на спавне, `isInHouse()` |
| `LeavesHouseThroughDoorAndClearsInHouse` | после таймера идёт мелкими шагами, проходит клетку `=` и в `houseExitTile()` получает `isInHouse() == false` |
| `GameReleasesGhostsInTurn` | через `Game::snapshot()`: в моменты `c_rosie/indigo/coralReleaseDelay ± 0.5 c` из дома вышли только те, чей таймер истёк. Остальные стоят на центре спавна, `Ruby` движется сразу |
| `ResetRestoresGhosts` | после `update` и `Reset` позиции в `snapshot().ghosts` совпадают со спавнами, `id` идут по порядку |
| `NoReverseInCorridor` | призрак идёт вправо по коридору, цель переносится влево → в следующем центре клетки направление остаётся `Right` |
| `ReversesInDeadEnd` | коридор упирается в стену → в последней клетке разворот на 180° |
| `TJunctionPicksShortestTurn` | приход в Т-перекрёсток снизу: цель справа → `Right`, цель слева → `Left` |
| `TiePriorityUpLeftDownRight` | `chooseNextDirection` в центре перекрёстка `+`. Цель на диагонали даёт равные дистанции: вверх-влево → `Up`, вниз-влево → `Left`, вниз-вправо → `Down` |
| `OutsideGhostNeverEntersDoor` | призрак вне дома стоит на `houseExitTile()`, цель внутри дома → ни на одном шаге не оказывается на клетке `GhostDoor` |
| `PassesThroughTunnelWithoutReversing` | ряд 5 `c_smallLevel`, цель за правым краем → призрак выходит в колонке 0 и ни разу не идёт `Left` |

### 4. `Presentation/RaylibRenderer.cpp`
- **`ghostTint(GhostId)`:** `Ruby` → `RED`, `Rosie` → `PINK`, `Indigo` → `SKYBLUE`, `Coral` → `ORANGE`.
- **`drawGhost(ghost, layout, totalTime, spritesheet)`.** `dest` и `origin` по центру клетки, как у игрока, без поворота:
  - нижний слой — `ghostBodyFrame(static_cast<int>(totalTime * 6.0))` с окраской `ghostTint`. Фаза идёт от `totalTime`, поэтому на паузе юбка замирает;
  - верхний слой — `ghostEyesFrame(ghost.direction)` с `WHITE`.
- В `render()` призраки рисуются после игрока, по всем элементам `snapshot.ghosts`.

### 5. Форматирование
`python automation/format_files.py`.

## Проверка
1. Код возврата каждого скрипта: `run_tests.py`, `format_files.py --check`, `run_tidy.py`, `run_asan.py`.
2. `GameApp` запускаю только по просьбе пользователя.
3. Коммит не делаю. Предложу команды `git add` и `git commit`, затем `/code-review` и PR из `feature/ghost-navigation`.

## Замечания (сообщаю, не правлю)
- В `Core/GridMover.cpp:67-68` две пустые строки подряд. Скорее всего, `format_files.py --check` это отметит, а `format_files.py` исправит файл-контракт.
- Призраки пока не взаимодействуют с игроком: столкновений и режимов нет, в задании их не требуется.
