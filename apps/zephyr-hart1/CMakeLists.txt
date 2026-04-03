# Hart 1 - Zephyr RTOS
# This is a placeholder CMakeLists.txt for Zephyr integration
# Zephyr has its own build system; this integrates it with the main project

add_custom_target(hart1.elf
    COMMENT "Hart1 (Zephyr) - Configure via Zephyr's build system"
    COMMENT "See apps/zephyr-hart1/README.md for build instructions"
)

# Placeholder: Will be built via Zephyr's own CMake workflow
message(STATUS "Hart1 (Zephyr RTOS) - requires separate Zephyr build")
