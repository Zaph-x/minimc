add_library (loader SHARED loader.cpp)
add_subdirectory (llvm)
add_subdirectory (mmc)
add_subdirectory (arm)

target_link_libraries (loader PUBLIC minimcloader llvmloader armloader)
target_include_directories(loader PUBLIC ${PROJECT_SOURCE_DIR}/include)

INSTALL (TARGETS loader)
