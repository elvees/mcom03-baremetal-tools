cmake_minimum_required(VERSION 3.15)

project(otp-flasher ASM C)

include_directories(../common)
include_directories(.)

set(REGIONS xip${XIP_NOM} ram)

foreach(REGION IN ITEMS ${REGIONS})
    set(CONFIG_RAM_ADDR ${RAM_ADDR})
    set(CONFIG_REGION ${REGION})

    set(FNAME otp-flasher-${CONFIG_ARCH}-${REGION})

    add_executable(${FNAME}.elf ../common/start-${CONFIG_ARCH}.S main.c
        snps_ssi.c otp.c ../common/console.c ../common/delay.c ../common/uart.c)
    set_target_properties(${FNAME}.elf
        PROPERTIES LINK_FLAGS "-Wl,-Map=${FNAME}.map -T ${FNAME}.ld")

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

    # Use system objcopy because objcopy from MIPS try to use 64-bit addresses
    add_custom_command(TARGET ${FNAME}.elf POST_BUILD COMMAND objcopy
        -O ihex ${FNAME}.elf ${FNAME}.hex
        BYPRODUCTS ${FNAME}.hex)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${FNAME}.hex DESTINATION .)

    install(TARGETS ${FNAME}.elf DESTINATION .)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${FNAME}.bin DESTINATION .)
endforeach()
