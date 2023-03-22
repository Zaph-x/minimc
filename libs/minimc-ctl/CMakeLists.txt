project(minimc-ctl CXX)

add_subdirectory(src)
target_include_directories(minimc-ctl PUBLIC include)

INSTALL(TARGETS minimc-ctl)
