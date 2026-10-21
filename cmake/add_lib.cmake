include_guard(GLOBAL)
#[[
if using LOCAL_DIR, the parameter REPOSITORY and VERSION will be ignored
if not using LOCAL_DIR, the parameter BIN_DIR will be ignored
]]
function(tx_add_lib)

	set(options "")
	set(oneValueArgs LIB_NAME LOCAL_DIR BIN_DIR VERSION REPOSITORY)
	set(multiValueArgs "")
	cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	set(LIB_NAME "${ARG_LIB_NAME}")
	message(STATUS "add_lib.cmake: Adding ${LIB_NAME}")

	set(${LIB_NAME}_SOURCE_DIR "")
	set(${LIB_NAME}_BINARY_DIR "")

	# resolve BIN_DIR
	if(ARG_BIN_DIR)
		message(VERBOSE "add_lib.cmake [${LIB_NAME}]: Using specialized bin dir: ${ARG_BIN_DIR}")
		set(${LIB_NAME}_BINARY_DIR "${ARG_BIN_DIR}/${LIB_NAME}")
	else()
		message(VERBOSE "add_lib.cmake [${LIB_NAME}]: Using default bin dir: \${CMAKE_CURRENT_BINARY_DIR}: ${CMAKE_CURRENT_BINARY_DIR}")
		set(${LIB_NAME}_BINARY_DIR "${CMAKE_CURRENT_BINARY_DIR}/${LIB_NAME}")
	endif()

	# prepare source
	set(REQUIRE_REMOTE_DOWNLOAD TRUE)

	if(ARG_LOCAL_DIR) # use local dir
		message(VERBOSE "add_lib.cmake [${LIB_NAME}]: Using local TXLib: ${ARG_LOCAL_DIR}")
		set(REQUIRE_REMOTE_DOWNLOAD FALSE)
		if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_LOCAL_DIR}") # look for in project dir first (relative path)
			set(${LIB_NAME}_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/${ARG_LOCAL_DIR}")
		elseif(EXISTS "${ARG_LOCAL_DIR}")
			set(${LIB_NAME}_SOURCE_DIR "${ARG_LOCAL_DIR}")
		else()
			# intented fallback to remote download
			message(VERBOSE "add_lib.cmake [${LIB_NAME}]: LOCAL_DIR path not found! Downloading remote repository.")
			set(REQUIRE_REMOTE_DOWNLOAD TRUE)
		endif()
	endif()

	if(REQUIRE_REMOTE_DOWNLOAD) # fetch from remote
		message(VERBOSE "add_lib.cmake [${LIB_NAME}]: Fetching ${LIB_NAME} from GitHub")		
	
		# resolve version
		if((NOT ARG_VERSION) OR (NOT ARG_REPOSITORY))
			message(FATAL_ERROR "add_lib.cmake [${LIB_NAME}]: Download requested but REPOSITORY or VERSION was not provided.")
		endif()

		# fetch content
		include(FetchContent)

		FetchContent_Declare(
			${LIB_NAME}
			GIT_REPOSITORY "${ARG_REPOSITORY}"
			GIT_TAG "${ARG_VERSION}"
		)

		FetchContent_MakeAvailable(${LIB_NAME})
		FetchContent_GetProperties(${LIB_NAME}
			SOURCE_DIR ${LIB_NAME}_SOURCE_DIR
        	BINARY_DIR ${LIB_NAME}_BINARY_DIR
		)
	else()
		add_subdirectory("${${LIB_NAME}_SOURCE_DIR}" "${${LIB_NAME}_BINARY_DIR}")
	endif()

	message(STATUS "add_lib.cmake [${LIB_NAME}]: Sources are ready.")
endfunction()