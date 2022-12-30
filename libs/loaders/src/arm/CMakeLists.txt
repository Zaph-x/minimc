add_library (armloader OBJECT loader.cpp context.cpp)
target_link_libraries (armloader PUBLIC model PRIVATE leg-assembly-loader)
target_include_directories(armloader PRIVATE ${PROJECT_SOURCE_DIR}/include )