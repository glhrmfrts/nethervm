cmake_minimum_required(VERSION 3.15)

project (nethervm)

option (NETHERVM_BUILD_SHARED "Build Shared library / DLL" OFF)

add_subdirectory (src)