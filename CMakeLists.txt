cmake_minimum_required(VERSION 3.15)

project(BaremetalTools ASM C)

set(XTI_FREQUENCY 27000000 CACHE STRING "MCom-03 input frequency")
add_definitions(-DXTI_FREQUENCY=${XTI_FREQUENCY})

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    add_compile_options(-Wall -fno-builtin-malloc -ffreestanding -ffunction-sections -fdata-sections)
    add_link_options(-T link.ld -nostdlib -Wl,--gc-sections)
    set(XIP_NOM "1")
    set(CONFIG_ARCH "aarch64")
    set(CONFIG_RAM_ADDR "0x00000000")
else()
    add_definitions(-DMIPS32)
    add_compile_options(-Wall -EL -mips32 -mabi=32 -fno-builtin-malloc
        -ffreestanding -ffunction-sections -fdata-sections)
    add_link_options(-T link.ld -EL -nostdlib
        -Wl,--gc-sections -mips32 -mabi=32)
    set(XIP_NOM "0")
    set(CONFIG_ARCH "mips")
    set(CONFIG_RAM_ADDR "0x80000000")
endif()

add_subdirectory(hello-world)
add_subdirectory(spi-flasher)
