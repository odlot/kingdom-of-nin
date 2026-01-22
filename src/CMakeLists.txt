FetchContent_Declare(SDL
    GIT_REPOSITORY https://github.com/libsdl-org/SDL.git
    GIT_TAG release-3.2.28
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SDL)

FetchContent_Declare(SDL_ttf
    GIT_REPOSITORY https://github.com/libsdl-org/SDL_ttf.git
    GIT_TAG release-3.2.0
    GIT_SHALLOW ON
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(SDL_ttf)

add_subdirectory(world)
add_subdirectory(items)
add_subdirectory(ecs)
add_subdirectory(ui)
add_subdirectory(events)
add_subdirectory(skills)
add_subdirectory(quests)

add_library(game)

target_sources(game
  PRIVATE
    game.cc
    camera.cc

  PUBLIC
    FILE_SET gameHeaders
    TYPE HEADERS
    BASE_DIRS
      ${CMAKE_SOURCE_DIR}/include
    FILES
      ${CMAKE_SOURCE_DIR}/include/game.h
      ${CMAKE_SOURCE_DIR}/include/camera.h
)

target_link_libraries(game PUBLIC SDL3_ttf::SDL3_ttf SDL3::SDL3 world ui items events skills quests
                           PRIVATE ecs spdlog::spdlog)
