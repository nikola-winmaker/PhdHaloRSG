"""Blackboard protocol renderer.

Provides platform-specific rendering for the blackboard protocol.
"""
from __future__ import annotations

import importlib.util
import logging
from pathlib import Path
from typing import Any, Dict

from jinja2 import Environment, FileSystemLoader, select_autoescape

try:
    from generator.codegen.render.render_utils import write_generated_file
    from generator.codegen.utils import (
        get_connection_integrity_config,
        get_connections_by_protocol,
        get_profiles,
        infer_c_type,
        to_c_value,
    )
except ImportError:
    import sys

    sw_path = Path(__file__).parent.parent.parent.parent.parent / "sw"
    if str(sw_path) not in sys.path:
        sys.path.insert(0, str(sw_path))
    from generator.codegen.render.render_utils import write_generated_file
    from generator.codegen.utils import (
        get_connection_integrity_config,
        get_connections_by_protocol,
        get_profiles,
        infer_c_type,
        to_c_value,
    )

logger = logging.getLogger(__name__)
TEMPLATES_DIR = Path(__file__).parent / "templates"
PROTOCOL_NAME = "blackboard"

def _get_supported_platforms() -> set[str]:
    """Load supported platforms from config.py so CLI updates stay in sync."""
    config_path = Path(__file__).with_name("config.py")
    spec = importlib.util.spec_from_file_location(f"{PROTOCOL_NAME}_config", config_path)
    if spec is None or spec.loader is None:
        raise RuntimeError(f"Failed to load protocol config: {config_path}")

    config_module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(config_module)
    config = config_module.get_protocol_config()
    return set(config.get("supported_platforms", []))

def _get_jinja_env() -> Environment:
    return Environment(
        loader=FileSystemLoader(str(TEMPLATES_DIR)),
        autoescape=select_autoescape(),
        trim_blocks=True,
        lstrip_blocks=True,
    )


def _render_protocol_header(
    platform_name: str,
    model: Dict[str, Any],
    analysis: Dict[str, Any],
    environment: str,
    output_dir: Path,
) -> None:
    env = _get_jinja_env()
    template = env.get_template(f"{PROTOCOL_NAME}_{platform_name}.h.j2")
    profiles = get_profiles(model)
    protocol_channels = _extract_protocol_channels(model)
    output = template.render(
        model=model,
        analysis=analysis,
        environment=environment,
        protocol=PROTOCOL_NAME,
        profiles=profiles,  # All profiles available to template
        protocol_channels=protocol_channels,  # Channels with profile_data attached
    )

    include_dir = output_dir.parent / "include"
    include_dir.mkdir(parents=True, exist_ok=True)

    output_file = include_dir / f"{PROTOCOL_NAME}_{platform_name}.h"
    write_generated_file(output_file, output)


def _extract_protocol_channels(model: Dict[str, Any]) -> list[Dict[str, Any]]:
    """Extract protocol channels with profile data converted to C syntax and typed fields."""
    channels = []
    profiles = get_profiles(model)
    
    try:
        protocol_connections = get_connections_by_protocol(model, PROTOCOL_NAME)
    except Exception:
        protocol_connections = []

    for connection in protocol_connections:
        profile_name = connection.get("profile", "")
        profile_data = profiles.get(profile_name, {}) if isinstance(profiles.get(profile_name), dict) else {}
        integrity_type = get_connection_integrity_config(model, connection)

        # Convert profile values to C syntax and infer types
        c_values = {}
        c_fields = {}
        for key, value in profile_data.items():
            c_values[key] = to_c_value(value)
            c_fields[key] = infer_c_type(value)
        
        channels.append(
            {
                "name": connection.get("name", "unknown"),
                "interface": connection.get("interface", ""),
                "profile_name": profile_name,
                "profile_data": profile_data,  # Original profile data
                "c_values": c_values,  # C-formatted values
                "c_fields": c_fields,  # Inferred C types
                "integrity_type": integrity_type,
                "from_component": connection.get("from_component", ""),
                "to_component": connection.get("to_component", ""),
            }
        )
    return channels


