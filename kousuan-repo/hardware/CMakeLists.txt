cmake_minimum_required(VERSION 3.13)

include($ENV{PICO_SDK_PATH}/external/pico_sdk_import.cmake)

project(kousuan C CXX ASM)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(kousuan pico_main.cpp)

target_link_libraries(kousuan
    pico_stdlib
    hardware_i2c
    hardware_pwm
    pico_rand
)

pico_enable_stdio_usb(kousuan 0)
pico_enable_stdio_uart(kousuan 1)
pico_add_extra_outputs(kousuan)
