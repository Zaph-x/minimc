add_library (armloader OBJECT loader.cpp )
target_link_libraries (armloader PUBLIC model)
target_include_directories(armloader PRIVATE ${PROJECT_SOURCE_DIR}/include)
