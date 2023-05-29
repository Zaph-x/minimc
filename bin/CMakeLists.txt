project (binary CXX)
FIND_PACKAGE(Boost COMPONENTS program_options REQUIRED)

if(ENABLE_SYMBOLIC)
    target_compile_options (support PUBLIC -DMINIMC_SYMBOLIC)
endif(ENABLE_SYMBOLIC)

add_subdirectory (interpreter)
find_program (NUSMV_EXECUTABLE nusmv)
if (NUSMV_EXECUTABLE)
    message (STATUS "Found nusmv: ${NUSMV_EXECUTABLE}")
    add_executable (minimc minimc.cpp enum.cpp mc.cpp plugin.cpp options.cpp smv.cpp)
else (NUSMV_EXECUTABLE)
    add_executable (minimc minimc.cpp enum.cpp mc.cpp plugin.cpp options.cpp)
endif (NUSMV_EXECUTABLE)

target_link_libraries  (minimc minimclib interpreter ${Boost_LIBRARIES})

INSTALL (TARGETS minimc )
