add_library(events)

target_sources(events
  PRIVATE
    event_bus.cc

  PUBLIC
    FILE_SET eventsHeaders
    TYPE HEADERS
    BASE_DIRS
      ${CMAKE_SOURCE_DIR}/include
    FILES
      ${CMAKE_SOURCE_DIR}/include/events/event_bus.h
      ${CMAKE_SOURCE_DIR}/include/events/events.h
)

target_include_directories(events PUBLIC ${CMAKE_SOURCE_DIR}/include)
