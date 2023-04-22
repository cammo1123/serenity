function(add_dependencies_to_libraries lib)
    # Get LAGOM_TOOL_LIBS from parent scope
    get_property(LAGOM_TOOL_LIBS GLOBAL PROPERTY LAGOM_TOOL_LIBS)
    
    string(SUBSTRING ${lib} 3 -1 NO_PREFIX_LIB)
    string(TOLOWER ${NO_PREFIX_LIB} NO_PREFIX_LIB)
    list(APPEND LAGOM_TOOL_LIBS "${CMAKE_BINARY_DIR}/lagom/lagom-${NO_PREFIX_LIB}.dll")

    # Set LAGOM_TOOL_LIBS in parent scope
    set_property(GLOBAL PROPERTY LAGOM_TOOL_LIBS ${LAGOM_TOOL_LIBS})

    # get that libraries dependencies
    get_target_property(lib_deps ${lib} LINK_LIBRARIES)
    foreach (lib_dep ${lib_deps})
        if (NOT lib_dep MATCHES "^Lib.*")
            continue()
        endif()

        get_target_property(lib_dep_type ${lib_dep} TYPE)
        # if it begins with Lib and is a shared library, add it to the list
        if (lib_dep_type STREQUAL "SHARED_LIBRARY" AND NOT lib_dep STREQUAL lib AND lib_dep MATCHES "^Lib.*")
            add_dependencies_to_libraries(${lib_dep})
        endif()
    endforeach()
endfunction()

function(lagom_tool tool)
    cmake_parse_arguments(LAGOM_TOOL "" "" "SOURCES;LIBS" ${ARGN})
    add_executable(${tool} ${SOURCES} ${LAGOM_TOOL_SOURCES})
    # alias for parity with exports
    add_executable(Lagom::${tool} ALIAS ${tool})
    target_link_libraries(${tool} LibCore LibFileSystem ${LAGOM_TOOL_LIBS})
    if(WIN32)
        set (LAGOM_TOOL_LIBS
            "${CMAKE_BINARY_DIR}/lagom/lagom-core.dll"
            "${CMAKE_BINARY_DIR}/lagom/lagom-filesystem.dll"
        )

        set_property(GLOBAL PROPERTY LAGOM_TOOL_LIBS ${LAGOM_TOOL_LIBS})
        foreach (lib ${LAGOM_TEST_LIBS})
            add_dependencies_to_libraries(${lib})
            get_property(LAGOM_TOOL_LIBS GLOBAL PROPERTY LAGOM_TOOL_LIBS)
        endforeach()

        foreach (lib ${LAGOM_TOOL_LIBS})
            add_custom_command(TARGET ${tool} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${lib}
                $<TARGET_FILE_DIR:${tool}>)
        endforeach()
    endif()
    install(
        TARGETS ${tool}
        EXPORT LagomTargets
        RUNTIME COMPONENT Lagom_Runtime
    )
endfunction()

add_subdirectory(CodeGenerators)
add_subdirectory(ConfigureComponents)
add_subdirectory(IPCMagicLinter)

if (ENABLE_JAKT)
    include(jakt)
endif()
