# setting the INSTALLATION scope global variables
set(TXLib_INSTALLATION_DIR "${TXLib_SOURCE_DIR}/TXCMake/installation")

# include installation script and resources
include("${TXLib_INSTALLATION_DIR}/log.cmake")
include("${TXLib_INSTALLATION_DIR}/preprocess_module.cmake")
include("${TXLib_INSTALLATION_DIR}/add_module.cmake")
include("${TXLib_SOURCE_DIR}/module_registry.cmake")

# include static parameters
include("${TXLib_INSTALLATION_DIR}/txlib_cxx_version.cmake")


# setup begin
tx_log("Setup starts.")

if(TXLib_INSTALLATION_MODULES) # if given requested modules	
	# pre-add check and process

	set(TXLib_INSTALLATION_DEPS "")

	# first pass - resolve dependencies of requested modules
	tx_preprocess_module(TXLib_INSTALLATION_MODULES TXLib_INSTALLATION_DEPS)

	# further passes - resolve dependencies of the dependencies
	set(BOOL_INCOMING_DEPENDENCIES TRUE)
	set(TXLib_INSTALLATION_DEPS_CUR ${TXLib_INSTALLATION_DEPS})
		
	while(BOOL_INCOMING_DEPENDENCIES)
		set(TXLib_INSTALLATION_DEPS_NXT "")
		tx_preprocess_module(
			TXLib_INSTALLATION_DEPS_CUR
			TXLib_INSTALLATION_DEPS_NXT
			BOOL_INCOMING_DEPENDENCIES)

		list(APPEND TXLib_INSTALLATION_DEPS ${TXLib_INSTALLATION_DEPS_NXT})
		set(TXLib_INSTALLATION_DEPS_CUR ${TXLib_INSTALLATION_DEPS_NXT})
	endwhile()


	tx_log("Modules:")
	foreach(MODULE IN LISTS TXLib_INSTALLATION_MODULES)
		tx_log("  ${MODULE}")
	endforeach()
	foreach(MODULE IN LISTS TXLib_INSTALLATION_DEPS)
		tx_log("  ${MODULE} (Dependency)")
	endforeach()

	# adding modules

	foreach(MODULE IN LISTS TXLib_INSTALLATION_MODULES)
		tx_add_module("${MODULE}")
	endforeach()
	foreach(MODULE IN LISTS TXLib_INSTALLATION_DEPS)
		tx_add_module("${MODULE}")
	endforeach()
else() # add the whole library
	tx_log("Modules:")
	foreach(MODULE IN LISTS TXLib_MODULES)
		tx_log("  ${MODULE}")
	endforeach()

	foreach(MODULE IN LISTS TXLib_INSTALLATION_MODULES)
		tx_add_module("${MODULE}")
	endforeach()
endif()

tx_log("Done adding TXLib.")