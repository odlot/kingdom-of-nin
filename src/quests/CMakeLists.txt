add_library(quests)

target_sources(quests
  PRIVATE
    quest_database.cc
    quest_helpers.cc
    quest_system.cc
  PUBLIC
    FILE_SET questHeaders
    TYPE HEADERS
    BASE_DIRS ${CMAKE_SOURCE_DIR}/include
    FILES
      ${CMAKE_SOURCE_DIR}/include/quests/quest.h
      ${CMAKE_SOURCE_DIR}/include/quests/quest_database.h
      ${CMAKE_SOURCE_DIR}/include/quests/quest_helpers.h
      ${CMAKE_SOURCE_DIR}/include/quests/quest_system.h
)

target_include_directories(quests PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(quests PRIVATE events)
