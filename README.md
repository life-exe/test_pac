# Pacman
Игра Pac-Man на C++ — учебный проект курса по инженерному C++

## Сборка
```powershell
python automation/generate_project_files.py
python automation/build_debug.py
python automation/build_release.py
```

## Запуск
Запускать из корня проекта: config.json читается из текущего каталога, а не из каталога с исполняемым файлом.

```powershell
.\build\Debug\game.exe
```

## Очистка проекта

```powershell
python automation/clean.py --dry-run
python automation/clean.py
```

## Стиль кода

стиль задан файлом .clang-format

проверка

```powershell
python automation/format_files.py --check
```

исправление

```powershell
python automation/format_files.py
```