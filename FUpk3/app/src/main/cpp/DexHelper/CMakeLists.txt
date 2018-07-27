cmake_minimum_required(VERSION 3.4.1)

include_directories(.)


aux_source_directory(. RootSrc)

add_library(DexHelper STATIC ${RootSrc} )

target_link_libraries(DexHelper AndroidDef android c dl log Utils)

