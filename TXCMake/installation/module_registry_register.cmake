include_guard(GLOBAL)
#[[
MODULE_NAME
Name of the target module of registering

SOURCE_DIR
Source Directory of the target module of registering

All trailing parameters:
DEPENDENCIES
The dependency of this module within TXLib.
Attension! This does not account for external dependencies. You have to link them yourself.
Write the name of the dependency module (eg. TXMath).
]]
function(tx_module_registry_register MODULE_NAME MODULE_SOURCE_DIR)
	set(TXLib_${MODULE_NAME}_SOURCE_DIR ${MODULE_SOURCE_DIR} CACHE INTERNAL "")
	set(TXLib_${MODULE_NAME}_DEPENDENCIES ${ARGN} CACHE INTERNAL "")
	set(TXLib_MODULES "${TXLib_MODULES}" "${MODULE_NAME}" PARENT_SCOPE)
endfunction()