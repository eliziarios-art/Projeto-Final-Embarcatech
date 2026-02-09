# == DO NOT EDIT THE FOLLOWING LINES for the Raspberry Pi Pico VS Code Extension to work ==
if(WIN32)
    set(USERHOME $ENV{USERPROFILE})
else()
    set(USERHOME $ENV{HOME})
endif()
set(sdkVersion 2.2.0)
set(toolchainVersion 14_2_Rel1)
set(picotoolVersion 2.2.0-a4)
set(picoVscode ${USERHOME}/.pico-sdk/cmake/pico-vscode.cmake)
if (EXISTS ${picoVscode})
    include(${picoVscode})
endif()
# ====================================================================================
cmake_minimum_required(VERSION 3.13)

set(PICO_BOARD pico_w)
include(pico_sdk_import.cmake)

project(teste_freertos C CXX)
set(CMAKE_C_STANDARD 11)
set(CMAKE_CXX_STANDARD 17)

pico_sdk_init()

# Import do FreeRTOS
set(FREERTOS_KERNEL_PATH "${CMAKE_CURRENT_LIST_DIR}/FreeRTOS-Kernel")
include("${FREERTOS_KERNEL_PATH}/portable/ThirdParty/GCC/RP2040/FreeRTOS_Kernel_import.cmake")

add_executable(teste_freertos
    freertos_hooks.c
    main.c
    src/soil.c
    src/aht10.c
    src/system_state.c
    src/tasks.c
    src/wifi_manager.c
    src/mqtt_task.c
)

# --- SOLUÇÃO FORÇADA PARA O SDK 2.2.0 ---
# Isso garante que o compilador ache o pico/cyw43_arch.h independente do link
#target_include_directories(teste_freertos SYSTEM PRIVATE
#    ${PICO_SDK_PATH}/src/rp2_common/pico_cyw43_arch/include
#)
# ----------------------------------------

target_include_directories(teste_freertos PRIVATE
    ${CMAKE_CURRENT_LIST_DIR}
    ${CMAKE_CURRENT_LIST_DIR}/Config
    ${CMAKE_CURRENT_LIST_DIR}/src
)

target_link_libraries(teste_freertos
    pico_stdlib
    pico_cyw43_arch_lwip_sys_freertos
    pico_lwip_mqtt
    FreeRTOS-Kernel
    FreeRTOS-Kernel-Heap4
    hardware_adc
    hardware_i2c
)

pico_enable_stdio_usb(teste_freertos 1)
pico_add_extra_outputs(teste_freertos)