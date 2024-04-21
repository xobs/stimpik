cmake_minimum_required(VERSION 3.13)

if(NOT DEFINED PICO_SDK_PATH)
    set(PICO_SDK_PATH "/usr/share/pico-sdk")
endif()

# initialize the SDK based on PICO_SDK_PATH
# note: this must happen before project()
include(pico_sdk_import.cmake)

project(attack C CXX ASM)

# initialize the Raspberry Pi Pico SDK
pico_sdk_init()

add_executable(attack
    stimpik.c
    bmp-loader.c
)

add_subdirectory(blackmagic)

# Add pico_stdlib library which aggregates commonly used features
target_link_libraries(attack pico_stdlib 
    hardware_uart 
    hardware_pwm
    blackmagic
)

pico_enable_stdio_usb(attack 1)

pico_add_extra_outputs(attack)

# Build the payload
add_custom_command(OUTPUT ${CMAKE_CURRENT_SOURCE_DIR}/payload.h
    COMMAND ${CMAKE_COMMAND} -E make_directory ${CMAKE_BINARY_DIR}/payload
    COMMAND ${CMAKE_C_COMPILER}
                -mthumb -mcpu=cortex-m3 -g3 -c
                ${CMAKE_CURRENT_SOURCE_DIR}/payload/start.S
                -o ${CMAKE_BINARY_DIR}/payload/start.o
    COMMAND ${CMAKE_C_COMPILER}
                -mthumb -mcpu=cortex-m3 -Os -g3 -c -Wall
                ${CMAKE_CURRENT_SOURCE_DIR}/payload/payload.c
                -o ${CMAKE_BINARY_DIR}/payload/payload.o
    COMMAND ${CMAKE_C_COMPILER}
                -nostartfiles -mcpu=cortex-m3 -mthumb -g3
                -T ${CMAKE_CURRENT_SOURCE_DIR}/payload/payload.ld
                ${CMAKE_BINARY_DIR}/payload/start.o
                ${CMAKE_BINARY_DIR}/payload/payload.o
                -o ${CMAKE_BINARY_DIR}/payload.elf
    COMMAND ${CMAKE_OBJCOPY} -O binary ${CMAKE_BINARY_DIR}/payload.elf ${CMAKE_BINARY_DIR}/payload.bin
    COMMAND ${CMAKE_COMMAND} -Dinput=${CMAKE_BINARY_DIR}/payload.bin
                             -Doutput=${CMAKE_CURRENT_SOURCE_DIR}/payload.h
                             -P ${CMAKE_CURRENT_SOURCE_DIR}/payload/Postprocess.cmake
    DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/payload/start.S
            ${CMAKE_CURRENT_SOURCE_DIR}/payload/payload.c
            ${CMAKE_CURRENT_SOURCE_DIR}/payload/payload.ld
            ${CMAKE_CURRENT_SOURCE_DIR}/payload/Postprocess.cmake
    COMMENT "Building payload.bin"
)
# Add the binary file to the target
add_custom_target(payload ALL DEPENDS ${CMAKE_CURRENT_SOURCE_DIR}/payload.h)

# Include the binary file in the main target
add_dependencies(attack payload)
