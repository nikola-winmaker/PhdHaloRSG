"""Configuration for the Blackboard protocol generator."""
from pathlib import Path


def get_protocol_config():
    """Return protocol generator configuration for HALO discovery."""
    module_dir = Path(__file__).parent
    render_module = module_dir / "render.py"

    return {
        "name": "blackboard",
        "supported_platforms": ["riscv64_h1_zephyr", "riscv64_h2_freertos", "riscv64_h3_baremetal", "riscv64_h4_linux"],
        "module": str(render_module),
        "description": "Blackboard/Sampled Port Protocol - One writer updates latest value, readers always read newest snapshot",
    }