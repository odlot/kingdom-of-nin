find_package(OpenCV REQUIRED COMPONENTS core imgproc imgcodecs) 

message(STATUS "Found OpenCV ${OpenCV_VERSION}: ${OpenCV_LIBS}")

add_library(world)

target_sources(world
  PRIVATE
    map.cc
    generator.cc

  PUBLIC
    FILE_SET worldHeaders
    TYPE HEADERS
    BASE_DIRS
      ${CMAKE_SOURCE_DIR}/include
    FILES
      ${CMAKE_SOURCE_DIR}/include/world/map.h
      ${CMAKE_SOURCE_DIR}/include/world/generator.h
      ${CMAKE_SOURCE_DIR}/include/world/region.h
      ${CMAKE_SOURCE_DIR}/include/world/tile.h
)

target_include_directories(world PUBLIC ${OpenCV_INCLUDE_DIRS})
target_link_libraries(world PRIVATE ${OpenCV_LIBS})
