# Compiler warnings of the netcodec targets, from cpp-best-practices/cmake_template (cmake/CompilerWarnings.cmake),
# reduced to C++ and to the three compilers of the project.
function(netcodec_set_warnings target warnings_as_errors)
	set(msvc_warnings
		/W4
		/w14242 # conversion, possible loss of data
		/w14254 # operator conversion of bit fields, possible loss of data
		/w14263 # member function does not override any base class virtual member function
		/w14265 # class has virtual functions, but its destructor is not virtual
		/w14287 # unsigned / negative constant mismatch
		/we4289 # loop control variable used outside the for-loop scope
		/w14296 # expression is always true or false
		/w14311 # pointer truncation
		/w14545 /w14546 /w14547 /w14549 # suspicious comma expressions
		/w14555 # expression has no effect
		/w14619 # pragma warning: there is no warning number
		/w14640 # thread-unsafe static member initialisation
		/w14826 # sign-extended conversion
		/w14905 /w14906 # string literal cast to LPSTR / LPWSTR
		/w14928 # illegal copy-initialisation
		/permissive-)
	set(clang_warnings
		-Wall -Wextra -Wpedantic
		-Wshadow
		-Wnon-virtual-dtor
		-Wold-style-cast
		-Wcast-align
		-Wunused
		-Woverloaded-virtual
		-Wconversion
		-Wsign-conversion
		-Wnull-dereference
		-Wdouble-promotion
		-Wformat=2
		-Wimplicit-fallthrough)
	set(gcc_warnings
		${clang_warnings}
		-Wmisleading-indentation
		-Wduplicated-cond
		-Wduplicated-branches
		-Wlogical-op
		-Wuseless-cast
		-Wsuggest-override)
	if(warnings_as_errors)
		list(APPEND msvc_warnings /WX)
		list(APPEND clang_warnings -Werror)
		list(APPEND gcc_warnings -Werror)
	endif()
	if(MSVC)
		target_compile_options(${target} INTERFACE ${msvc_warnings})
	elseif(CMAKE_CXX_COMPILER_ID MATCHES ".*Clang")
		target_compile_options(${target} INTERFACE ${clang_warnings})
	elseif(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
		target_compile_options(${target} INTERFACE ${gcc_warnings})
	else()
		message(AUTHOR_WARNING "netcodec: no warning set for compiler '${CMAKE_CXX_COMPILER_ID}'")
	endif()
endfunction()
