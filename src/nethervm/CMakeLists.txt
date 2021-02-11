set (TARGET_NAME libnethervm)

file (GLOB SOURCE_FILES *.c)

file (GLOB HEADER_FILES ${PROJECT_SOURCE_DIR}/include/nethervm/*.h)

include_directories(${PROJECT_SOURCE_DIR}/include/)

if (NETHERVM_BUILD_SHARED)
    add_library (${TARGET_NAME} SHARED ${SOURCE_FILES} ${HEADER_FILES})
else ()
    add_library (${TARGET_NAME} ${SOURCE_FILES} ${HEADER_FILES})
endif (NETHERVM_BUILD_SHARED)