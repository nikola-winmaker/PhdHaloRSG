"""Configuration for the Sharedmemory protocol generator."""
from pathlib import Path


def get_protocol_config():
    """Return protocol generator configuration for HALO discovery."""
    module_dir = Path(__file__).parent
    render_module = module_dir / "render.py"

    return {
        "name": "sharedmemory",
        "supported_platforms": ["zephyr", "halo_gen_riscv64_h1_zephyr", "riscv64_h2_freertos", "halo_gen_riscv64_h3_baremetal", "halo_gen_riscv64_h4_linux"],
        "module": str(render_module),
        "description": "HALO protocol generator for SharedMemory",
    }