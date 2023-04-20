include(FetchContent)
find_package(Git REQUIRED)

FetchContent_Declare(ctl
  SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/ctl-src
  BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/ctl-build
  GIT_REPOSITORY https://github.com/sillydan1/ctl-expr.git
  GIT_TAG v2.0.1
)

if (NOT ctl_POPULATED)
  FetchContent_Populate(ctl)
  add_subdirectory(${ctl_SOURCE_DIR} ${ctl_BINARY_DIR})
endif()

