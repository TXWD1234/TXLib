include_guard(GLOBAL)

include("${CMAKE_CURRENT_LIST_DIR}/log.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/../optimization.cmake")
include("${CMAKE_CURRENT_LIST_DIR}/make_module_available.cmake")

#[[
MODULE_NAME
(Mandatory)
The name of the module. It will be used as the cmake target name. Example: TXMath

MODULE_DIR
The path of the module.
Modules are not strictly defined by CMakeLists.txt within the module dir anymore.
The path of the module only have to contain the source of the module.
The CMake definition (the calling of this funciton) can be anywhere.
Default to `${TXLib_SOURCE_DIR}/${MODULE_NAME}`.
Note: This parameter exist for flexibility, but the best practice is still keep the module name identical with the dir name

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

DEPENDENCIES
The dependency of this module within TXLib.
Attension! This does not account for external dependencies. You have to link them yourself.
Write the name of the dependency module.

]]
function(tx_txlib_module)

	set(options "")
    set(oneValueArgs MODULE_NAME MODULE_DIR LIB_TYPE)
    set(multiValueArgs DEPENDENCIES PUBLIC_HEADERS IMPL_HEADERS SOURCES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

	# parameter integrity check
	if(NOT ARG_MODULE_NAME)
		tx_error_log("UNKNOWN" "Failed to declare module. Module Name not provided.")
	endif()
	if(NOT ARG_LIB_TYPE)
		tx_error_log("${ARG_MODULE_NAME}" "Failed to declare module. Missing mandatory parameter: LIB_TYPE.")
	endif()
	if(NOT ARG_PUBLIC_HEADERS)
		tx_error_log("${ARG_MODULE_NAME}" "Failed to declare module. Missing mandatory parameter: PUBLIC_HEADERS.")
	endif()

	# to defence being included multiple times
	if(TARGET ${ARG_MODULE_NAME})
		return()
	endif()
	# to defence the invalid add_subdirectories call that's not from add_txlib.cmake
	if(NOT TXLib_SOURCE_DIR OR NOT TXLib_BINARY_DIR)
		tx_error_log( "${ARG_MODULE_NAME}" "Variable TXLib_SOURCE_DIR or TXLib_BINARY_DIR does not exist." "Hint: Not added by `add_txlib.cmake`.")
	endif()

	# resolving module dir
	if(ARG_MODULE_DIR)
		set(TXLib_MODULE_DIR ${ARG_MODULE_DIR})
		if(NOT EXISTS ${TXLib_MODULE_DIR})
			tx_error_log("${ARG_MODULE_NAME}" "Cannot find MODULE_DIR path:" "  ${TXLib_MODULE_DIR}")
		endif()
	else()
		set(TXLib_MODULE_DIR "${TXLib_SOURCE_DIR}/${ARG_MODULE_NAME}")
		if(NOT EXISTS ${TXLib_MODULE_DIR})
			tx_error_log("${ARG_MODULE_NAME}" "MODULE_DIR not set, and cannot find default path:" "  ${TXLib_MODULE_DIR}")
		endif()
	endif()

	# start target configuration

	set(TXLib_MODULE_NAME "${ARG_MODULE_NAME}")
	
	if(ARG_LIB_TYPE STREQUAL "STATIC")
		# static library
		if(NOT ARG_SOURCES)
			tx_error_log("${TXLib_MODULE_NAME}" "Missing SOURCES entry." "When LIB_TYPE is STATIC, at least one SOURCES entry have to be provided.")
		endif()
		add_library("${TXLib_MODULE_NAME}" STATIC)
		tx_add_release_ops("${TXLib_MODULE_NAME}")

		set(SCOPE_PUBLIC "PUBLIC")
		set(SCOPE_PRIVATE "PRIVATE")

		foreach(FILE IN LISTS ARG_SOURCES) # SOURCES
			set(FILE "${TXLib_MODULE_DIR}/src/${FILE}")
			if(NOT EXISTS ${FILE})
				tx_error_log("${TXLib_MODULE_NAME}" "Cannot find source file (SOURCES):" "  ${FILE}")
			endif()
			target_sources("${TXLib_MODULE_NAME}" ${SCOPE_PRIVATE} "${FILE}")
		endforeach()
	elseif(ARG_LIB_TYPE STREQUAL "INTERFACE")
		# interface library
		add_library("${TXLib_MODULE_NAME}" INTERFACE)
		tx_add_release_ops_interface("${TXLib_MODULE_NAME}")
		
		set(SCOPE_PUBLIC "INTERFACE")
		set(SCOPE_PRIVATE "INTERFACE")
	else()
		tx_error_log("${TXLib_MODULE_NAME}" "Unsupported LIB_TYPE: ${ARG_LIB_TYPE}.")
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
	foreach(DEP_MODULE IN LISTS ARG_DEPENDENCIES)
		tx_make_module_available("${DEP_MODULE}")
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