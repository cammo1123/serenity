set(SOURCES
	AnonymousBuffer.cpp
	ArgsParser.cpp
	ConfigFile.cpp
	DateTime.cpp
	MappedFile.cpp
	EventLoop.cpp
	ElapsedTimer.cpp
	Event.cpp
	Notifier.cpp
	NetworkJob.cpp
	SessionManagement.cpp
	LocalServer.cpp
	MimeData.cpp
    IODevice.cpp
	Property.cpp
	Object.cpp
	StandardPaths.cpp
	SystemServerTakeover.cpp
	Directory.cpp
	TCPServer.cpp
	Timer.cpp
	Version.cpp
)
if (NOT WIN32)
	list(APPEND SOURCES
	    Command.cpp
		DirIterator.cpp
	    File.cpp
	    LockFile.cpp
	    Process.cpp
	    ProcessStatisticsReader.cpp
	    SecretString.cpp
	    SOCKSProxyClient.cpp
	    Stream.cpp
	    System.cpp
	    TempFile.cpp
	    UDPServer.cpp
	)
else()
	list(APPEND SOURCES
		Windows/System.cpp
		Windows/File.cpp	    
		Windows/Stream.cpp
		Windows/DirIterator.cpp
	)
endif()
if (NOT ANDROID AND NOT WIN32 AND NOT EMSCRIPTEN)
    list(APPEND SOURCES
        Account.cpp
        FilePermissionsMask.cpp
        GetPassword.cpp
        Group.cpp
        LocalServer.cpp
    )
endif()

# FIXME: Implement Core::FileWatcher for macOS, *BSD, and Windows.
if (SERENITYOS)
    list(APPEND SOURCES FileWatcherSerenity.cpp)
elseif (LINUX AND NOT EMSCRIPTEN)
    list(APPEND SOURCES FileWatcherLinux.cpp)
elseif (APPLE)
    list(APPEND SOURCES FileWatcherMacOS.mm)
else()
    list(APPEND SOURCES FileWatcherUnimplemented.cpp)
endif()

serenity_lib(LibCore core)
target_link_libraries(LibCore PRIVATE LibCrypt LibSystem)

if (WIN32)
	target_link_libraries(LibCore PRIVATE ws2_32)
endif ()

if (APPLE)
    target_link_libraries(LibCore PUBLIC "-framework CoreFoundation")
    target_link_libraries(LibCore PUBLIC "-framework CoreServices")
    target_link_libraries(LibCore PUBLIC "-framework Foundation")
endif()
