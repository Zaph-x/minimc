
add_executable (symbol symbol.cpp)
target_link_libraries (symbol doctest model)
add_test(NAME MODEL_SYMBOL COMMAND symbol)

add_executable (typecheck typechecker.cpp)
target_link_libraries (typecheck doctest model)
add_test(NAME typechec COMMAND typecheck)
	      

add_subdirectory (analysis)
