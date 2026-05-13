include_guard(GLOBAL)

function(tx_log)
	string(JOIN "\nTXLib: " messageStr ${ARGN})

	message(STATUS "TXLib: ${messageStr}")
endfunction()

function(tx_error_log moduleName)
	string(JOIN "\nTXLib:   " messageStr ${ARGN})

	message(FATAL_ERROR "TXLib: In module: [${moduleName}]:\nTXLib:   ${messageStr}")
endfunction()