project(blackmagic)

message(STATUS "Resolving GIT Version")
set(_build_version "unknown")
find_package(Git)
if(GIT_FOUND)
    execute_process(
        COMMAND ${GIT_EXECUTABLE} describe --always --dirty --tags
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE _farpatch_build_version
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    execute_process(
        COMMAND ${GIT_EXECUTABLE} -C blackmagic/blackmagic describe --always --dirty --tags
        WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
        OUTPUT_VARIABLE _bmp_version
        ERROR_QUIET
        OUTPUT_STRIP_TRAILING_WHITESPACE
    )
    message( STATUS "Farpatch version: ${_farpatch_build_version}")
    message( STATUS "Blackmagic version: ${_bmp_version}")
else()
    message(STATUS "GIT not found")
endif()
string(TIMESTAMP _time_stamp)
configure_file(
    ${CMAKE_SOURCE_DIR}/blackmagic/attack/include/version.h.in
    ${CMAKE_SOURCE_DIR}/blackmagic/attack/include/version.h
    @ONLY
)

# Include all C files in the specified directories
file(GLOB SOURCES
    "blackmagic/src/target/*.c"
    "blackmagic/src/*.c"
    "blackmagic/src/platforms/common/*.c"
    "attack/src/*.c"
)

# Exclude certain files
list(REMOVE_ITEM SOURCES
    "${CMAKE_SOURCE_DIR}/blackmagic/blackmagic/src/platforms/common/aux_serial.c"
    "${CMAKE_SOURCE_DIR}/blackmagic/blackmagic/src/platforms/common/cdcacm.c"
    "${CMAKE_SOURCE_DIR}/blackmagic/blackmagic/src/platforms/common/jtagtap.c"
    "${CMAKE_SOURCE_DIR}/blackmagic/blackmagic/src/platforms/common/swdptap.c"
    "${CMAKE_SOURCE_DIR}/blackmagic/blackmagic/src/platforms/common/usb_serial.c"
    "${CMAKE_SOURCE_DIR}/blackmagic/blackmagic/src/platforms/common/usb_dfu_stub.c"
    "${CMAKE_SOURCE_DIR}/blackmagic/blackmagic/src/platforms/common/usb.c"
    "${CMAKE_SOURCE_DIR}/blackmagic/blackmagic/src/target/jtagtap_generic.c"
    "${CMAKE_SOURCE_DIR}/blackmagic/blackmagic/src/target/swdptap_generic.c"
    "${CMAKE_SOURCE_DIR}/blackmagic/blackmagic/src/target/semihosting.c"
    "${CMAKE_SOURCE_DIR}/blackmagic/blackmagic/src/target/spi.c"
    "${CMAKE_SOURCE_DIR}/blackmagic/blackmagic/src/target/renesas_rz.c"
    "${CMAKE_SOURCE_DIR}/blackmagic/blackmagic/src/target/rp.c"
    "${CMAKE_SOURCE_DIR}/blackmagic/blackmagic/src/target/imxrt.c"
    "${CMAKE_SOURCE_DIR}/blackmagic/blackmagic/src/target/lpc43xx.c"
    "${CMAKE_SOURCE_DIR}/blackmagic/blackmagic/src/main.c"
    "${CMAKE_SOURCE_DIR}/blackmagic/blackmagic/src/gdb_main.c"
    "${CMAKE_SOURCE_DIR}/blackmagic/blackmagic/src/gdb_packet.c"
)

set(INCLUDE_DIRECTORIES
    "blackmagic/src/target"
    "blackmagic/src"
    "blackmagic/src/include"
    "blackmagic/src/platforms/common"
    "attack/include"
    ".."
)

add_library(blackmagic ${SOURCES})
add_definitions(-DPROBE_HOST=pico -DPC_HOSTED=0 -DENABLE_DEBUG=1 -DNO_LIBOPENCM3=1)
target_include_directories(blackmagic PRIVATE ${INCLUDE_DIRECTORIES})
target_link_libraries(blackmagic pico_stdlib)
