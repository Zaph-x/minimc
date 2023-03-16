include(FetchContent)
find_package(Git REQUIRED)

FetchContent_Declare(ctl-expr
        SOURCE_DIR ${CMAKE_CURRENT_BINARY_DIR}/ctl-expr-src
        BINARY_DIR ${CMAKE_CURRENT_BINARY_DIR}/ctl-expr-build
        GIT_REPOSITORY https://github.com/sillydan1/ctl-expr.git
        GIT_TAG v1.1.3
        )

if (NOT ctl-expr_POPULATED)
    FetchContent_Populate(ctl-expr)
    add_subdirectory(${ctl-expr_SOURCE_DIR} ${ctl-expr_BINARY_DIR})
endif()