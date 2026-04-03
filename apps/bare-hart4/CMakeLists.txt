add_executable(hart4.elf
    src/start.S
    src/main.c
    src/uart.c
)

target_include_directories(hart4.elf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_SOURCE_DIR}/src
)

# Hart 4 specifics: U74 core
set_target_properties(hart4.elf PROPERTIES
    LINK_FLAGS "-T${CMAKE_CURRENT_SOURCE_DIR}/linker.ld -march=rv64gc -mabi=lp64d"
)

add_custom_command(TARGET hart4.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary hart4.elf hart4.bin
    COMMENT "Generating Hart4 binary image: hart4.bin"
)
