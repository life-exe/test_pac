project(Pacman LANGUAGES CXX)                                                                                                                            
                                                                                                                                                             
add_executable(${PROJECT_NAME} main.cpp)                                                                                                                 
target_link_libraries(${PROJECT_NAME} PRIVATE Common)                                                                                                    
                                                                                                                                                             
set_target_properties(${PROJECT_NAME} PROPERTIES FOLDER ${PROJECT_NAME})                                                                                 
set_target_properties(${PROJECT_NAME} PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")                                                    
set_project_warnings(${PROJECT_NAME})                                                                                                                    
enable_sanitizers(${PROJECT_NAME})