find_package(raylib REQUIRED)

# --- 1. GameCore (pure logic) ---
add_library(GameCore STATIC
    Core/Types.h
    Core/Command.h
    Core/GameSnapshot.h
    Core/Game.h
    Core/Game.cpp
    Core/Level.h
    Core/Maze.h
    Core/Maze.cpp
    Core/GameConstants.h
    Core/GridMover.h
    Core/GridMover.cpp
    Core/Player.h
    Core/Player.cpp
    Core/Ghost.h
    Core/Ghost.cpp
    Core/GhostStrategy.cpp
    Core/RandomSource.h
)
target_include_directories(GameCore PUBLIC Core)

# --- 2. GamePresentation (rendering and input) ---
add_library(GamePresentation STATIC
    Presentation/ConsoleRenderer.h
    Presentation/ConsoleRenderer.cpp
    Presentation/RaylibRenderer.h
    Presentation/RaylibRenderer.cpp
    Presentation/SpriteAtlas.h
)
target_include_directories(GamePresentation PUBLIC Presentation)
# raylib is PUBLIC: RaylibRenderer.h holds a Texture2D, so its users need raylib headers too
target_link_libraries(GamePresentation PUBLIC GameCore raylib)
target_precompile_headers(GamePresentation PRIVATE <raylib.h>)

# --- 3. GameApp (executable entry point) ---
add_executable(GameApp
    App/Timer.h
    App/main.cpp
)
target_link_libraries(GameApp PRIVATE GamePresentation GameCore Common)

# --- 4. GameTests (core unit tests) ---
add_executable(GameTests
    Tests/GameTest.cpp
    Tests/TypesTest.cpp
    Tests/TestHelpers.h
    Tests/MazeTest.cpp
    Tests/PlayerTest.cpp
    Tests/GhostTest.cpp
    Tests/GhostStrategyTest.cpp
    Tests/SpriteAtlasTest.cpp
    Tests/Acceptance/KickoffAcceptanceTest.cpp
    Tests/Acceptance/MazeAcceptanceTest.cpp
    Tests/Acceptance/PlayerMovementAcceptanceTest.cpp
    Tests/Acceptance/SpritesheetAcceptanceTest.cpp
    Tests/Acceptance/PelletsAndScoreAcceptanceTest.cpp
    Tests/Acceptance/GhostNavigationAcceptanceTest.cpp
    Tests/Acceptance/GhostAiStrategyAcceptanceTest.cpp
    Tests/Acceptance/CollisionsAndLivesAcceptanceTest.cpp
    Tests/Acceptance/BuffsAndStateMachineAcceptanceTest.cpp
    Tests/Acceptance/GameScreensAndMenuAcceptanceTest.cpp
)
# Presentation is only an include path: SpriteAtlas.h is header-only and raylib-free, so tests stay off raylib
target_include_directories(GameTests PRIVATE Tests Presentation)
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

if(MSVC)
    target_link_options(GameApp PRIVATE /ignore:4099)
endif()
