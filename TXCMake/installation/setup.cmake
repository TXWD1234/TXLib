# setting the INSTALLATION scope global variables
set(TXLib_INSTALLATION_DIR "${TXLib_SOURCE_DIR}/TXCMake/installation")

# include installation script and resources
include("${TXLib_INSTALLATION_DIR}/log.cmake")
include("${TXLib_INSTALLATION_DIR}/preprocess_module.cmake")
include("${TXLib_INSTALLATION_DIR}/add_module.cmake")
include("${TXLib_SOURCE_DIR}/module_registry.cmake")

include("${TXLib_INSTALLATION_DIR}/txlib_module.cmake") # for the CMakeLists.txt of the modules

# include static parameters
include("${TXLib_INSTALLATION_DIR}/txlib_cxx_version.cmake")


# setup begin
tx_log("Setup starts.")

if(TXLib_INSTALLATION_MODULES) # if given requested modules	
	# module integrity check and
	# compose dependency list

	set(TXLib_INSTALLATION_DEPS "")

	set(BOOL_INCOMING_DEPENDENCIES "")
	# first pass - resolve dependencies of requested modules
	tx_preprocess_module(TXLib_INSTALLATION_MODULES TXLib_INSTALLATION_DEPS BOOL_INCOMING_DEPENDENCIES)

	# further passes - resolve dependencies of the dependencies
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

	# end of stage 2 - module list are ready
	tx_log("Modules:")
	foreach(MODULE IN LISTS TXLib_INSTALLATION_MODULES)
		tx_log("  ${MODULE}")
	endforeach()
	foreach(MODULE IN LISTS TXLib_INSTALLATION_DEPS)
		tx_log("  ${MODULE} (Dependency)")
	endforeach()

	# adding modules

	# topological sort
	# *my own dumb implementation*
	# implementation details:
	#   the TXLib_INSTALLATION_MODULES and TXLib_INSTALLATION_DEPS was almost
	#   useless out side of printing the list. the actual data is stored as 
	#   TXLib_${MODULE}_INCLUDED (The CMake Variable Exploit lookup table)

	# clear the data
	unset(TXLib_INSTALLATION_DEPS)
	set(TXLib_INSTALLATION_MODULES "") # clear

	# sort according to the order of TXLib_MODULES, since TXLib_MODULES is already sorted
	# use CMake variable exploit again as the lookup table / hash map
	foreach(MODULE IN LISTS TXLib_MODULES)
		if(TXLib_${MODULE}_INCLUDED)
			list(APPEND TXLib_INSTALLATION_MODULES ${MODULE})
		endif()		
	endforeach()

	# actually adding the modules' CMakeLists.txt
	foreach(MODULE IN LISTS TXLib_INSTALLATION_MODULES)
		tx_add_module("${MODULE}")
	endforeach()

else() # add the whole library
	tx_log("Modules:")
	foreach(MODULE IN LISTS TXLib_MODULES)
		tx_log("  ${MODULE}")
	endforeach()

	# note: because TXLib_MODULES list is already topologically sorted, therefore no sorting required.
	foreach(MODULE IN LISTS TXLib_MODULES)
		tx_add_module("${MODULE}")
	endforeach()
endif()

tx_log("Done adding TXLib.")