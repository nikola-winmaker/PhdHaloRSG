"""Configuration for the Riscv64 H1 Zephyr platform generator."""
from __future__ import annotations

from pathlib import Path
from typing import Any, Dict


def get_generator_config() -> Dict[str, Any]:
    module_path = Path(__file__).parent / "render.py"
    return {
        "name": "riscv64_h1_zephyr",
        "description": "Zephyr RTOS running on riscv 64 U74",
        "outputDir": "output/riscv64_h1_zephyr",
        "template": None,
        "module": str(module_path),
        "options": {
            "os": True,
            "platform": "riscv64_h1_zephyr",
            "template_engine": "jinja2",
        },
        "entry_point": "render_riscv64_h1_zephyr",
    }