cmake_minimum_required(VERSION 3.10)
project(module)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(DEMO_MODULE_SRCS "${CMAKE_CURRENT_LIST_DIR}/valkey-module.cpp" "${CMAKE_CURRENT_LIST_DIR}/random.cpp")
set(PAIR_MODULE_SRCS "${CMAKE_CURRENT_LIST_DIR}/new-data-type.cpp")

add_library(valkey-module SHARED ${DEMO_MODULE_SRCS})
add_library(valkey-pair-module SHARED ${PAIR_MODULE_SRCS})

target_include_directories(valkey-module PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src")
target_include_directories(valkey-pair-module PRIVATE "${CMAKE_CURRENT_LIST_DIR}/src")

# Lets remove the "lib" prefix
set_target_properties(valkey-module PROPERTIES PREFIX "")
set_target_properties(valkey-pair-module PROPERTIES PREFIX "")

if(APPLE)
    # Some symbols can only be resolved during runtime (they exist in the executable)
    target_link_options(valkey-module PRIVATE -undefined dynamic_lookup)
    target_link_options(valkey-pair-module PRIVATE -undefined dynamic_lookup)
endif()
