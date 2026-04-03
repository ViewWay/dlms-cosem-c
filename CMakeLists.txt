cmake_minimum_required(VERSION 3.10)
project(dlms-cosem-c VERSION 1.0.0 LANGUAGES C)
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Compiler warnings
if(CMAKE_C_COMPILER_ID MATCHES "GNU|Clang")
    add_compile_options(-Wall -Wextra -Wpedantic -Wshadow)
endif()

# Library
add_library(dlms-cosem STATIC
    src/dlms_core.c
    src/dlms_hdlc.c
    src/dlms_axdr.c
    src/dlms_asn1.c
    src/dlms_security.c
    src/dlms_cosem.c
    src/dlms_client.c
)
target_include_directories(dlms-cosem PUBLIC include)

# Tests
enable_testing()
file(GLOB TEST_SOURCES tests/test_*.c)
add_executable(test_runner tests/test_main.c ${TEST_SOURCES})
target_link_libraries(test_runner dlms-cosem)
add_test(NAME dlms_tests COMMAND test_runner)

# Example
add_executable(meter_simulation examples/meter_simulation.c)
target_link_libraries(meter_simulation dlms-cosem)
