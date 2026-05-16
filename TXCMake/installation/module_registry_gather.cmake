include_guard(GLOBAL)

function(tx_module_registry_gather)
    file(GLOB moduleDirs "${TXLib_SOURCE_DIR}/modules/*")

    foreach(moduleDir IN LISTS moduleDirs)
        set(local_reg_file "${local_dir}/module_registry.cmake")
        
        if(EXISTS "${local_reg_file}")
            include("${local_reg_file}")
        endif()
    endforeach()
endfunction()