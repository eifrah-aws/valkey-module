cmake_minimum_required(VERSION 3.10)
project(module)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(DEMO_MODULE_SRCS "${CMAKE_CURRENT_LIST_DIR}/valkey-module.cpp" "${CMAKE_CURRENT_LIST_DIR}/random.cpp")
set(PAIR_MODULE_SRCS "${CMAKE_CURRENT_LIST_DIR}/new-data-type.cpp")

option(ASAN_BUILD "Build with ASan enabled" OFF)

function(create_module MODULE_NAME SOURCES)
    add_library(${MODULE_NAME} SHARED "${SOURCES}")
    target_include_directories(${MODULE_NAME} PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src")

    if(ASAN_BUILD)
        target_compile_options(${MODULE_NAME} PRIVATE "-fsanitize=address")
        target_compile_options(${MODULE_NAME} PRIVATE "-fno-sanitize-recover=all")
        target_compile_options(${MODULE_NAME} PRIVATE "-fno-omit-frame-pointer")
        target_link_options(${MODULE_NAME} PRIVATE "-fsanitize=address")
        message(STATUS "Address Sanitization (ASan) is enabled for module: ${MODULE_NAME}")
    endif()

    # Lets remove the "lib" prefix
    set_target_properties(${MODULE_NAME} PROPERTIES PREFIX "")
    if(APPLE)
        # Some symbols can only be resolved during runtime (they exist in the executable)
        target_link_options(${MODULE_NAME} PRIVATE -undefined dynamic_lookup)
    endif()
endfunction()

create_module(valkey-module "${DEMO_MODULE_SRCS}")
create_module(valkey-pair-module "${PAIR_MODULE_SRCS}")
