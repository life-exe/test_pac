# Pacman
Игра Pac-Man на C++ — учебный проект курса по инженерному C++

## Сборка
```powershell
conan install . --output-folder=build -pr:h profiles/windows-msvc-debug -pr:b profiles/windows-msvc-debug --build=missing
conan install . --output-folder=build -pr:h profiles/windows-msvc-release -pr:b profiles/windows-msvc-release --build=missing
cmake -S . -B build "-DCMAKE_TOOLCHAIN_FILE=build/conan_toolchain.cmake"
cmake --build build --config Debug
```

## Запуск
Запускать из корня проекта: config.json читается из текущего каталога, а не из каталога с исполняемым файлом.

```powershell
.\build\Debug\game.exe
```