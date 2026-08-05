# Umbrella target for everything
if(TARGET TXLib)
	return()
endif()

add_library(TXLib INTERFACE)

foreach(MODULE IN LISTS TXLib_INSTALLATION_MODULES)
	target_link_libraries(TXLib INTERFACE ${MODULE})
endforeach()

target_sources(TXLib INTERFACE
	FILE_SET HEADERS
	BASE_DIRS "${TXLib_SOURCE_DIR}/include"
	FILES "${TXLib_SOURCE_DIR}/include/tx/txlib.h"
)