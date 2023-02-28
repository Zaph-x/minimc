add_subdirectory(ctplalgorithms)
add_subdirectory(ctplformula)
add_subdirectory(ctplparser)

add_library (ctpl STATIC ctlanalysis.cpp)

target_link_libraries (ctpl PUBLIC  ctplalgorithms ctplformula ctplparser)

target_include_directories (ctpl PUBLIC ${PROJECT_SOURCE_DIR}/include)
