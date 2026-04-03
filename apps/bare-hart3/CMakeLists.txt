add_executable(hart3.elf
    src/start.S
    src/main.c
    src/uart.c
)

target_include_directories(hart3.elf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

# Hart 3 specifics: U74 core (RV64GC + V extension capable)
set_target_properties(hart3.elf PROPERTIES
    LINK_FLAGS "-T${CMAKE_CURRENT_SOURCE_DIR}/linker.ld -march=rv64gc -mabi=lp64d"
)

add_custom_command(TARGET hart3.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary hart3.elf hart3.bin
    COMMENT "Generating Hart3 binary image: hart3.bin"
)
