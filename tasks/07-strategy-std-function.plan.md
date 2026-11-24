# План: задача 07b. Перевод стратегии GoF (`IGhostStrategy`) на `std::function`

## Контекст
- **Сейчас.** `GhostStrategy.h` содержит интерфейс `IGhostStrategy` и 4 класса `final`. `Ghost` хранит `std::unique_ptr<IGhostStrategy>`, а `Game.cpp` создаёт стратегии через `std::make_unique<...>()`.
- **Отличие от задания.** В задании функция называется `Game::createGhosts`. В коде это свободная функция `makeGhosts(const Maze&)` в анонимном пространстве имён `Game.cpp`. Правлю её и не переименовываю.
- **Где ещё встречаются эти типы.** `IGhostStrategy`, `*Strategy` и `unique_ptr` есть только в `GhostStrategy.*`, `Ghost.*`, `Game.cpp` и `GhostStrategyTest.cpp`. В `GameTest.cpp`, приёмочных тестах и `Presentation/` их нет.
- **Делегирующий конструктор.** `Ghost{config, nullptr}` продолжит компилироваться: у `std::function` есть конструктор от `nullptr_t`, он даёт пустую функцию. Проверка `if (m_mode == Chase && m_strategy)` тоже работает, через `explicit operator bool`.

## Шаги

### 1. Сохранить план
Сразу после утверждения записать его в `tasks/07-strategy-std-function.plan.md`.

### 2. `Core/GhostStrategy.h`
- Подключить `<functional>` рядом с `"Types.h"`.
- `GhostContext` оставить без изменений.
- Удалить `IGhostStrategy`, `RubyStrategy`, `RosieStrategy`, `IndigoStrategy` и `CoralStrategy`.
- Добавить `using GhostStrategy = std::function<GridPos(const GhostContext&)>;`.
- Объявить 4 свободные функции: `rubyStrategy`, `rosieStrategy`, `indigoStrategy`, `coralStrategy` с сигнатурой `GridPos(const GhostContext& context)`.

### 3. `Core/GhostStrategy.cpp`
- Анонимное пространство имён с `c_coralChaseRadiusSq{64}` и `squaredDistance` не трогаю.
- Переношу 4 формулы без изменений: `RubyStrategy::targetTile` превращается в `rubyStrategy` и так далее.

### 4. `Core/Ghost.h` и `Core/Ghost.cpp`
- Удалить `#include <memory>` из `Ghost.h`. Тип `GhostStrategy` приходит через `GhostStrategy.h`.
- Новая сигнатура: `Ghost(const GhostConfig& config, GhostStrategy strategy);`. Поле становится `GhostStrategy m_strategy;`.
- В списке инициализации оставить `m_strategy{std::move(strategy)}`. Без `std::move` clang-tidy сработает на `performance-unnecessary-value-param`. `<utility>` в `Ghost.cpp` уже подключён.
- В `resolveTarget` вызывать `return m_strategy(context);`.
- `explicit Ghost(const GhostConfig&)` и `update(const Maze&, float)` не меняются.

### 5. `Core/Game.cpp`
- В `makeGhosts` заменить `std::make_unique<RubyStrategy>()` на `rubyStrategy`, и так же для остальных трёх. Имя функции неявно превращается в указатель, а из него конструируется `std::function`.
- Удалить `#include <memory>`: после замены он не нужен. `<utility>` оставить, `std::move` используется.

### 6. `Tests/GhostStrategyTest.cpp`
- Удалить локальные объекты `const Core::XxxStrategy strategy;` и вызывать функции напрямую: `Core::rosieStrategy(makeContext(...))`.
- Все 11 тестов, их имена и ожидаемые значения сохраняются. Каждая из 4 функций и обе ветки `coral` остаются покрыты, поэтому покрытие `GhostStrategy.cpp` останется 100%.

### 7. Форматирование и проверки
Сначала `python automation/format_files.py`. Затем по очереди, сверяя код возврата каждого: `format_files.py --check`, `run_tests.py`, `run_tidy.py`, `run_asan.py`.

## Проверка
1. Отчитаюсь кодом возврата каждого скрипта.
2. Проверю, что в `src/` нигде не осталось `IGhostStrategy`, `make_unique` и `unique_ptr<IGhostStrategy`.
3. Проверю, что `GameCore` по-прежнему не зависит от `raylib`. `CMakeLists.txt` не меняется.
4. Коммит не делаю. Предложу команды `git add` и `git commit`, затем `/code-review` и PR из `feature/ghost-ai-strategy`.

## Замечания (сообщаю, но не правлю)
- **`Ghost` и `Game` станут копируемыми.** `std::function` копируется, `unique_ptr` — нет. Тест `static_assert(is_move_*<Game>)` по-прежнему проходит. `m_ghosts = makeGhosts(m_maze)` в `reset()` теперь может выбрать копирующее присваивание. Но правая часть — временный объект, так что выбирается перемещение.
- **Цена вызова.** Для указателя на функцию `std::function` обычно не выделяет память в куче (small buffer). Вызов идёт через стирание типа, по цене как виртуальный вызов. Поведение игры не меняется.
- **Порядок `#include` в `Game.cpp`.** Он уже неаккуратный: нет пустой строки между системными и проектными заголовками. Удалю только строку `<memory>`, остальное оставлю как есть, если `clang-format` сам не переставит.
