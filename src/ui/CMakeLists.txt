add_library(ui)
target_sources(ui
  PRIVATE
    minimap.cc
    floating_text_system.cc
    inventory.cc
    character_stats.cc
    quest_log.cc
    text_utils.cc
    npc_dialog.cc
    shop_panel.cc
    render_utils.cc
    quest_marker_helpers.cc
    skill_bar.cc
    buff_bar.cc
    skill_tree.cc
  PUBLIC
    FILE_SET uiHeaders
    TYPE HEADERS
    BASE_DIRS ${CMAKE_SOURCE_DIR}/include
    FILES
      ${CMAKE_SOURCE_DIR}/include/ui/minimap.h
      ${CMAKE_SOURCE_DIR}/include/ui/floating_text_system.h
      ${CMAKE_SOURCE_DIR}/include/ui/inventory.h
      ${CMAKE_SOURCE_DIR}/include/ui/character_stats.h
      ${CMAKE_SOURCE_DIR}/include/ui/quest_log.h
      ${CMAKE_SOURCE_DIR}/include/ui/shop_panel.h
      ${CMAKE_SOURCE_DIR}/include/ui/render_utils.h
      ${CMAKE_SOURCE_DIR}/include/ui/text_utils.h
      ${CMAKE_SOURCE_DIR}/include/ui/npc_dialog.h
      ${CMAKE_SOURCE_DIR}/include/ui/quest_marker_helpers.h
      ${CMAKE_SOURCE_DIR}/include/ui/skill_bar.h
      ${CMAKE_SOURCE_DIR}/include/ui/buff_bar.h
      ${CMAKE_SOURCE_DIR}/include/ui/skill_tree.h
)
target_include_directories(ui PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(ui PRIVATE SDL3::SDL3 SDL3_ttf::SDL3_ttf world events items skills quests)
