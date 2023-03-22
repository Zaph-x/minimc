include(FetchContent)
find_package(Git REQUIRED)

FetchContent_Declare(leg-assembly-loader
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/leg-src
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/leg-build
  GIT_REPOSITORY https://github.com/Zaph-x/leg-assembly-loader.git
  GIT_TAG origin/main
)

if (NOT leg-assembly-loader_POPULATED)
  FetchContent_Populate(leg-assembly-loader)
  add_subdirectory(${leg-assembly-loader_SOURCE_DIR} ${leg-assembly-loader_BINARY_DIR})
endif()

