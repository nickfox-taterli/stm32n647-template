cmake_minimum_required(VERSION 3.20)

set(CMAKE_SYSTEM_NAME Generic)
set(CMAKE_SYSTEM_PROCESSOR ARM)

set(TOOLCHAIN_PREFIX arm-none-eabi-)
set(CMAKE_C_COMPILER   ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_CXX_COMPILER ${TOOLCHAIN_PREFIX}g++)
set(CMAKE_ASM_COMPILER ${TOOLCHAIN_PREFIX}gcc)
set(CMAKE_AR           ${TOOLCHAIN_PREFIX}ar)
set(CMAKE_OBJCOPY      ${TOOLCHAIN_PREFIX}objcopy)
set(CMAKE_OBJDUMP      ${TOOLCHAIN_PREFIX}objdump)
set(CMAKE_SIZE         ${TOOLCHAIN_PREFIX}size)

set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_C_STANDARD 11)

set(CMAKE_TRY_COMPILE_TARGET_TYPE STATIC_LIBRARY)

project(app C ASM)

set(CMSIS_INC
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/ST/STM32N6xx/Include
)
set(LL_INC ${CMAKE_SOURCE_DIR}/Drivers/STM32N6xx_HAL_Driver/Inc)

set(LL_SRCS
    Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_ll_gpio.c
    Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_ll_pwr.c
    Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_ll_rcc.c
    Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_ll_utils.c
)

set(APP_SRCS
    Core/Src/main.c
    Core/Src/stm32n6xx_it.c
    Core/Src/system_stm32n6xx_s.c
    Core/Src/secure_nsc.c
    Core/Src/syscalls.c
    Core/Src/sysmem.c
)

set(STARTUP Startup/startup_stm32n647x0hxq.s)

set(CPU_FLAGS -mcpu=cortex-m55 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard)
set(DEFS -DSTM32N647xx -DUSE_FULL_LL_DRIVER)
set(WARN -Wall -Wextra -Wpedantic)
set(OPT  -Og -g3)
set(SEC  -fdata-sections -ffunction-sections -fno-builtin -fno-common)

set(APP_ELF ${CMAKE_PROJECT_NAME}.elf)

add_executable(${APP_ELF}
    ${STARTUP}
    ${APP_SRCS}
    ${LL_SRCS}
)

target_include_directories(${APP_ELF} PRIVATE
    ${CMAKE_SOURCE_DIR}/Core/Inc
    ${CMAKE_SOURCE_DIR}/Secure_nsclib
    ${CMSIS_INC}
    ${LL_INC}
)

target_compile_options(${APP_ELF} PRIVATE
    ${CPU_FLAGS} -mcmse ${DEFS} ${WARN} ${OPT} ${SEC}
    $<$<COMPILE_LANGUAGE:ASM>:${CPU_FLAGS}>
)

target_link_options(${APP_ELF} PRIVATE
    ${CPU_FLAGS} -mcmse
    -T${CMAKE_SOURCE_DIR}/ld/STM32N647X0HXQ_ROMxspi2.ld
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -specs=nano.specs -specs=nosys.specs
    -Wl,-Map=${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map
)

add_custom_command(TARGET ${APP_ELF} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Obinary ${APP_ELF} ${CMAKE_PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -Oihex   ${APP_ELF} ${CMAKE_PROJECT_NAME}.hex
    COMMAND ${CMAKE_SIZE}     ${APP_ELF}
    COMMENT "Post-build: generating BIN and HEX"
)
