function(GetCompilerRootPath path compiler language)
	# Run the compiler with an empty input and get the error stream
	# -x: Choose language (e.g. "c",c++","objective-c",...)
	# -E: Preprocess only
	# -v: Print info about stages of execution on stderr
	execute_process(COMMAND "echo"
		COMMAND "${compiler}" "-x" "${language}" "-E" "-v" "-"
		ERROR_VARIABLE erroutstring
		OUTPUT_QUIET)

	#Convert the output to a list using a non-convential seperator
	string(ASCII 27 Esc)
	string(REGEX REPLACE "\n" "${Esc};" erroutlist "${erroutstring}")

	#Clear temporary list
	set(tmppath "")

	set(_ininclude OFF)
	foreach(line ${erroutlist})
		#Clean up line. Get rid of extra seperator.
		string(REGEX REPLACE "${Esc}" "" line "${line}")
		string(STRIP "${line}" line)

		#Look for the end of the block containing the include path.
		if (line MATCHES "^End of search list.$")
			set(_ininclude OFF)
		endif()

		if (${_ininclude})
			if ("${line}" MATCHES "include$")
				get_filename_component(directory "${line}" DIRECTORY)
				list(APPEND tmppath "${directory}")
			endif()
		endif()

		#Look for the start of the block containing the include path.
		#We're only interested in the <...> type includes.
		if (line MATCHES "^#include <...> search starts here:$")
			set(_ininclude ON)
		endif()
	endforeach(line)

	#Set the output variable
	set("${path}" "${tmppath}" PARENT_SCOPE)

endfunction(GetCompilerRootPath)

