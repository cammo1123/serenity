set(SOURCES
    IDLGenerators.cpp
    main.cpp
)

lagom_tool(BindingsGenerator LIBS LibIDL LibMain)

if (!MSVC)
    target_compile_options(BindingsGenerator PUBLIC -g)
endif()
