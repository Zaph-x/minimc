
add_library(minimc-ctl STATIC ctl.cpp ctl-checker.cpp)

target_link_libraries(minimc-ctl PUBLIC ctl host model)

