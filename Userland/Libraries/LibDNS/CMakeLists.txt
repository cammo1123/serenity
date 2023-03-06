set(SOURCES
    Answer.cpp
    Name.cpp
    Packet.cpp
)

serenity_lib(LibDNS dns)
target_link_libraries(LibDNS PRIVATE LibCore LibIPC)

if (WIN32)
	target_link_libraries(LibDNS PRIVATE ws2_32)
endif ()
