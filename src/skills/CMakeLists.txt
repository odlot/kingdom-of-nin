add_library(skills)

target_sources(skills
  PRIVATE
    skill_database.cc
    skill_tree.cc

  PUBLIC
    FILE_SET skillsHeaders
    TYPE HEADERS
    BASE_DIRS
      ${CMAKE_SOURCE_DIR}/include
    FILES
      ${CMAKE_SOURCE_DIR}/include/skills/skill.h
      ${CMAKE_SOURCE_DIR}/include/skills/skill_database.h
      ${CMAKE_SOURCE_DIR}/include/skills/skill_tree.h
)

target_include_directories(skills PUBLIC ${CMAKE_SOURCE_DIR}/include)
