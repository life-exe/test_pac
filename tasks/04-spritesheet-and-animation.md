==================== TASK FOR AGENT: project/tasks/04-spritesheet-and-animation.md ====================
# Задача 04. Генератор `assets/spritesheet.png`, отрисовка Чомпера в `RaylibRenderer` и тесты `SpriteAtlas`

## Контракты (менять нельзя)
- Файлы `Core/Types.h`, `Core/Command.h`, `Core/GameConstants.h`, `Core/Maze.*`, `Core/GridMover.*`, `Core/Player.*`, `Core/GameSnapshot.h` и `Presentation/SpriteAtlas.h` написаны вручную — не изменяй их.
- Раскладка атласа `assets/spritesheet.png` жёстко задана в `Presentation/SpriteAtlas.h`: 8-битный RGBA PNG размером `112x48` пикселей (`7x3` тайлов по `16x16`), где заполнены все 18 обязательных кадров:
  - строка `0`: столбцы `0..2` — Чомпер в движении (рот закрыт, полуоткрыт, широко открыт; смотрит вправо), столбцы `3..6` — 4 фазы схлопывания Чомпера;
  - строка `1`: столбцы `0..1` — белое тело призрака (2 фазы юбки, окрашивается через `tint`), столбцы `2..3` — испуганный призрак (синий и белый мигающий);
  - строка `2`: столбцы `0..3` — глаза призрака (`Right`, `Down`, `Left`, `Up`), столбец `4` — точка (`Pellet`), столбец `5` — энерджайзер (`PowerPellet`), столбец `6` — иконка жизни для HUD.

## Что нужно сделать агенту
1. Сохрани краткий пошаговый план реализации в файл `tasks/04-spritesheet-and-animation.plan.md` (в `Plan Mode` покажи план на согласование, а сразу после утверждения запиши его в этот файл).
2. Создай скрипт `automation/generate_assets.py` только на стандартной библиотеке Python (`math`, `pathlib`, `struct`, `zlib`, без сторонних `pip`-пакетов), который процедурно рисует и сохраняет `assets/spritesheet.png` (`112x48` RGBA, все 18 кадров непустые). Запусти скрипт, чтобы файл `assets/spritesheet.png` появился на диске. Также добавь копирование папки `assets` в `dist/assets` в шаг `Package game` (Windows и Linux) файла `.github/workflows/build.yml`.
3. Обнови `Presentation/RaylibRenderer.h` и `Presentation/RaylibRenderer.cpp`:
   - добавь `#include <raylib.h>` и поле `Texture2D m_spritesheet{};` в `RaylibRenderer.h`;
   - в `RaylibRenderer::init(...)` после `InitWindow` загружай текстуру `LoadTexture("assets/spritesheet.png")` и включай пиксельный фильтр `SetTextureFilter(m_spritesheet, TEXTURE_FILTER_POINT)`;
   - в `RaylibRenderer::close()` перед `CloseWindow()` освобождай текстуру через `UnloadTexture(m_spritesheet)` (если `m_spritesheet.id != 0`) и обнуляй `m_spritesheet = {}`;
   - замени временный жёлтый круг Чомпера на отрисовку кадра `SpriteAtlas::playerMoveFrame(...)` через `DrawTexturePro` (переключай кадры по `snapshot.player.animTime`, например 10–12 кадров/с; поворачивай по `snapshot.player.facing`: `Right = 0°`, `Down = 90°`, `Left = 180°`, `Up = 270°` вокруг центра тайла `origin = {size * 0.5f, size * 0.5f}` с `dest = {centerX, centerY, size, size}`).
4. В `src/Game/CMakeLists.txt` добавь `Presentation` в `target_include_directories(GameTests PRIVATE Tests Presentation)` (без линковки `GamePresentation` к `GameTests`!).
5. Дополни `src/Game/Tests/PlayerTest.cpp` проверками `animTime` (рост во время движения, заморозка при остановке у стены и на паузе, сброс в `0.0f` при `Reset`).
6. Создай `src/Game/Tests/SpriteAtlasTest.cpp` (проверка размеров констант атласа, вхождения всех 18 кадров в границы `112x48`, попарной уникальности всех 18 прямоугольников, зацикливания `playerMoveFrame`/`ghostBodyFrame` и `clamp` в `playerDeathFrame`) и подключи его к `GameTests` в `src/Game/CMakeLists.txt`.

## Ограничения и приёмка
- `GameCore` и `Presentation/SpriteAtlas.h` не должны зависеть от `raylib`.
- Соблюдай `AGENTS.md`; после реализации сам прогони `format_files.py --check`, `run_tests.py`, `run_tidy.py` и `run_asan.py`.
=======================================================================================================
