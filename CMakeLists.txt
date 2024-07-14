cmake_minimum_required(VERSION 3.15)

project(BaremetalTools ASM C)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the build type" FORCE)
endif()

set(XTI_FREQUENCY 27000000 CACHE STRING "MCom-03 input frequency")
execute_process(COMMAND git describe --match=NeVeRmAtCh --always --abbrev=7 --dirty
    WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}" OUTPUT_VARIABLE GIT_SHA1_SHORT
    ERROR_QUIET OUTPUT_STRIP_TRAILING_WHITESPACE)
set(BUILD_ID "$ENV{BUILD_ID}")
add_compile_definitions(XTI_FREQUENCY=${XTI_FREQUENCY} GIT_SHA1_SHORT=${GIT_SHA1_SHORT}
    BUILD_ID=${BUILD_ID})

if(CMAKE_SYSTEM_PROCESSOR STREQUAL "aarch64")
    add_compile_options(-Wall -fno-builtin-malloc -ffreestanding -ffunction-sections
        -fdata-sections -fno-plt)
    add_link_options(-nostdlib -Wl,--gc-sections)
    set(XIP_NOM "1")
    set(CONFIG_ARCH "aarch64")
    set(RAM_ADDR "0x00000000")
    # Loading address must be away from U-Boot memory
    set(ELF_LOAD_ADDR "0x891000000")
else()
    add_definitions(-DMIPS32)
    add_compile_options(-Wall -EL -mips32 -mabi=32 -fno-builtin-malloc
        -ffreestanding -ffunction-sections -fdata-sections)
    add_link_options(-EL -nostdlib -Wl,--gc-sections -mips32 -mabi=32)
    set(XIP_NOM "0")
    set(CONFIG_ARCH "mips")
    set(RAM_ADDR "0x80000000")
endif()

add_subdirectory(hello-world)
add_subdirectory(spi-flasher)
