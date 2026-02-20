add_library(mobs)
target_sources(mobs
  PRIVATE
    mob_database.cc
  PUBLIC
    FILE_SET mobsHeaders
    TYPE HEADERS
    BASE_DIRS ${CMAKE_SOURCE_DIR}/include
    FILES
      ${CMAKE_SOURCE_DIR}/include/mobs/mob_database.h
)

target_include_directories(mobs PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(mobs PUBLIC SDL3::SDL3 items)
