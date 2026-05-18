include_guard(GLOBAL)

include("${TXLib_INSTALLATION_DIR}/log.cmake")

function(tx_preprocess_module MODULES OUT_DEPS ARG_BOOL_INCOMING_DEPENDENCIES)
	# module presence, register requests
	foreach(MODULE IN LISTS ${MODULES})
		# module presence check
		if(NOT EXISTS TXLib_${MODULE}_SOURCE_DIR)
			tx_error_log(
				"Cannot find SOURCE_DIR of requested module: ${MODULE}"
				"  SOURCE_DIR: \"${TXLib_${MODULE}_SOURCE_DIR}\""
			)
		endif()

		# register user request
		set(TXLib_${MODULE}_INCLUDED TRUE PARENT_SCOPE) # for lookup in the future
	endforeach()

	# resolve dependencies
	set(LOCAL_BOOL_INCOMING_DEPENDENCIES FALSE)
	set(LOCAL_NEW_DEPS "")
	foreach(MODULE IN LISTS ${MODULES})
		foreach(DEP IN LISTS TXLib_${MODULE}_DEPENDENCIES)
			if(NOT TXLib_${DEP}_INCLUDED)
				tx_log("Found module [${DEP}] as dependency of requested module [${MODULE}]"
				       "  Adding [${DEP}] to module list.")
				list(APPEND LOCAL_NEW_DEPS "${DEP}")
				set(TXLib_${DEP}_INCLUDED TRUE PARENT_SCOPE)
				set(LOCAL_BOOL_INCOMING_DEPENDENCIES TRUE)
			endif()
		endforeach()
	endforeach()

	# parent scope everything
	set(${OUT_DEPS} ${LOCAL_NEW_DEPS} PARENT_SCOPE)
	set(${ARG_BOOL_INCOMING_DEPENDENCIES} ${LOCAL_BOOL_INCOMING_DEPENDENCIES} PARENT_SCOPE)
endfunction()