cmake_minimum_required(VERSION 3.4.1)

aux_source_directory(. RootSrc)

add_library(Utils STATIC ${RootSrc} )

target_link_libraries(Utils AndroidDef android c dl log)

