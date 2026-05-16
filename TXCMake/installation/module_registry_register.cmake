include_guard(GLOBAL)

function(tx_module_registry_register ARG_MODULE_NAME ARG_SOURCE_DIR)
	set(TXLib_${ARG_MODULE_NAME}_SOURCE_DIR ${ARG_SOURCE_DIR} CACHE INTERNAL "")
	set(TXLib_${ARG_MODULE_NAME}_DEPENDENCIES ${ARGN} CACHE INTERNAL "")
endfunction()