def _render_platform_impl(
    platform_name: str,
    model: Dict[str, Any],
    analysis: Dict[str, Any],
    environment: str,
    output_dir: Path,
) -> None:
    _render_protocol_header(platform_name, model, analysis, environment, output_dir)

    env = _get_jinja_env()
    template_name = f"{PROTOCOL_NAME}_{platform_name}.c.j2"
    if not (TEMPLATES_DIR / template_name).exists():
        template_name = f"{PROTOCOL_NAME}_impl.c.j2"
    template = env.get_template(template_name)
    profiles = get_profiles(model)
    protocol_channels = _extract_protocol_channels(model)

    output = template.render(
        model=model,
        analysis=analysis,
        environment=environment,
        protocol=PROTOCOL_NAME,
        profiles=profiles,  # All profiles available to template
        protocol_channels=protocol_channels,  # Channels with profile_data attached
    )

    output_file = output_dir / f"{PROTOCOL_NAME}_{platform_name}.c"
    write_generated_file(output_file, output)


def _render_common_header(
    model: Dict[str, Any],
    analysis: Dict[str, Any],
    output_dir: Path,
) -> None:
    """Render the platform-independent common header file."""
    env = _get_jinja_env()
    template = env.get_template(f"{PROTOCOL_NAME}_common.h.j2")
    profiles = get_profiles(model)
    protocol_channels = _extract_protocol_channels(model)
    output = template.render(
        model=model,
        analysis=analysis,
        protocol=PROTOCOL_NAME,
        profiles=profiles,  # All profiles available to template
        protocol_channels=protocol_channels,  # Channels with profile_data attached
    )

    include_dir = output_dir.parent / "include"
    include_dir.mkdir(parents=True, exist_ok=True)

    output_file = include_dir / f"{PROTOCOL_NAME}_common.h"
    write_generated_file(output_file, output)


def _render_common_impl(
    model: Dict[str, Any],
    analysis: Dict[str, Any],
    output_dir: Path,
) -> None:
    """Render the platform-independent common implementation file."""
    env = _get_jinja_env()
    template = env.get_template(f"{PROTOCOL_NAME}_common.c.j2")
    profiles = get_profiles(model)
    protocol_channels = _extract_protocol_channels(model)
    output = template.render(
        model=model,
        analysis=analysis,
        protocol=PROTOCOL_NAME,
        profiles=profiles,  # All profiles available to template
        protocol_channels=protocol_channels,  # Channels with profile_data attached
    )

    output_file = output_dir / f"{PROTOCOL_NAME}_common.c"
    write_generated_file(output_file, output)



def _build_render_function(platform_name: str):
    def render_protocol(
        model: Dict[str, Any], analysis: Dict[str, Any], environment: str, output_dir: Path
    ) -> None:
        # Always render common files first (only once, harmless if called multiple times)
        _render_common_header(model, analysis, output_dir)
        _render_common_impl(model, analysis, output_dir)
        # Then render platform-specific files
        _render_platform_impl(platform_name, model, analysis, environment, output_dir)

    render_protocol.__name__ = f"render_{PROTOCOL_NAME}_{platform_name}"
    return render_protocol


def render_blackboard(
    model: Dict[str, Any], analysis: Dict[str, Any], environment: str, output_dir: Path
) -> None:
    """Main entry point: renders common protocol files."""
    _render_common_header(model, analysis, output_dir)
    _render_common_impl(model, analysis, output_dir)


def __getattr__(name: str):
    prefix = f"render_{PROTOCOL_NAME}_"
    if not name.startswith(prefix):
        raise AttributeError(name)

    platform_name = name[len(prefix):]
    if platform_name not in _get_supported_platforms():
        raise AttributeError(name)

    return _build_render_function(platform_name)
