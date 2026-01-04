cmake_minimum_required(VERSION 3.28.3)

project(kingdom_of_nin LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(SPDLOG_BUILD_PIC ON)

add_executable(kingdom_of_nin)

target_sources(kingdom_of_nin
  PRIVATE
    main.cc
)

include(FetchContent)
FetchContent_Declare(spdlog
    GIT_REPOSITORY https://github.com/gabime/spdlog.git
    GIT_TAG v1.16.0
    GIT_SHALLOW ON  
    EXCLUDE_FROM_ALL
    SYSTEM)
FetchContent_MakeAvailable(spdlog)

add_subdirectory(src)

target_compile_features(kingdom_of_nin PRIVATE cxx_std_20)
target_link_libraries(kingdom_of_nin PRIVATE game spdlog::spdlog)

option(KINGDOM_OF_NIN_ENABLE_CLANG_TIDY "Enable clang-tidy for project targets" OFF)
if(KINGDOM_OF_NIN_ENABLE_CLANG_TIDY)
  find_program(CLANG_TIDY_EXE NAMES clang-tidy)
  if(CLANG_TIDY_EXE)
    set(CLANG_TIDY_COMMAND "${CLANG_TIDY_EXE}" "-p=${CMAKE_BINARY_DIR}")
    if(TARGET kingdom_of_nin)
      set_property(TARGET kingdom_of_nin PROPERTY CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
    endif()
    if(TARGET game)
      set_property(TARGET game PROPERTY CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
    endif()
    if(TARGET ecs)
      set_property(TARGET ecs PROPERTY CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
    endif()
    if(TARGET world)
      set_property(TARGET world PROPERTY CXX_CLANG_TIDY "${CLANG_TIDY_COMMAND}")
    endif()
  else()
    message(WARNING "clang-tidy not found; install it or disable KINGDOM_OF_NIN_ENABLE_CLANG_TIDY.")
  endif()
endif()

add_custom_command(TARGET kingdom_of_nin POST_BUILD
  COMMAND ${CMAKE_COMMAND} -E copy_directory
    ${CMAKE_SOURCE_DIR}/assets
    $<TARGET_FILE_DIR:kingdom_of_nin>/assets
)
