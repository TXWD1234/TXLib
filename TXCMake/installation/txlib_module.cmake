include_guard(GLOBAL)

include("${TXLib_INSTALLATION_DIR}/log.cmake")
include("${TXLib_INSTALLATION_DIR}/../set_compile_flags.cmake")

#[[
LIB_TYPE
(Mandatory)
Library type of the module, either STATIC or INTERFACE

PUBLIC_HEADERS
(Mandatory)
The public headers of the module, which will be included by the user.
List the files in `${Module}/include/tx/`. These files will be relative to `${Module}/include/tx/`
(The path `${Module}/include/tx/` will be added before the path provided)
The file listed here should only contain `.h` and `.hpp` files.
They will be in PUBLIC scope.

IMPL_HEADERS
The implementation headers of the module, which will be included by PUBLIC_HEADERS, potentially used by SOURCES.
List the files in `${Module}/include/impl/`. These files will be relative to `${Module}/include/impl/`
(The path `${Module}/include/impl/` will be added before the path provided)
The file listed here should only contain `.h` and `.hpp` files.
They will be in PUBLIC scope, since they need to be visible for user to use PUBLIC_HEADERS that includes them.
But they should not be used by the user.

SOURCES
(Mandatory for LIB_TYPE == STATIC)
The static source of the module, which will be compiled into binary.
List the files in `${Module}/src/`. These files will be relative to `${Module}/src/`
(The path `${Module}/src/` will be added before the path provided)
The file listed here should only contain `.cpp` files.

Note:
The name of the module is already setted by the registry
]]
function(tx_txlib_module)

	set(options "")
    set(oneValueArgs LIB_TYPE)
    set(multiValueArgs PUBLIC_HEADERS IMPL_HEADERS SOURCES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})
	
	if(NOT TXLib_INSTALLATION_MODULE_NAME)
		tx_error_log(FALSE
			"Variable TXLib_INSTALLATION_MODULE_NAME does not exist."
		    "  Hint: Not added by `add_txlib.cmake` nor `setup.cmake`.")
	endif()
	set(TXLib_MODULE_NAME "${TXLib_INSTALLATION_MODULE_NAME}")

	# parameter integrity check
	if(NOT ARG_LIB_TYPE)
		tx_error_log("${TXLib_MODULE_NAME}" "Failed to declare module. Missing mandatory parameter: LIB_TYPE.")
	endif()
	if(NOT ARG_PUBLIC_HEADERS)
		tx_error_log("${TXLib_MODULE_NAME}" "Failed to declare module. Missing mandatory parameter: PUBLIC_HEADERS.")
	endif()

	# to defend against being included multiple times
	if(TARGET ${TXLib_MODULE_NAME})
		return()
	endif()
	# to defend against invalid add_subdirectories calls not from add_txlib.cmake
	if(NOT TXLib_SOURCE_DIR OR NOT TXLib_BINARY_DIR)
		tx_error_log("${TXLib_MODULE_NAME}" "Variable TXLib_SOURCE_DIR or TXLib_BINARY_DIR does not exist."
		                                    "  Hint: Not added by `add_txlib.cmake`.")
	endif()
	
	# start target configuration

	set(TXLib_MODULE_DIR ${TXLib_${TXLib_MODULE_NAME}_SOURCE_DIR})
	
	if(ARG_LIB_TYPE STREQUAL "STATIC")
		# static library
		if(NOT ARG_SOURCES)
			tx_error_log("${TXLib_MODULE_NAME}" "Missing SOURCES entry."
			                                    "  When LIB_TYPE is STATIC, at least one SOURCES entry have to be provided.")
		endif()
		add_library("${TXLib_MODULE_NAME}" STATIC)
		tx_set_compile_flags("${TXLib_MODULE_NAME}" PUBLIC)

		set(SCOPE_PUBLIC "PUBLIC")
		set(SCOPE_PRIVATE "PRIVATE")

		foreach(FILE IN LISTS ARG_SOURCES) # SOURCES - moved up to here because interface does not need sources
			set(FILE "${TXLib_MODULE_DIR}/src/${FILE}")
			if(NOT EXISTS ${FILE})
				tx_error_log("${TXLib_MODULE_NAME}" "Cannot find source file (SOURCES):" "  ${FILE}")
			endif()
			target_sources("${TXLib_MODULE_NAME}" ${SCOPE_PRIVATE} "${FILE}")
		endforeach()
	elseif(ARG_LIB_TYPE STREQUAL "INTERFACE")
		# interface library
		add_library("${TXLib_MODULE_NAME}" INTERFACE)
		tx_set_compile_flags("${TXLib_MODULE_NAME}" INTERFACE)
		
		set(SCOPE_PUBLIC "INTERFACE")
		set(SCOPE_PRIVATE "INTERFACE")
	else()
		tx_error_log("${TXLib_MODULE_NAME}" "Unsupported LIB_TYPE:" "  ${ARG_LIB_TYPE}.")
	endif()

	# add source files
	foreach(FILE IN LISTS ARG_PUBLIC_HEADERS) # PUBLIC_HEADERS
		set(FILE "${TXLib_MODULE_DIR}/include/tx/${FILE}")
		if(NOT EXISTS ${FILE})
			tx_error_log("${TXLib_MODULE_NAME}" "Cannot find source file (PUBLIC_HEADERS):" "  ${FILE}")
		endif()
		target_sources("${TXLib_MODULE_NAME}" ${SCOPE_PUBLIC} "${FILE}")
	endforeach()
	
	foreach(FILE IN LISTS ARG_IMPL_HEADERS) # IMPL_HEADERS
		set(FILE "${TXLib_MODULE_DIR}/include/impl/${FILE}")
		if(NOT EXISTS ${FILE})
			tx_error_log("${TXLib_MODULE_NAME}" "Cannot find source file (IMPL_HEADERS):" "  ${FILE}")
		endif()
		target_sources("${TXLib_MODULE_NAME}" ${SCOPE_PUBLIC} "${FILE}")
	endforeach()

	# resolve dependencies
	foreach(DEP_MODULE IN LISTS TXLib_${TXLib_MODULE_NAME}_DEPENDENCIES)
		target_link_libraries("${TXLib_MODULE_NAME}" ${SCOPE_PUBLIC} ${DEP_MODULE})		
	endforeach()
	
	# target properties

	# cxx version
	target_compile_features("${TXLib_MODULE_NAME}" ${SCOPE_PUBLIC} ${TXLib_CXX_VERSION})

	# include dir
	target_include_directories("${TXLib_MODULE_NAME}" ${SCOPE_PUBLIC}
		"${TXLib_MODULE_DIR}/include"
	)
endfunction()