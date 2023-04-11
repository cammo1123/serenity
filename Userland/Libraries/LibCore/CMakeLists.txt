set(SOURCES
    ArgsParser.cpp
    ConfigFile.cpp
    DateTime.cpp
    Directory.cpp
    DirectoryEntry.cpp
    DirIterator.cpp
    ElapsedTimer.cpp
    Event.cpp
    EventLoop.cpp
    EventLoopImplementation.cpp
    File.cpp
    LockFile.cpp
    MappedFile.cpp
    MimeData.cpp
    NetworkJob.cpp
    Notifier.cpp
    Object.cpp
    Property.cpp
    SecretString.cpp
    StandardPaths.cpp
    SystemServerTakeover.cpp
    TCPServer.cpp
    ThreadEventQueue.cpp
    Timer.cpp
    Version.cpp
    )
    if (NOT WIN32)
        list(APPEND SOURCES
        AnonymousBuffer.cpp
        Command.cpp
        DeprecatedFile.cpp
        EventLoopImplementationUnix.cpp
        LockFile.cpp
        Process.cpp
        ProcessStatisticsReader.cpp
        Socket.cpp
        SessionManagement.cpp
        SOCKSProxyClient.cpp
        System.cpp
        TempFile.cpp
        UDPServer.cpp
    )
else()
	list(APPEND SOURCES
		AnonymousBufferWindows.cpp
        EventLoopImplementationWindows.cpp
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
