add_executable(quest_system_test quest_system_test.cc)
target_link_libraries(quest_system_test PRIVATE quests events)
target_include_directories(quest_system_test PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(NAME quest_system_test COMMAND quest_system_test)
