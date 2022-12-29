add_subdirectory (llvm)
add_subdirectory (xxhash)
add_subdirectory (smtlib)
add_subdirectory (leg)

add_subdirectory (doctest EXCLUDE_FROM_ALL)
if (ENABLE_PYTHON)
add_subdirectory (pybind11)
endif()
