"""Render module for the Riscv64 H2 Freertos platform generator."""
from __future__ import annotations

from pathlib import Path
from typing import Any, Dict

from jinja2 import Environment, FileSystemLoader, select_autoescape

try:
    from generator.codegen.render.render_utils import write_generated_file
except ImportError:
    import sys

    sw_path = Path(__file__).parent.parent.parent.parent.parent / "sw"
    if str(sw_path) not in sys.path:
        sys.path.insert(0, str(sw_path))
    from generator.codegen.render.render_utils import write_generated_file


def _has_profile_type_connections(model, profile_type):
    """Check if any connections use profiles of a specific type.
    
    Args:
        model: Platform model with profiles and connections
        profile_type: Profile type to check for (e.g., 'sharedmemory', 'dma', 'mmio')
        
    Returns:
        bool: True if any connection uses a profile of the specified type
    """
    profiles = model.get('profiles', {})
    connections = model.get('connections', [])
    
    for conn in connections:
        profile_name = conn.get('profile', '')
        if profile_name and profile_name in profiles:
            profile = profiles[profile_name]
            if profile.get('type', '').lower() == profile_type.lower():
                return True
    return False


def _is_protocol_generator_available(protocol_type):
    """Check if a protocol generator is available in the system.
    
    Args:
        protocol_type: Protocol type to check (e.g., 'eventchannel', 'sharedmemory')
        
    Returns:
        bool: True if generator is installed
    """
    try:
        from generator.codegen.utils import is_protocol_generator_available as check_available
        return check_available(protocol_type)
    except ImportError:
        return False


def render_riscv64_h2_freertos(
    *,
    model: Dict[str, Any],
    analysis: Dict[str, Any],
    environment: str,
    output_dir: Path,
) -> None:
    """Render code for the target platform."""
    template_dir = Path(__file__).parent / "templates"
    jenv = Environment(
        loader=FileSystemLoader(str(template_dir)),
        autoescape=select_autoescape(),
        trim_blocks=True,
        lstrip_blocks=True,
    )

    jenv.filters["upper"] = str.upper
    jenv.filters["lower"] = str.lower

    context = {
        "environment": environment,
        "components": model.get("components", []),
        "connections": model.get("connections", []),
        "interfaces": model.get("interfaces", {}),
        "profiles": model.get("profiles", {}),
        "model": model,
        "analysis": analysis,
        "has_os": True,
        "_has_profile_type_connections": _has_profile_type_connections,
        "_is_protocol_generator_available": _is_protocol_generator_available,
    }

    platform_include_dir = output_dir / "include"
    platform_src_dir = output_dir / "src"

    platform_include_dir.mkdir(parents=True, exist_ok=True)
    platform_src_dir.mkdir(parents=True, exist_ok=True)
    _render_template(
        jenv,
        "platform_init.c.j2",
        context,
        platform_src_dir / f"{environment}_init.c",
    )
    _render_template(
        jenv,
        "platform_init.h.j2",
        context,
        platform_include_dir / f"{environment}_init.h",
    )
    _render_template(
        jenv,
        "config.h.j2",
        context,
        platform_include_dir / f"{environment}_config.h",
    )


def _render_template(
    jenv: Environment,
    template_name: str,
    context: Dict[str, Any],
    destination: Path,
) -> None:
    write_generated_file(destination, jenv.get_template(template_name).render(context))
