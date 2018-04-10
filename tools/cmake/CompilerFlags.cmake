# check if a C compiler flag is possible
include(CheckCCompilerFlag)
function(check_cc_flag CC_FLAG)
    check_c_compiler_flag("${CC_FLAG}" CC_HAS_${CC_FLAG})
    if(CC_HAS_${CC_FLAG})
        add_definitions("${CC_FLAG}")
    endif()
endfunction()

# check if an untested C compiler flag is possible
function(check_cc_flag_untested CC_FLAG)
    check_c_compiler_flag("${CC_FLAG}" CC_HAS_${CC_FLAG})
    if(CC_HAS_${CC_FLAG})
        add_definitions("${CC_FLAG}")
        message(WARNING "Add untested flag: ${CC_FLAG}")
    endif()
endfunction()
