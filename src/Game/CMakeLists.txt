# --- 1. GameCore (pure logic) ---
add_library(GameCore STATIC
    Core/Types.h
    Core/Command.h
    Core/Game.h
    Core/Game.cpp
)
target_include_directories(GameCore PUBLIC Core)

# --- 2. GamePresentation (rendering and input) ---
add_library(GamePresentation STATIC
    Presentation/ConsoleRenderer.h
    Presentation/ConsoleRenderer.cpp
    Presentation/SdlRenderer.h
    Presentation/SdlRenderer.cpp
)
find_package(SDL3 REQUIRED)
target_include_directories(GamePresentation PUBLIC Presentation)
target_link_libraries(GamePresentation PUBLIC GameCore PRIVATE SDL3::SDL3)
target_compile_definitions(GamePresentation PRIVATE SDL_MAIN_HANDLED)
target_precompile_headers(GamePresentation PRIVATE <SDL3/SDL.h>)

# --- 3. GameApp (executable entry point) ---
add_executable(GameApp
    App/Timer.h
    App/main.cpp
)
target_include_directories(GameApp PRIVATE App)
target_link_libraries(GameApp PRIVATE GamePresentation GameCore Common)

# --- 4. GameTests (core unit tests) ---
add_executable(GameTests
    Tests/GameTest.cpp
)
target_link_libraries(GameTests PRIVATE GameCore GTest::gtest_main)

# --- Properties, IDE folders and debugger ---
set_target_properties(GameCore GamePresentation GameApp GameTests PROPERTIES FOLDER Game)
set_target_properties(GameApp PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")

# --- Warnings (/W4 /WX) ---
set_project_warnings(GameCore)
set_project_warnings(GamePresentation)
set_project_warnings(GameApp)
set_project_warnings(GameTests)

# --- Sanitizers (ASan) ---
enable_sanitizers(GameCore)
enable_sanitizers(GamePresentation)
enable_sanitizers(GameApp)
enable_sanitizers(GameTests)

# --- CTest & GTest test discovery ---
include(GoogleTest)
gtest_discover_tests(GameTests DISCOVERY_TIMEOUT 30)