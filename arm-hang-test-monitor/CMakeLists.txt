cmake_minimum_required(VERSION 3.15)

project(Arm_Hang_Test_Monitor ASM C)

include_directories(../common)

set(FNAME arm-hang-test-monitor)

if(${CONFIG_ARCH} STREQUAL "mips")
    add_executable(${FNAME}.elf startup.S main.c
        ../common/clk.c ../common/delay.c ../common/uart.c ../common/wdt.c ../common/cpu.c)

    set_target_properties(${FNAME}.elf
        PROPERTIES LINK_FLAGS "-Wl,-Map=${FNAME}.map -T ${PROJECT_SOURCE_DIR}/link.ld")

    add_custom_command(TARGET ${FNAME}.elf POST_BUILD COMMAND ${CMAKE_OBJDUMP}
        -S ${FNAME}.elf > ${FNAME}.dis BYPRODUCTS ${FNAME}.dis)

    add_custom_command(TARGET ${FNAME}.elf POST_BUILD COMMAND ${CMAKE_OBJCOPY}
        -O binary ${FNAME}.elf ${FNAME}.bin BYPRODUCTS ${FNAME}.bin)

    install(TARGETS ${FNAME}.elf DESTINATION .)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${FNAME}.bin DESTINATION .)
endif()
