cmake_minimum_required(VERSION 3.14)
project (ublox_gps)

if (WIN32)
	set(STREAM_SRC "src/Stream_win32.cpp")
endif (WIN32)

add_library(ublox_gps STATIC
    SparkFun_u-blox_GNSS_Arduino_Library/src/SparkFun_u-blox_GNSS_Arduino_Library.cpp
    src/Print.cpp
    src/Utils.cpp
    ${STREAM_SRC}
)

target_include_directories(ublox_gps
    PUBLIC
        ${CMAKE_CURRENT_SOURCE_DIR}/inc
        #${CMAKE_CURRENT_SOURCE_DIR}/SparkFun_Ublox_Arduino_Library/src
        ${CMAKE_CURRENT_SOURCE_DIR}/SparkFun_u-blox_GNSS_Arduino_Library/src
)

add_executable(ublox_f9p_test
    linux_examples/ublox_f9p_test.cpp
)

target_link_libraries(ublox_f9p_test ublox_gps)