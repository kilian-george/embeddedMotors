cmake_minimum_required(VERSION 3.13)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

# For CS466 my labs and documentation expect all class material to be based off a 
# single directory.  In my case this is ${HOME}/src You are free to change this to match 
# your environment but to use all the CMakeLists.txt files I supply having a defined root dir
# will make your life easier.
#
set(MY_ROOT_DIR $ENV{HOME}/WSU/SPRING_2026/CS466-EmbeddedSystems/Projects)
message(STATUS "Using project ROOT dir = ${MY_ROOT_DIR}")

# 
# set a define for the FreeRTOS Kernel
#
set(FREERTOS_KERNEL ${MY_ROOT_DIR}/FreeRTOS-LTS/FreeRTOS/FreeRTOS-Kernel)

#
# An option to select the Pico-2 board (C33 core) vs the original Pico board (M0+ core)  
#
option(ENABLE_PICO_2 "Set ON tocompile for Pico2 (C33 core) otherwise set OFF for orginal Pico (M0+ core)" OFF)

if(ENABLE_PICO_2)
    set(PICO_BOARD pico2)
    set(PICO_PLATFORM rp2350-arm-s)
    set(destination_file "/media/$ENV{USER}/RP2350")
    set(FREERTOS_PORT_DIR portable/GCC/ARM_CM33)
    set(FREERTOS_PORT GCC_ARM_CM33)
else()
    set(PICO_BOARD pico)        # or pico_w / pico2 / etc.
    set(PICO_PLATFORM rp2040)   # or rp2350 if using Pico 2
    set(destination_file "/media/$ENV{USER}/RPI-RP2")
    set(FREERTOS_PORT_DIR portable/ThirdParty/GCC/RP2040)
    set(FREERTOS_PORT GCC_ARM_CM0)
endif()

# Pull in Pico SDK (must be done before project)
message(STATUS "include(pico_sdk_import.cmake)")
include(${MY_ROOT_DIR}/pico/pico-sdk/external/pico_sdk_import.cmake)

# Pull in FreeRTOS (must be done before project)
message(STATUS "include(FreeRTOS_Kernel_import.cmake)")
include(${FREERTOS_KERNEL}/${FREERTOS_PORT_DIR}/FreeRTOS_Kernel_import.cmake)


# 
# Project Definition!
# 
project(slave C CXX ASM)

# 
# This will go do Pico things, Honestly I've never traced it.
#
pico_sdk_init()

# 
# these are the executables that you use for the lab.   If you have multiple files 
# just add them here.
#
add_executable(${PROJECT_NAME}
    slave.c
    ../common/pwm.c
    ../common/motor.c
    ../common/myAssert.c
)

pico_enable_stdio_usb(${PROJECT_NAME} 1)
pico_enable_stdio_uart(${PROJECT_NAME} 0)

# 
# If you have extra include dirs (outside pico-sdk and FreeRTOS) add them here.
#
target_include_directories(${PROJECT_NAME} PRIVATE
    .
    ../common
)


target_compile_definitions(${PROJECT_NAME} PRIVATE
    PICO_STDIO_STACK_BUFFER_SIZE=64 # use a small printf on stack buffer
)

target_link_libraries(${PROJECT_NAME} 
    pico_stdlib 
    FreeRTOS-Kernel 
    FreeRTOS-Kernel-Heap3
    hardware_pwm
)

pico_add_extra_outputs(${PROJECT_NAME})

set(CMAKE_BUILD_TYPE Debug)