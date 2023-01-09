add_library(armloader OBJECT loader.cpp loader2.cpp converter.cpp converter.hpp)
target_link_libraries(armloader PUBLIC support model util PUBLIC parser)
target_include_directories(armloader PRIVATE ${PROJECT_SOURCE_DIR}/include)

