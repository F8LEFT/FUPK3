project(Fupk3)
cmake_minimum_required(VERSION 3.4.1)

add_definitions(-DNO_LOCALE_SUPPORT=True)

include_directories(.)

add_subdirectory(AndroidDef)
add_subdirectory(utils)
add_subdirectory(DexHelper)

aux_source_directory(. RootSrc)

add_library(Fupk3 SHARED ${RootSrc} )

target_link_libraries(Fupk3 AndroidDef DexHelper android c dl log Utils)

