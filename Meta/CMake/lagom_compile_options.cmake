include(${CMAKE_CURRENT_LIST_DIR}/common_compile_options.cmake)

add_compile_options(-Wno-maybe-uninitialized)
add_compile_options(-Wno-shorten-64-to-32)
add_compile_options(-fsigned-char)
add_compile_options(-g3)
add_compile_options(-O0)

if (NOT ENABLE_FUZZERS AND NOT APPLE AND NOT WIN32)
    add_compile_options(-fno-semantic-interposition)
endif()
if (NOT WIN32)
    add_compile_options(-fPIC)
endif()
