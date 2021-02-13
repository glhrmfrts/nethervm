set (TARGET_NAME nethervmtest)

file (GLOB SOURCE_FILES *.c)

include_directories(${PROJECT_SOURCE_DIR}/include/)

add_executable(${TARGET_NAME} ${SOURCE_FILES})

target_link_libraries(${TARGET_NAME} PRIVATE libnethervm)