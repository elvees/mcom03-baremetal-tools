cmake_minimum_required(VERSION 3.15)

project(Soak_Test ASM C)

include_directories(../common)

set(REGIONS xip${XIP_NOM} ram)

if(${CONFIG_ARCH} STREQUAL "mips")
    foreach(REGION IN ITEMS ${REGIONS})
        foreach(BOARD IN ITEMS "pmi" "smarc3")
            foreach(CFG IN ITEMS "min" "max" "cpu")
                set(CONFIG_RAM_ADDR ${RAM_ADDR})
                set(CONFIG_REGION ${REGION})

                set(FNAME soak-test-${CONFIG_ARCH}-${REGION}-${BOARD}-${CFG})

                add_executable(${FNAME}.elf ../common/start-${CONFIG_ARCH}.S main.c ../common/uart.c
                    ../common/delay.c ../common/pvt.c ../common/clk.c)

                if(${BOARD} STREQUAL "pmi")
                    set(BOARD_DEFINE "-DIS_PMI_BOARD")
                elseif(${BOARD} STREQUAL "smarc3")
                    set(BOARD_DEFINE "-DIS_SMARC3_BOARD")
                endif()

                if(${CFG} STREQUAL "min")
                    target_compile_definitions(${FNAME}.elf PUBLIC ${BOARD_DEFINE})
                elseif(${CFG} STREQUAL "max")
                    target_compile_definitions(${FNAME}.elf PUBLIC ${BOARD_DEFINE} -DUSE_CPU
                        -DUSE_SDR -DUSE_MEDIA -DUSE_HSP -DUSE_DDR)
                elseif(${CFG} STREQUAL "cpu")
                    target_compile_definitions(${FNAME}.elf PUBLIC ${BOARD_DEFINE} -DUSE_CPU)
                endif()

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

                install(TARGETS ${FNAME}.elf DESTINATION .)
                install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${FNAME}.bin DESTINATION .)
            endforeach()
        endforeach()
    endforeach()
endif()
