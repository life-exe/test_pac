==================== TASK FOR AGENT: project/tasks/07-strategy-std-function.md ====================
# Задача 07b. Рефакторинг классической GoF-стратегии (`IGhostStrategy`) на `std::function`

## Цель
Заменить иерархию классов `IGhostStrategy` / `RubyStrategy` / `RosieStrategy` / `IndigoStrategy` / `CoralStrategy` и `std::unique_ptr<IGhostStrategy>` на современный C++-подход: `using GhostStrategy = std::function<GridPos(const GhostContext&)>;` и четыре свободные функции. Поведение игры и математика всех четырёх стратегий не меняются.

## Контракты (менять нельзя)
- Структуру `GhostContext` и её поля (`playerPos`, `playerDir`, `rubyPos`, `ghostPos`, `scatterCorner`) не изменяй.
- Публичный контракт `Game` (`update`, `handleCommand`, `reset`, `snapshot`), волновой таймер и прежнюю перегрузку `Ghost::update(const Maze&, float)` сохрани без изменений.
- Конструктор `explicit Ghost(const GhostConfig& config)` сохрани (делегирует в `Ghost{config, nullptr}`).

## Что нужно сделать агенту
1. Сохрани краткий пошаговый план рефакторинга в файл `tasks/07-strategy-std-function.plan.md` (в `Plan Mode` покажи план на согласование, а сразу после утверждения запиши его в этот файл).
2. Обнови `src/Game/Core/GhostStrategy.h`:
   - подключи `<functional>`;
   - удали `IGhostStrategy`, `RubyStrategy`, `RosieStrategy`, `IndigoStrategy`, `CoralStrategy`;
   - объяви псевдоним `using GhostStrategy = std::function<GridPos(const GhostContext&)>;`;
   - объяви четыре свободные функции в `namespace LifeExe::Core`:
     - `GridPos rubyStrategy(const GhostContext& context);`
     - `GridPos rosieStrategy(const GhostContext& context);`
     - `GridPos indigoStrategy(const GhostContext& context);`
     - `GridPos coralStrategy(const GhostContext& context);`
3. Обнови `src/Game/Core/GhostStrategy.cpp`: перенеси реализации всех четырёх формул в свободные функции `rubyStrategy`, `rosieStrategy`, `indigoStrategy`, `coralStrategy`.
4. Обнови `src/Game/Core/Ghost.h` и `src/Game/Core/Ghost.cpp`:
   - убери неиспользуемый `#include <memory>` из `Ghost.h`;
   - замени `std::unique_ptr<IGhostStrategy>` на `GhostStrategy` в конструкторе `Ghost(const GhostConfig& config, GhostStrategy strategy)` и в поле `GhostStrategy m_strategy;`;
   - в `Ghost::resolveTarget(const GhostContext& context) const` вызывай `m_strategy(context)` вместо `m_strategy->targetTile(context)`.
5. Обнови `src/Game/Core/Game.cpp`:
   - в `Game::createGhosts` передай `rubyStrategy`, `rosieStrategy`, `indigoStrategy`, `coralStrategy` напрямую вместо `std::make_unique<...>()`;
   - убери `#include <memory>`, если он больше не используется в `Game.cpp`.
6. Обнови `src/Game/Tests/GhostStrategyTest.cpp`: переведи тесты на вызов свободных функций `Core::rubyStrategy`, `Core::rosieStrategy`, `Core::indigoStrategy`, `Core::coralStrategy` (сохранив все проверки и 100% покрытие `GhostStrategy.cpp`).

## Ограничения и приёмка
- `GameCore` не должен зависеть от `raylib`.
- Соблюдай `AGENTS.md`; после рефакторинга сам прогони `format_files.py --check`, `run_tests.py`, `run_tidy.py` и `run_asan.py`.
===================================================================================================
