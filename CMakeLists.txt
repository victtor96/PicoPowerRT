cmake_minimum_required(VERSION 3.13)

# Pull in SDK (must be before project)
include(pico_sdk_import.cmake)

# Pull in FreeRTOS
include(FreeRTOS_Kernel_import.cmake)

project(example C CXX ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

add_executable(main
        main.cpp
        hooks.cpp
)

target_include_directories(main PRIVATE
        ${CMAKE_CURRENT_LIST_DIR}
)

target_link_libraries(main 
        pico_stdlib
        hardware_adc hardware_irq
        FreeRTOS-Kernel FreeRTOS-Kernel-Heap1
        pico_async_context_freertos
        pico_lwip_core pico_lwip_core4 pico_lwip_freertos pico_lwip_arch
)
pico_add_extra_outputs(main)
