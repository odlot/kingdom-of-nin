add_library(ui)

target_sources(ui
  PRIVATE
    minimap.cc
    floating_text_system.cc
    inventory.cc
    character_stats.cc

  PUBLIC
    FILE_SET uiHeaders
    TYPE HEADERS
    BASE_DIRS
      ${CMAKE_SOURCE_DIR}/include
    FILES
      ${CMAKE_SOURCE_DIR}/include/ui/minimap.h
      ${CMAKE_SOURCE_DIR}/include/ui/floating_text_system.h
      ${CMAKE_SOURCE_DIR}/include/ui/inventory.h
      ${CMAKE_SOURCE_DIR}/include/ui/character_stats.h
)

target_include_directories(ui PUBLIC ${CMAKE_SOURCE_DIR}/include)

target_link_libraries(ui PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf world events items)
