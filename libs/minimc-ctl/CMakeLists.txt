project(minimc-ctl CXX)

add_subdirectory(src)
target_include_directories(minimc-ctl PUBLIC include)
target_link_libraries(minimc-ctl PUBLIC model)

INSTALL(TARGETS minimc-ctl)
