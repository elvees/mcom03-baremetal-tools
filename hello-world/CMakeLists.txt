cmake_minimum_required(VERSION 3.15)

project(Hello_World ASM C)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the build type" FORCE)
endif()

include_directories(../common)

foreach(CONFIG_REGION IN ITEMS xip${XIP_NOM} ram)
    set(FNAME hello-world-${CONFIG_ARCH}-${CONFIG_REGION})

    add_executable(${FNAME}.elf ../common/start-${CONFIG_ARCH}.S main.c ../common/uart.c)
    set_target_properties(${FNAME}.elf
        PROPERTIES LINK_FLAGS "-Wl,-Map=${FNAME}.map")
    add_custom_command(
        TARGET ${FNAME}.elf PRE_BUILD
        COMMAND cp ${PROJECT_SOURCE_DIR}/link.ld.in link.ld
        COMMAND sed -i 's/CONFIG_REGION/${CONFIG_REGION}/g' link.ld
        COMMAND sed -i 's/CONFIG_ARCH/${CONFIG_ARCH}/g' link.ld
        COMMAND sed -i 's/CONFIG_RAM_ADDR/${CONFIG_RAM_ADDR}/g' link.ld
        BYPRODUCTS link.ld ${FNAME}.map)
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
