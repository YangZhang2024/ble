cmake_minimum_required(VERSION 3.22)
project(stm32f1xx_basic_template C ASM)
set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(HAL_DIR Drivers/STM32F1xx_HAL_Driver)
set(STM32_TARGET "RCT6" CACHE STRING "STM32 target: RCT6 or C8T6")
if(STM32_TARGET STREQUAL "C8T6")
    set(STM32_DEVICE_DEFINE STM32F103xB)
    set(STARTUP_FILE startup_stm32f103xb.s)
else()
    set(STM32_DEVICE_DEFINE STM32F103xE)
    set(STARTUP_FILE startup_stm32f103xe.s)
endif()
add_executable(${PROJECT_NAME}
    Core/Src/main.c Core/Src/soft_i2c.c Drivers/LCD/lcd.c Core/Src/stm32f1xx_it.c Core/Src/system_stm32f1xx.c Core/Src/sysmem.c Core/Src/syscalls.c
    ${STARTUP_FILE}
    ${HAL_DIR}/Src/stm32f1xx_hal.c
    ${HAL_DIR}/Src/stm32f1xx_hal_cortex.c
    ${HAL_DIR}/Src/stm32f1xx_hal_rcc.c
    ${HAL_DIR}/Src/stm32f1xx_hal_rcc_ex.c
    ${HAL_DIR}/Src/stm32f1xx_hal_gpio.c
    ${HAL_DIR}/Src/stm32f1xx_hal_gpio_ex.c
    ${HAL_DIR}/Src/stm32f1xx_hal_flash.c
    ${HAL_DIR}/Src/stm32f1xx_hal_flash_ex.c
    ${HAL_DIR}/Src/stm32f1xx_hal_pwr.c
)
target_include_directories(${PROJECT_NAME} PRIVATE Core/Inc Drivers/LCD ${HAL_DIR}/Inc ${HAL_DIR}/Inc/Legacy Drivers/CMSIS/Include Drivers/CMSIS/Device/ST/STM32F1xx/Include)
target_compile_definitions(${PROJECT_NAME} PRIVATE USE_HAL_DRIVER ${STM32_DEVICE_DEFINE})
target_link_libraries(${PROJECT_NAME} PRIVATE m)
add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD COMMAND ${CMAKE_OBJCOPY} -O ihex $<TARGET_FILE:${PROJECT_NAME}> ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}.hex VERBATIM)

