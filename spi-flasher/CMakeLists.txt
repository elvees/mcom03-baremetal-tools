cmake_minimum_required(VERSION 3.15)

project(SPI_Flasher ASM C)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the build type" FORCE)
endif()

include_directories(../common)

set(REGIONS xip${XIP_NOM} ram)
if(${CONFIG_ARCH} STREQUAL "aarch64")
    set(REGIONS ${REGIONS} uboot)
endif()

foreach(REGION IN ITEMS ${REGIONS})
    if(${REGION} STREQUAL "uboot")
        set(CONFIG_RAM_ADDR ${ELF_LOAD_ADDR})
        set(CONFIG_REGION "ram")
    else()
        set(CONFIG_RAM_ADDR ${RAM_ADDR})
        set(CONFIG_REGION ${REGION})
    endif()

    set(FNAME spi-flasher-${CONFIG_ARCH}-${REGION})

    add_executable(${FNAME}.elf ../common/start-${CONFIG_ARCH}.S main.c  ../common/qspi.c
        ../common/uart.c)
    set_target_properties(${FNAME}.elf
        PROPERTIES LINK_FLAGS "-Wl,-Map=${FNAME}.map -T ${FNAME}.ld")

    if(${REGION} STREQUAL "uboot")
        target_compile_definitions(${FNAME}.elf PUBLIC CAN_RETURN)
    endif()

    add_custom_command(
        TARGET ${FNAME}.elf PRE_BUILD
        COMMAND cp ${PROJECT_SOURCE_DIR}/link.ld.in ${FNAME}.ld
        COMMAND sed -i 's/CONFIG_REGION/${CONFIG_REGION}/g' ${FNAME}.ld
        COMMAND sed -i 's/CONFIG_ARCH/${CONFIG_ARCH}/g' ${FNAME}.ld
        COMMAND sed -i 's/CONFIG_RAM_ADDR/${CONFIG_RAM_ADDR}/g' ${FNAME}.ld
        BYPRODUCTS ${FNAME}.ld ${FNAME}.map)
    add_custom_command(TARGET ${FNAME}.elf POST_BUILD COMMAND ${CMAKE_OBJDUMP}
        -S ${FNAME}.elf > ${FNAME}.dis
        BYPRODUCTS ${FNAME}.dis)
    add_custom_command(TARGET ${FNAME}.elf POST_BUILD COMMAND ${CMAKE_OBJCOPY}
        -O binary ${FNAME}.elf ${FNAME}.bin
        BYPRODUCTS ${FNAME}.bin)

    if(NOT "${REGION}" STREQUAL "uboot")
        # Use system objcopy because objcopy from MIPS try to use 64-bit addresses
        add_custom_command(TARGET ${FNAME}.elf POST_BUILD COMMAND objcopy
            -O ihex ${FNAME}.elf ${FNAME}.hex
            BYPRODUCTS ${FNAME}.hex)
        install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${FNAME}.hex DESTINATION .)
    endif()

    install(TARGETS ${FNAME}.elf DESTINATION .)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${FNAME}.bin DESTINATION .)
endforeach()
