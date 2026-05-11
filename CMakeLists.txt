cmake_minimum_required(VERSION 3.16)

set(CMAKE_TOOLCHAIN_FILE ${CMAKE_CURRENT_SOURCE_DIR}/cmake/toolchain-arm-none-eabi.cmake)

project(stm32f103_uart C)
set(CMAKE_C_STANDARD 11)

set(CPU_FLAGS "-mcpu=cortex-m3 -mthumb")

set(CMAKE_C_FLAGS
    "${CPU_FLAGS} -O2 -Wall -Wextra -fdata-sections -ffunction-sections \
     -DCFG_TUSB_MCU=OPT_MCU_STM32F1 -DSTM32F103xB")

set(CMAKE_EXE_LINKER_FLAGS
    "${CPU_FLAGS} \
    -T${CMAKE_CURRENT_SOURCE_DIR}/ld/STM32F103C8TX_FLASH.ld \
    -Wl,--gc-sections \
    -Wl,-Map=${PROJECT_NAME}.map \
    --specs=nano.specs \
    --specs=nosys.specs")

include_directories(
    include
    lib/tinyusb/src
    lib/tinyusb/src/portable/st/stm32_fsdev
)

# TinyUSB sources — only what we need for device-mode CDC
set(TINYUSB_SOURCES
    lib/tinyusb/src/tusb.c
    lib/tinyusb/src/common/tusb_fifo.c
    lib/tinyusb/src/device/usbd.c
    lib/tinyusb/src/device/usbd_control.c
    lib/tinyusb/src/class/cdc/cdc_device.c
    lib/tinyusb/src/portable/st/stm32_fsdev/dcd_stm32_fsdev.c
)

add_executable(${PROJECT_NAME}.elf
    startup/startup_stm32f103xb.c
    src/main.c
    src/st7735.c
    src/usb_descriptors.c
    src/usb_cdc_app.c
    ${TINYUSB_SOURCES}
)

add_custom_command(TARGET ${PROJECT_NAME}.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary
            $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.bin
    COMMAND ${CMAKE_OBJCOPY} -O ihex
            $<TARGET_FILE:${PROJECT_NAME}.elf> ${PROJECT_NAME}.hex
    COMMAND ${CMAKE_SIZE} $<TARGET_FILE:${PROJECT_NAME}.elf>
)

add_custom_target(flash
    COMMAND openocd -f ${CMAKE_CURRENT_SOURCE_DIR}/openocd.cfg
            -c "program ${PROJECT_NAME}.bin verify reset exit 0x08000000"
    DEPENDS ${PROJECT_NAME}.elf
    COMMENT "Flashing via J-Link SWD"
)
