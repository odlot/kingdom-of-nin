add_library(ecs)
target_sources(ecs
  PRIVATE
    registry.cc
    system/system.cc
    system/graphic_system.cc
    system/movement_system.cc
    system/respawn_system.cc
    system/pushback_system.cc
  PUBLIC
    FILE_SET ecsHeaders
    TYPE HEADERS
    BASE_DIRS ${CMAKE_SOURCE_DIR}/include
    FILES
      ${CMAKE_SOURCE_DIR}/include/ecs/position.h
      ${CMAKE_SOURCE_DIR}/include/ecs/registry.h
      ${CMAKE_SOURCE_DIR}/include/ecs/system/system.h
      ${CMAKE_SOURCE_DIR}/include/ecs/system/movement_system.h
      ${CMAKE_SOURCE_DIR}/include/ecs/system/graphic_system.h
      ${CMAKE_SOURCE_DIR}/include/ecs/system/respawn_system.h
      ${CMAKE_SOURCE_DIR}/include/ecs/system/pushback_system.h
      ${CMAKE_SOURCE_DIR}/include/ecs/component/component.h
      ${CMAKE_SOURCE_DIR}/include/ecs/component/buff_component.h
      ${CMAKE_SOURCE_DIR}/include/ecs/component/class_component.h
      ${CMAKE_SOURCE_DIR}/include/ecs/component/collision_component.h
      ${CMAKE_SOURCE_DIR}/include/ecs/component/health_component.h
      ${CMAKE_SOURCE_DIR}/include/ecs/component/inventory_component.h
      ${CMAKE_SOURCE_DIR}/include/ecs/component/level_component.h
      ${CMAKE_SOURCE_DIR}/include/ecs/component/loot_component.h
      ${CMAKE_SOURCE_DIR}/include/ecs/component/mana_component.h
      ${CMAKE_SOURCE_DIR}/include/ecs/component/mob_component.h
      ${CMAKE_SOURCE_DIR}/include/ecs/component/movement_component.h
      ${CMAKE_SOURCE_DIR}/include/ecs/component/npc_component.h
      ${CMAKE_SOURCE_DIR}/include/ecs/component/projectile_component.h
      ${CMAKE_SOURCE_DIR}/include/ecs/component/shop_component.h
      ${CMAKE_SOURCE_DIR}/include/ecs/component/quest_log_component.h
      ${CMAKE_SOURCE_DIR}/include/ecs/component/pushback_component.h
      ${CMAKE_SOURCE_DIR}/include/ecs/component/skill_bar_component.h
      ${CMAKE_SOURCE_DIR}/include/ecs/component/skill_tree_component.h
      ${CMAKE_SOURCE_DIR}/include/ecs/component/equipment_component.h
      ${CMAKE_SOURCE_DIR}/include/ecs/component/stats_component.h
      ${CMAKE_SOURCE_DIR}/include/ecs/component/transform_component.h
      ${CMAKE_SOURCE_DIR}/include/ecs/component/graphic_component.h
)
target_include_directories(ecs PUBLIC ${CMAKE_SOURCE_DIR}/include)
target_link_libraries(ecs PRIVATE SDL3::SDL3 spdlog::spdlog items)
