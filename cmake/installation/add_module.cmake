include_guard(GLOBAL)

# this function is simply just the wrapper of add_subdirectory
function(tx_add_module MODULE)
	set(TXLib_INSTALLATION_MODULE_NAME ${MODULE})
	add_subdirectory(
		"${TXLib_${MODULE}_SOURCE_DIR}"
		"${TXLib_BINARY_DIR}/${MODULE}")
endfunction()