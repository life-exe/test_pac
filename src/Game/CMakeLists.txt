# --- 1. GameCore (pure logic) ---
add_library(GameCore STATIC
    Core/Types.h
    Core/Game.h
    Core/Game.cpp
)
target_include_directories(GameCore PUBLIC Core)

# --- 2. GamePresentation (rendering and input) ---
add_library(GamePresentation STATIC
    Presentation/ConsoleRenderer.h
    Presentation/ConsoleRenderer.cpp
)
target_include_directories(GamePresentation PUBLIC Presentation)
target_link_libraries(GamePresentation PUBLIC GameCore)

# --- 3. GameApp (executable entry point) ---
add_executable(GameApp
    App/main.cpp
)
target_link_libraries(GameApp PRIVATE GamePresentation GameCore)

# --- 4. GameTests (core unit tests) ---
if(BUILD_TESTING)
    add_executable(GameTests
        Tests/GameTest.cpp
    )
    target_link_libraries(GameTests PRIVATE GameCore GTest::gtest_main)
    add_test(NAME GameCoreTests COMMAND GameTests)
endif()