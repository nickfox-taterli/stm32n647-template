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

project(fsbl C ASM)

# ──────────────────────── compile flags ────────────────────────
set(CPU_FLAGS -mcpu=cortex-m55 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard)
set(DEFS -DSTM32N647xx -DUSE_FULL_LL_DRIVER)
set(WARN -Wall -Wextra -Wpedantic)
set(OPT  -Og -g3)
set(SEC  -fdata-sections -ffunction-sections -fno-builtin -fno-common)

# ═══════════════════════ FSBL ELF ═══════════════════════
set(FSBL_ELF ${CMAKE_PROJECT_NAME}.elf)

add_executable(${FSBL_ELF}

    # startup
    Startup/startup_stm32n647x0hxq_fsbl.s

    # Application
    Core/Src/main.c
    Core/Src/stm32n6xx_it.c
    Core/Src/system_stm32n6xx_fsbl.c
    Core/Src/syscalls.c

    # BSP
    Drivers/BSP/NORFlash/norflash.c
    Drivers/BSP/HyperRAM/hyperram.c

    # Middleware (boot)
    Middlewares/STM32_ExtMem_Manager/boot/stm32_boot_xip.c

    # LL drivers
    Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_ll_exti.c
    Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_ll_gpio.c
    Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_ll_pwr.c
    Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_ll_rcc.c
    Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_ll_utils.c
    Drivers/STM32N6xx_HAL_Driver/Src/stm32n6xx_ll_xspi.c
)

target_include_directories(${FSBL_ELF} PRIVATE

    # Application
    ${CMAKE_SOURCE_DIR}/Core/Inc

    # CMSIS
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Include
    ${CMAKE_SOURCE_DIR}/Drivers/CMSIS/Device/Include

    # LL drivers
    ${CMAKE_SOURCE_DIR}/Drivers/STM32N6xx_HAL_Driver/Inc

    # BSP
    ${CMAKE_SOURCE_DIR}/Drivers/BSP/NORFlash
    ${CMAKE_SOURCE_DIR}/Drivers/BSP/HyperRAM

    # Middleware
    ${CMAKE_SOURCE_DIR}/Middlewares/STM32_ExtMem_Manager
    ${CMAKE_SOURCE_DIR}/Middlewares/STM32_ExtMem_Manager/boot
)

target_compile_options(${FSBL_ELF} PRIVATE
    ${CPU_FLAGS} -mcmse ${DEFS} ${WARN} ${OPT} ${SEC}
    $<$<COMPILE_LANGUAGE:ASM>:${CPU_FLAGS}>
)

target_link_options(${FSBL_ELF} PRIVATE
    ${CPU_FLAGS} -mcmse
    -T${CMAKE_SOURCE_DIR}/ld/STM32N647X0HXQ_AXISRAM2_fsbl.ld
    -Wl,--gc-sections
    -Wl,--print-memory-usage
    -specs=nano.specs
    -Wl,-Map=${CMAKE_BINARY_DIR}/${CMAKE_PROJECT_NAME}.map
)

# ═══════════════════ post-build: BIN + HEX + Sign ════════════════════
set(CUBECLT_ROOT "/opt/st/stm32cubeclt_1.22.0" CACHE PATH
    "STM32CubeCLT installation root")
set(SIGNTOOL "${CUBECLT_ROOT}/STM32CubeProgrammer/bin/STM32_SigningTool_CLI"
    CACHE FILEPATH "STM32 Signing Tool executable")
if(NOT EXISTS "${SIGNTOOL}")
    message(FATAL_ERROR "STM32 Signing Tool not found: ${SIGNTOOL}")
endif()

add_custom_command(TARGET ${FSBL_ELF} POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -Obinary  ${FSBL_ELF} ${CMAKE_PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -Oihex    ${FSBL_ELF} ${CMAKE_PROJECT_NAME}.hex
    COMMAND ${SIGNTOOL} -s -bin ${CMAKE_PROJECT_NAME}.bin -nk -of 0x80000000 -t fsbl -o ${CMAKE_PROJECT_NAME}-trusted.bin -hv 2.3 -align -dump ${CMAKE_PROJECT_NAME}-trusted.bin
    COMMAND ${CMAKE_OBJCOPY} -I binary ${CMAKE_PROJECT_NAME}-trusted.bin --change-addresses 0x70000000 -O ihex ${CMAKE_PROJECT_NAME}-trusted.hex
    COMMAND ${CMAKE_SIZE}     ${FSBL_ELF}
    COMMENT "Post-build: generating BIN, HEX, signed trusted BIN/HEX"
)
