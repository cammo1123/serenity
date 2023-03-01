function(lagom_tool tool)
    cmake_parse_arguments(LAGOM_TOOL "" "" "SOURCES;LIBS" ${ARGN})
    add_executable(${tool} ${SOURCES} ${LAGOM_TOOL_SOURCES})
    # alias for parity with exports
    add_executable(Lagom::${tool} ALIAS ${tool})
    target_link_libraries(${tool} LibCore ${LAGOM_TOOL_LIBS})

	# move all the libs we need into the Lagom_Runtime component
	set_target_properties(${tool} PROPERTIES
		RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lagom
		RUNTIME_OUTPUT_DIRECTORY_DEBUG ${CMAKE_BINARY_DIR}/lagom
		RUNTIME_OUTPUT_DIRECTORY_RELEASE ${CMAKE_BINARY_DIR}/lagom
		RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO ${CMAKE_BINARY_DIR}/lagom
		RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL ${CMAKE_BINARY_DIR}/lagom
		RUNTIME_OUTPUT_NAME ${tool}
		RUNTIME_OUTPUT_NAME_DEBUG ${tool}
		RUNTIME_OUTPUT_NAME_RELEASE ${tool}
		RUNTIME_OUTPUT_NAME_RELWITHDEBINFO ${tool}
		RUNTIME_OUTPUT_NAME_MINSIZEREL ${tool}
	)
	
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
