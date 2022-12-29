include(FetchContent)
find_package(Git REQUIRED)
FetchContent_Declare(
    leg
    PREFIX ${CMAKE_CURRENT_BINARY_DIR}/leg
    GIT_REPOSITORY https://github.com/Zaph-x/leg-assembly-loader
    Git_TAG 2b6e503f13d769fb5fdf706f23ba12dc3bffe233
)

FetchContent_MakeAvailable (leg)
