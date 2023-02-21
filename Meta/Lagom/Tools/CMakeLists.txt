function(lagom_tool tool)
    cmake_parse_arguments(LAGOM_TOOL "" "" "SOURCES;LIBS" ${ARGN})
    add_executable(${tool} ${SOURCES} ${LAGOM_TOOL_SOURCES})
    # alias for parity with exports
    add_executable(Lagom::${tool} ALIAS ${tool})
    target_link_libraries(${tool} LibCore ${LAGOM_TOOL_LIBS})
	
	#copy liblagom-core.dll to the build directory
	add_custom_command(TARGET ${tool} POST_BUILD
	COMMAND ${CMAKE_COMMAND} -E copy
		${CMAKE_SOURCE_DIR}/../../../serenity/Build/lagom/Userland/Libraries/LibCore/liblagom-core.dll
		$<TARGET_FILE_DIR:${tool}>)	
	
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
