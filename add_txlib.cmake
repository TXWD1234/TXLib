include_guard(GLOBAL)
#[[
if using LOCAL_DIR, the parameter VERSION will be ignored
if not using LOCAL_DIR, the parameter BIN_DIR will be ignored
]]
function(tx_add_txlib)
	message(STATUS "TXLib: Adding TXLib")

	set(options "")
    set(oneValueArgs LOCAL_DIR BIN_DIR VERSION)
    set(multiValueArgs COMPONENTS)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	set(TXLib_SOURCE_DIR "")
	set(TXLib_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}")

	# resolve BIN_DIR
	if(ARG_BIN_DIR)
		message(STATUS "TXLib: Using specialized bin dir")
		set(TXLib_BINARY_DIR "${ARG_BIN_DIR}")
	else()
		message(STATUS "TXLib: Using default bin dir: \${CMAKE_CURRENT_BINARY_DIR}: ${CMAKE_CURRENT_BINARY_DIR}")
	endif()

	# prepare source
	set(REQUIRE_REMOTE_DOWNLOAD TRUE)

	if(ARG_LOCAL_DIR) # use local dir
		message(STATUS "TXLib: Using local TXLib")
		set(REQUIRE_REMOTE_DOWNLOAD FALSE)
		if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_LOCAL_DIR}") # look for in project dir first (relative path)
			set(TXLib_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_LOCAL_DIR}")
		elseif(EXISTS "${ARG_LOCAL_DIR}")
			set(TXLib_SOURCE_DIR "${ARG_LOCAL_DIR}")
		else()
			# intented fallback to remote download
			message(STATUS "TXLib: LOCAL_DIR path not found! downloading TXLib.")
			set(REQUIRE_REMOTE_DOWNLOAD TRUE)
		endif()
		set(TXLib_BINARY_DIR "${TXLib_BINARY_DIR}/TXLib")
	endif()

	if(REQUIRE_REMOTE_DOWNLOAD) # fetch from remote
		message(STATUS "TXLib: Fetching TXLib from GitHub")		
	
		# resolve version
		if(ARG_VERSION)
			set(GIT_TAG_VAL "${ARG_VERSION}")
		else()
			set(GIT_TAG_VAL "main")
		endif()

		# fetch content
		include(FetchContent)

		FetchContent_Declare(
			txlib
			GIT_REPOSITORY "https://github.com/TXWD1234/TXLib.git"
			GIT_TAG "${GIT_TAG_VAL}"
			SOURCE_SUBDIR "TXCMakeUtils"
		)
		# use `SOURCE_SUBDIR` to work around the MakeAvailable's auto `add_subdirectory`

		FetchContent_MakeAvailable(txlib)
		FetchContent_GetProperties(txlib)

		set(TXLib_SOURCE_DIR "${txlib_SOURCE_DIR}")
		set(TXLib_BINARY_DIR "${txlib_BINARY_DIR}")
	endif()
	
	# <-------------------------------------------------
	# Sources are ready; TXLib is locally exist;
	# call TXLib_SOURCE_DIR/TXCMakeUtilis/impl/install.cmake or something like that

	# everything below should be in that install.cmake 



	# TXLib settings variables
	set(TXLib_CXX_VERSION "cxx_std_20")

	# load TXLib module registry
	# <------------------------------

	# include TXLib cmake utilities

	# resolve components
	if(ARG_COMPONENTS) # if given components
		foreach(comp IN LISTS ARG_COMPONENTS)
			if(EXISTS "${TXLib_SOURCE_DIR}/${comp}/CMakeLists.txt")
				message(STATUS "TXLib: Adding component [${comp}]")
				add_subdirectory("${TXLib_SOURCE_DIR}/${comp}" "${TXLib_BINARY_DIR}/${comp}")
			else()
				message(WARNING "TXLib: Component [${comp}] not found in source!")
			endif()
		endforeach()
	else() # add the whole library
		message(STATUS "TXLib: Adding TXLib")
		add_subdirectory("${TXLib_SOURCE_DIR}" "${TXLib_BINARY_DIR}")
	endif()
	
	set(TXLib_SOURCE_DIR "${TXLib_SOURCE_DIR}" PARENT_SCOPE)
endfunction()