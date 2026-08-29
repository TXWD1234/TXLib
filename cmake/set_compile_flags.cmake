include_guard(GLOBAL)

# scope should be PUBLIC | PRIVATE | INTERFACE (case-sensitive in CMake)
function(tx_set_compile_flags in_target scope)
	target_compile_options(${in_target} ${scope}
		-Wall -Wextra
		-Wno-comment -Wno-unused-parameter
    )

    # Check if we are in any Release-based configuration
    if(CMAKE_BUILD_TYPE STREQUAL "Release" OR CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")     
        
        # Enable LTO for both profiles
        set_property(TARGET ${in_target} PROPERTY INTERPROCEDURAL_OPTIMIZATION TRUE)
        
        # Base Release optimizations
        target_compile_options(${in_target} ${scope}
            -O2 
            -march=x86-64-v3
        )
        
        # Extra debug/profiling info specifically for RelWithDebInfo
        if(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
            target_compile_options(${in_target} ${scope}
                -g 
                -fno-omit-frame-pointer
            )
        endif()
	elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
		
		target_compile_options(${in_target} ${scope}
			-g
    	)


    endif()

endfunction()