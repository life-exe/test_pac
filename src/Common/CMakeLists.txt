                                                                                                                                                         
project(Common LANGUAGES CXX)                                                                                                                            
                                                                                                                                                             
# 1. Статическая библиотека конфигурации                                                                                                                 
add_library(Common STATIC                                                                                                                                
        Config.cpp                                                                                                                                           
        Config.h                                                                                                                                             
)                                                                                                                                                        
                                                                                                                                                             
# Делаем текущую папку доступной для include всем, кто линкует Common                                                                                    
target_include_directories(Common PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})                                                                                    
# Линкуем парсер JSON nlohmann_json публично, так как типы json видны в интерфейсе                                                                       
target_link_libraries(Common PUBLIC nlohmann_json::nlohmann_json)                                                                                        
                                                                                                                                                             
set_target_properties(Common PROPERTIES FOLDER Common)                                                                                                   
set_project_warnings(Common)                                                                                                                             
enable_sanitizers(Common)                                                                                                                                
                                                                                                                                                             
# 2. Модульные тесты модуля Common                                                                                                                       
add_executable(CommonTests Tests/ConfigTest.cpp)                                                                                                         
target_link_libraries(CommonTests PRIVATE Common GTest::gtest_main)                                                                                      
set_target_properties(CommonTests PROPERTIES FOLDER Common)                                                                                              
set_project_warnings(CommonTests)                                                                                                                        
enable_sanitizers(CommonTests)                                                                                                                           
                                                                                                                                                             
include(GoogleTest)                                                                                                                                      
gtest_discover_tests(CommonTests DISCOVERY_TIMEOUT 30)   