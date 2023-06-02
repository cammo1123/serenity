set(TEST_SOURCES
    TestTLSHandshake.cpp
)

foreach(source IN LISTS TEST_SOURCES)
    serenity_test("${source}" LibTLS LIBS LibTLS LibCrypto)

    if (WIN32)
        get_filename_component(name ${source} NAME_WE)
        target_link_libraries(${name} PRIVATE ws2_32 wsock32)
    endif()
endforeach()
