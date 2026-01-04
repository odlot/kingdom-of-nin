add_library(items)

target_sources(items
  PRIVATE
    item_database.cc

  PUBLIC
    FILE_SET itemsHeaders
    TYPE HEADERS
    BASE_DIRS
      ${CMAKE_SOURCE_DIR}/include
    FILES
      ${CMAKE_SOURCE_DIR}/include/items/item.h
      ${CMAKE_SOURCE_DIR}/include/items/item_database.h
)

target_include_directories(items PUBLIC ${CMAKE_SOURCE_DIR}/include)
