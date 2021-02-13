cmake_minimum_required(VERSION 3.15)

project (nethervm)

option (NETHERVM_BUILD_SHARED "Build Shared library / DLL" OFF)
option (NETHERVM_BUILD_TESTS "Build tests" OFF)

add_subdirectory (src)

if (NETHERVM_BUILD_TESTS)
    add_subdirectory (test)
endif (NETHERVM_BUILD_TESTS)