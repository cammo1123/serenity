set(SOURCES
	ArgsParser.cpp
	EventLoop.cpp
	Event.cpp
    IODevice.cpp
	Property.cpp
	Object.cpp
	Version.cpp
)
if (NOT WIN32)
	list(APPEND SOURCES
		AnonymousBuffer.cpp
	    Command.cpp
	    ConfigFile.cpp
	    DateTime.cpp
	    Directory.cpp
	    DirIterator.cpp
	    ElapsedTimer.cpp
	    File.cpp
	    LockFile.cpp
	    MappedFile.cpp
	    MimeData.cpp
	    NetworkJob.cpp
	    Notifier.cpp
	    Process.cpp
	    ProcessStatisticsReader.cpp
	    SecretString.cpp
	    SessionManagement.cpp
	    SOCKSProxyClient.cpp
	    StandardPaths.cpp
	    Stream.cpp
	    System.cpp
	    SystemServerTakeover.cpp
	    TCPServer.cpp
	    TempFile.cpp
	    Timer.cpp
	    UDPServer.cpp
	)
else()
	list(APPEND SOURCES
		Windows/System.cpp
		Windows/File.cpp	    
		Windows/Stream.cpp
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

if (APPLE)
    target_link_libraries(LibCore PUBLIC "-framework CoreFoundation")
    target_link_libraries(LibCore PUBLIC "-framework CoreServices")
    target_link_libraries(LibCore PUBLIC "-framework Foundation")
endif()
