# План: задача 04 — `assets/spritesheet.png`, спрайт Чомпера и тесты `SpriteAtlas`

## Контекст

Эти файлы уже написаны вручную, их не трогаю:
- `Core/Types.h`, `Command.h`, `GameConstants.h`, `Maze.*`, `GridMover.*`;
- `Player.*`: `animTime` растёт только при `isMoving`;
- `GameSnapshot.h`: `PlayerSnapshot::animTime`;
- `Presentation/SpriteAtlas.h`: атлас `112x48`, `7x3` тайлов по `16`.

`Game.h` уже передаёт `animTime` в снимок. Что работает само, без новых правок:
- **Пауза.** `Game::update` двигает игрока только в состоянии `Playing`, поэтому на паузе `animTime` стоит на месте.
- **Сброс.** `reset()` пересоздаёт `Player`, поэтому `animTime` возвращается в `0.0f`.

## Шаги

### 1. Сохранить план
Сразу после утверждения записать его в `tasks/04-spritesheet-and-animation.plan.md`.

### 2. `automation/generate_assets.py` (только `math`, `pathlib`, `struct`, `zlib`)
- **Холст.** Буфер RGBA `112x48`, изначально полностью прозрачный. Вспомогательные функции: `put_pixel`, `fill_circle`, `fill_rect` и попиксельная проверка «точка внутри фигуры» относительно центра тайла.
- **PNG без сторонних библиотек.** Сигнатура, чанки `IHDR` (8 бит, color type 6), `IDAT` (`zlib.compress`, перед каждой строкой байт фильтра `0`), `IEND`. CRC считается через `zlib.crc32`.
- **Кадры** (столбец, строка):
  - `(0..2, 0)` — Чомпер смотрит вправо: жёлтый круг r≈7 с вырезом-ртом примерно 0°, 25° и 45° (полуугол);
  - `(3..6, 0)` — схлопывание: рот раскрывается на 90°, 150°, 220°, последний кадр — тонкий «огрызок»/искры (кадр не пустой);
  - `(0..1, 1)` — белое тело призрака: купол и юбка с двумя фазами волн. Глаза не рисуются, потому что тело окрашивается через `tint`;
  - `(2, 1)` — испуганный синий призрак с бледным лицом; `(3, 1)` — белый с красным лицом (мигание);
  - `(0..3, 2)` — глаза: белки и синие зрачки, смещённые `Right`, `Down`, `Left`, `Up`;
  - `(4, 2)` — точка `2x2` в центре; `(5, 2)` — энерджайзер, круг r≈5; `(6, 2)` — иконка жизни, маленький Чомпер.
- **Самопроверка.** Перед записью скрипт проверяет, что во всех 18 тайлах есть непрозрачный пиксель, иначе завершается с кодом `1`. Результат детерминированный, путь к файлу считается от корня репозитория через `pathlib`.
- **Запуск.** Запустить `python automation/generate_assets.py` и убедиться, что `assets/spritesheet.png` появился на диске, а в заголовке `112x48` RGBA.

### 3. `.github/workflows/build.yml` — шаг `Package game`
- **Windows:** `Copy-Item -Recurse assets dist/assets`.
- **Linux:** `cp -r assets dist/assets`.

### 4. `Presentation/RaylibRenderer.h` / `.cpp`
- **Заголовок.** В `.h` добавить `#include <raylib.h>` и поле `Texture2D m_spritesheet{};`.
- **`init()`.** После `InitWindow` вызвать `m_spritesheet = LoadTexture("assets/spritesheet.png");` и `SetTextureFilter(m_spritesheet, TEXTURE_FILTER_POINT);`.
- **`close()`.** Перед `CloseWindow()` написать `if (m_spritesheet.id != 0) { UnloadTexture(m_spritesheet); }` и затем `m_spritesheet = {};`.
- **`drawPlayer(player, layout, texture)`** вместо жёлтого круга:
  - `constexpr float c_chomperFps{12.0f};` и `frame = static_cast<int>(player.animTime * c_chomperFps)`, затем `SpriteAtlas::playerMoveFrame(frame)`;
  - `source` строится из `SpriteRect`; `dest = {centerX, centerY, size, size}`; `origin = {size * 0.5f, size * 0.5f}`;
  - угол берётся из вспомогательной функции `facingAngle(Direction)`: `Right` 0, `Down` 90, `Left` 180, `Up` 270, `None` 0;
  - рисует `DrawTexturePro(texture, source, dest, origin, angle, WHITE)`.

  Если игрок стоит, `animTime` заморожен и кадр не меняется.
