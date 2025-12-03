cmake_minimum_required(VERSION 3.15)

project(Arm_Hang_Test_Monitor ASM C)

macro(build_and_install target_name additional_compile_defs text_region)
    add_executable(${target_name}.elf startup.S main.c
        ../common/clk.c ../common/delay.c ../common/uart.c ../common/wdt.c ../common/cpu.c)

    target_compile_definitions(${target_name}.elf PUBLIC ${additional_compile_defs})

    set_target_properties(${target_name}.elf
        PROPERTIES LINK_FLAGS "-Wl,-Map=${target_name}.map -T ${target_name}.ld")

    add_custom_command(TARGET ${target_name}.elf PRE_BUILD
        COMMAND cp ${PROJECT_SOURCE_DIR}/link.ld.in ${target_name}.ld
        COMMAND sed -i 's/CONFIG_REGION/${text_region}/g' ${target_name}.ld
        BYPRODUCTS ${target_name}.ld ${target_name}.map)

    add_custom_command(TARGET ${target_name}.elf POST_BUILD COMMAND ${CMAKE_OBJDUMP}
        -S ${target_name}.elf > ${target_name}.dis BYPRODUCTS ${target_name}.dis)

    add_custom_command(TARGET ${target_name}.elf POST_BUILD COMMAND ${CMAKE_OBJCOPY}
        -O binary ${target_name}.elf ${target_name}.bin BYPRODUCTS ${target_name}.bin)

    install(TARGETS ${target_name}.elf DESTINATION .)
    install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${target_name}.bin DESTINATION .)
endmacro()

include_directories(../common)

if(${CONFIG_ARCH} STREQUAL "mips")
    build_and_install(ahtm-xip0 "" xip0)
    build_and_install(ahtm-jtag "-DJTAG_BUILD" cram)
endif()

install(FILES ${PROJECT_SOURCE_DIR}/ahtm-run.mdb DESTINATION .)
