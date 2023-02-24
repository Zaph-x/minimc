add_library (ctpl STATIC ctlanalysis.cpp)

target_link_libraries (ctpl  )
target_include_directories(ctpl PUBLIC ${PROJECT_SOURCE_DIR}/include)
add_executable(ctplexe ctlanalysis.cpp)

INSTALL (TARGETS ctpl)
