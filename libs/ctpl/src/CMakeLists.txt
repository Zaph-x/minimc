add_library (ctpl STATIC ctl-checker.cpp)
target_include_directories (ctpl PUBLIC ${PROJECT_SOURCE_DIR}/include)
target_link_libraries (ctpl PUBLIC model)
