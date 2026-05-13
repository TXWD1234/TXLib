include_guard(GLOBAL)

# have to be called with TXLib_MODULE_NAME exist in the parent scope
function(tx_make_module_available moduleName)
	if(NOT TXLib_MODULE_NAME)
		message(FATAL_ERROR "TXLib: TXLib_MODULE_NAME does not exist. Hint: TXLib_MODULE_NAME not setted")
	endif()
	
	if(NOT TARGET ${moduleName})
		message(STATUS "TXLib: Adding component [${moduleName}] as dependency of [${TXLib_MODULE_NAME}]")
		add_subdirectory("${TXLib_SOURCE_DIR}/${moduleName}" "${TXLib_BINARY_DIR}/${moduleName}") # <--------------- make new function: tx_link_module (use dependency)
	endif()
endfunction()