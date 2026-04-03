add_executable(hart0.elf
    bootstrap.S
    main.c
)

target_include_directories(hart0.elf PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_SOURCE_DIR}/src
)

# Hart 0 specifics: S7 core, runs in M-mode (Machine mode)
set_target_properties(hart0.elf PROPERTIES
    LINK_FLAGS "-T${CMAKE_CURRENT_SOURCE_DIR}/linker.ld -march=rv64imac -mabi=lp64"
)

# Custom command to generate binary
add_custom_command(TARGET hart0.elf POST_BUILD
    COMMAND ${CMAKE_OBJCOPY} -O binary hart0.elf hart0.bin
    COMMENT "Generating Hart0 binary image: hart0.bin"
)
