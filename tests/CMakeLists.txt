add_executable(quest_system_test quest_system_test.cc)
target_link_libraries(quest_system_test PRIVATE quests events)
target_include_directories(quest_system_test PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(NAME quest_system_test COMMAND quest_system_test)

add_executable(quest_marker_helpers_test quest_marker_helpers_test.cc)
target_link_libraries(quest_marker_helpers_test PRIVATE ui quests world SDL3::SDL3)
target_include_directories(quest_marker_helpers_test PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(NAME quest_marker_helpers_test COMMAND quest_marker_helpers_test)

add_executable(character_stats_input_test character_stats_input_test.cc)
target_link_libraries(character_stats_input_test PRIVATE ui SDL3::SDL3 SDL3_ttf::SDL3_ttf)
target_include_directories(character_stats_input_test PRIVATE ${CMAKE_SOURCE_DIR}/include)

add_test(NAME character_stats_input_test COMMAND character_stats_input_test)
