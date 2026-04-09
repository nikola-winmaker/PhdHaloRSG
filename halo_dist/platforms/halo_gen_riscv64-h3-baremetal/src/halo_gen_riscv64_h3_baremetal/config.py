"""Configuration for the Riscv64 H3 Baremetal platform generator."""
from __future__ import annotations

from pathlib import Path
from typing import Any, Dict


def get_generator_config() -> Dict[str, Any]:
    module_path = Path(__file__).parent / "render.py"
    return {
        "name": "riscv64_h3_baremetal",
        "description": "Bare-metal application running on riscv 64 U74",
        "outputDir": "output/riscv64_h3_baremetal",
        "template": None,
        "module": str(module_path),
        "options": {
            "os": False,
            "platform": "riscv64_h3_baremetal",
            "template_engine": "jinja2",
        },
        "entry_point": "render_riscv64_h3_baremetal",
    }