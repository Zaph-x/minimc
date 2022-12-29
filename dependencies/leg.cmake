include(FetchContent)
find_package(Git REQUIRED)
FetchContent_Declare(
    leg
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/leg
    GIT_REPOSITORY https://github.com/Zaph-x/leg-assembly-loader
)

FetchContent_MakeAvailable (leg)
