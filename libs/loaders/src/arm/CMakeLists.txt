add_library(armloader OBJECT loader2.cpp )
target_link_libraries(armloader PUBLIC support model util PUBLIC parser)
target_include_directories(armloader PRIVATE ${PROJECT_SOURCE_DIR}/include)

