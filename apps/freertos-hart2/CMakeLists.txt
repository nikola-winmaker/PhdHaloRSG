# Hart 2 - FreeRTOS
# Placeholder for FreeRTOS integration

add_custom_target(hart2.elf
    COMMENT "Hart2 (FreeRTOS) - Configure via FreeRTOS build system"
    COMMENT "See apps/freertos-hart2/README.md for build instructions"
)

# FreeRTOS will be built separately or via custom build rules
message(STATUS "Hart2 (FreeRTOS) - requires separate FreeRTOS build")
