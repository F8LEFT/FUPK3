
aux_source_directory(libdex SRC_LIBDEX)
aux_source_directory(dexdump SRC_DEXDUMP)
aux_source_directory(vm SRC_VM)
aux_source_directory(vm/oo SRC_VM_OO)
aux_source_directory(vm/native SRC_VM_NATIVE)


set(SOURCE_FILES ${SRC_LIBDEX} ${SRC_DEXDUMP} ${SRC_VM} ${SRC_VM_OO} ${SRC_VM_NATIVE} BaseDef.cpp)
add_library(AndroidDef STATIC ${SOURCE_FILES})
target_link_libraries(AndroidDef z log)

target_include_directories(AndroidDef PUBLIC .)