- **Подключение.** `#include "SpriteAtlas.h"`; комментарий о временной заглушке убрать.

### 5. `src/Game/CMakeLists.txt`
- `target_include_directories(GameTests PRIVATE Tests Presentation)`. `GamePresentation` к `GameTests` **не** линкую.
- `Tests/SpriteAtlasTest.cpp` добавить в `add_executable(GameTests ...)`.
- **Нужная правка вне списка задания (согласована).** `PRIVATE raylib` → `PUBLIC raylib` у `GamePresentation`: после `#include <raylib.h>` в публичном `RaylibRenderer.h` цель `GameApp` (её `main.cpp` подключает этот заголовок) иначе не найдёт `raylib.h`. `GameCore` и `GameTests` от этого `raylib` не получают.

### 6. `Tests/PlayerTest.cpp` — проверки `animTime`
| TEST | Сценарий |
|---|---|
| `AnimTimeStartsAtZero` | после спавна `animTime == 0.0f` |
| `AnimTimeGrowsWhileMoving` | `MoveLeft`, `update(0.125f)` → `0.125f`; ещё `update(0.0625f)` → `0.1875f` |
| `AnimTimeFreezesWhenStoppedAtWall` | `MoveDown`, кадрами по `0.125f` до остановки у стены (`isMoving == false`), запомнить значение; дальнейшие `update` его не меняют |
| `AnimTimeFreezesWhilePaused` | движение, `Pause`, `update(0.5f)` → без изменений |
| `ResetClearsAnimTime` | движение, `Reset` → `0.0f` |

Двоичные дроби дают точные суммы, поэтому сравнение через `EXPECT_FLOAT_EQ`.

### 7. `Tests/SpriteAtlasTest.cpp` (новый)
- **Константы:** `c_tileSize 16`, `c_atlasCols 7`, `c_atlasRows 3`, `c_atlasWidth 112`, `c_atlasHeight 48`.
- **Все 18 кадров.** Хелпер собирает `std::array<SpriteRect, 18>`: move 0..2, death 0..3, body 0..1, frightened false/true, eyes R/D/L/U, pellet, power, life. Для каждого кадра `isInsideAtlas`, а также явная проверка границ `x + w <= 112`, `y + h <= 48`.
- **Уникальность:** все пары `i < j` различны.
- **Зацикливание:** `playerMoveFrame(3) == (0)`, `(5) == (2)`, отрицательный индекс → кадр 0; `ghostBodyFrame(2) == (0)`, `(3) == (1)`, отрицательный → 0.
- **Clamp:** `playerDeathFrame(-1) == (0)`, `(4)` и `(100) == (3)`.
- **Конкретные координаты** для пары кадров, например `ghostEyesFrame(Up) == tileRect(3, 2)` и `None → Left`.

### 8. Форматирование
`python automation/format_files.py`. Заодно исчезнет пробельная строка, которая сейчас висит в `Game.h`.

## Проверка
1. `python automation/generate_assets.py` → код `0`, файл на диске.
2. `python automation/build_debug.py`: сборка без предупреждений (`/W4 /WX`).
3. Скрипты из задания с кодами возврата:
   - `python automation/run_tests.py` — все тесты зелёные, включая новые `PlayerTest.*` и `SpriteAtlasTest.*`;
   - `python automation/format_files.py --check`;
   - `python automation/run_tidy.py`;
   - `python automation/run_asan.py`.
4. По желанию пользователя запустить `GameApp` из корня (`assets/` берётся из текущей директории, в VS её задаёт `VS_DEBUGGER_WORKING_DIRECTORY`): Чомпер анимируется, поворачивается и замирает у стены и на паузе.
5. Коммит не делаю. Предложу команды `git add` (включая `assets/spritesheet.png`) и `git commit`; доставка через `/code-review` и PR из текущей ветки `feature/spritesheet-and-animation`.

## Замечания (сообщаю, не правлю)
- Если `GameApp` запущен не из корня, `LoadTexture` вернёт `id == 0`, и Чомпер не будет виден: `raylib` только напишет предупреждение в лог. Запасной вариант с жёлтым кругом при `id == 0` в задании не требуется.
- В `RaylibRenderer.h` уже есть `[[nodiscard]]`, хотя стиль курса его не одобряет. Это старое, в этой задаче не трогаю.
