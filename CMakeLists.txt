cmake_minimum_required(VERSION 3.13)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

# Pull in FreeRTOS
include(FreeRTOS_Kernel_import.cmake)

project(example C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)
set(PICO_BOARD pico_w)

pico_sdk_init()

add_executable(main
        main.cpp
        hooks.cpp
)

target_compile_definitions(main PRIVATE
        WIFI_SSID=\"$ENV{WIFI_SSID}\"
        WIFI_PASSWORD=\"$ENV{WIFI_PASSWORD}\"
)

target_include_directories(main PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(main 
        pico_stdlib
        hardware_adc
        hardware_irq
        FreeRTOS-Kernel-Heap4
        pico_cyw43_arch_lwip_sys_freertos
)
pico_add_extra_outputs(main)
