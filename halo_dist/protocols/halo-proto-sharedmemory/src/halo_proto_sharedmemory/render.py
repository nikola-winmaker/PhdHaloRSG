"""Sharedmemory protocol renderer.

Provides platform-specific rendering for the sharedmemory protocol.
"""
from __future__ import annotations

import logging
from pathlib import Path
from typing import Any, Dict

from jinja2 import Environment, FileSystemLoader, select_autoescape

try:
    from generator.codegen.render.render_utils import write_generated_file
    from generator.codegen.utils import (
        get_connections_by_protocol,
        get_connection_integrity_config,
        get_profile_for_connection,
        get_interface_for_connection,
        get_max_payload_for_connection,
        parse_size_string,
    )
except ImportError:
    import sys

    sw_path = Path(__file__).parent.parent.parent.parent.parent / "sw"
    if str(sw_path) not in sys.path:
        sys.path.insert(0, str(sw_path))
    from generator.codegen.render.render_utils import write_generated_file
    from generator.codegen.utils import (
        get_connections_by_protocol,
        get_connection_integrity_config,
        get_profile_for_connection,
        get_interface_for_connection,
        get_max_payload_for_connection,
        parse_size_string,
    )


logger = logging.getLogger(__name__)
TEMPLATES_DIR = Path(__file__).parent / "templates"
PROTOCOL_NAME = "sharedmemory"
SUPPORTED_PLATFORMS = {
    "baremetal",
    "freertos",
    "linux",
    "stm32",
    "zephyr",
}


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
    protocol_channels = _extract_protocol_channels(model)
    output = template.render(model=model, analysis=analysis, environment=environment, protocol=PROTOCOL_NAME, protocol_channels=protocol_channels)

    include_dir = output_dir.parent / "include"
    include_dir.mkdir(parents=True, exist_ok=True)

    output_file = include_dir / f"{PROTOCOL_NAME}_{platform_name}.h"
    write_generated_file(output_file, output)
    #logger.info("Generated: %s", output_file)


def _render_shared_crc_header(output_dir: Path) -> None:
    """Generate the shared CRC header file used by all platforms."""
    env = _get_jinja_env()
    try:
        template = env.get_template("sharedmemory_crc.h.j2")
        output = template.render()

        include_dir = output_dir.parent / "include"
        include_dir.mkdir(parents=True, exist_ok=True)

        output_file = include_dir / "sharedmemory_crc.h"
        write_generated_file(output_file, output)
        #logger.info("Generated: %s", output_file)
    except Exception as e:
        logger.warning("Could not generate shared CRC header: %s", e)


def _extract_protocol_channels(model: Dict[str, Any]) -> list[Dict[str, Any]]:
    channels = []
    try:
        protocol_connections = get_connections_by_protocol(model, PROTOCOL_NAME)
    except Exception:
        protocol_connections = []

    for connection in protocol_connections:
        profile_name = connection.get("profile", "")
        interface_name = connection.get("interface", "")
        
        # Get integrity type from core helper - abstracts away profile/interface details
        integrity_type = get_connection_integrity_config(model, connection)

        # Get profile for other configurations
        profile = get_profile_for_connection(model, connection)
        if not profile:
            profile = {}

        # Extract sync type from profile
        sync_type = "SHAREDMEMORY_SYNC_NONE"
        if "syncType" in profile:
            sync = profile["syncType"].lower()
            if sync == "acquirerelease":
                sync_type = "SHAREDMEMORY_SYNC_ACQ_REL"
            elif sync == "fullence":
                sync_type = "SHAREDMEMORY_SYNC_FULL"

        # Extract cache settings from profile
        cacheable = profile.get("cacheable", False)
        
        # Extract memory size and base address
        mem_size = profile.get("memSize", "4KB")
        base_addr = profile.get("baseAddress", "0x80000000")
        
        # Parse memory size to bytes
        if isinstance(mem_size, str):
            size_bytes = parse_size_string(mem_size)
            if size_bytes == 0:
                size_bytes = 4096  # Default 4KB if parsing failed
        else:
            size_bytes = mem_size

        # Get max_payload from what this connection actually sends
        max_payload = get_max_payload_for_connection(model, connection)
        
        # Verify buffer is large enough for at least one message
        # sizeof(sharedmemory_ringbuf_ctrl_t) = ~16 bytes (4 x u32)
        # sizeof(sharedmemory_header_t) = ~24 bytes (3x u32 + u16 + 2x u8)
        min_buffer_size = 16 + 24 + max_payload
        if size_bytes < min_buffer_size:
            logger.warning(
                "Buffer size for %s (%d bytes) is too small for payload (%d bytes). "
                "Minimum required: %d bytes. Using minimum size.",
                connection.get("name", "unknown"),
                size_bytes,
                max_payload,
                min_buffer_size
            )
            size_bytes = min_buffer_size
        
        channels.append(
            {
                "name": connection.get("name", "unknown"),
                "interface": interface_name,
                "profile_name": profile_name,
                "from_component": connection.get("from_component", ""),
                "to_component": connection.get("to_component", ""),
                "sync": sync_type,
                "integrity_type": integrity_type,
                "cacheable": str(cacheable).lower(),
                "base_address": base_addr,
                "size": size_bytes,
                "max_payload": max_payload,
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
    #logger.info("Rendering %s protocol for %s", PROTOCOL_NAME, platform_name)
    
    # Generate shared CRC header (idempotent - safe to call multiple times)
    _render_shared_crc_header(output_dir)
    
    _render_protocol_header(platform_name, model, analysis, environment, output_dir)

    env = _get_jinja_env()
    template_name = f"{PROTOCOL_NAME}_{platform_name}.c.j2"
    if not (TEMPLATES_DIR / template_name).exists():
        template_name = f"{PROTOCOL_NAME}_impl.c.j2"
    template = env.get_template(template_name)
    protocol_channels = _extract_protocol_channels(model)

    output = template.render(
        model=model,
        analysis=analysis,
        environment=environment,
        protocol=PROTOCOL_NAME,
        protocol_channels=protocol_channels,
    )

    output_file = output_dir / f"{PROTOCOL_NAME}_{platform_name}.c"
    write_generated_file(output_file, output)
    #logger.info("Generated: %s", output_file)


def _build_render_function(platform_name: str):
    def render_protocol(
        model: Dict[str, Any], analysis: Dict[str, Any], environment: str, output_dir: Path
    ) -> None:
        _render_platform_impl(platform_name, model, analysis, environment, output_dir)

    render_protocol.__name__ = f"render_{PROTOCOL_NAME}_{platform_name}"
    return render_protocol


def __getattr__(name: str):
    prefix = f"render_{PROTOCOL_NAME}_"
    if not name.startswith(prefix):
        raise AttributeError(name)

    platform_name = name[len(prefix):]
    if platform_name not in SUPPORTED_PLATFORMS:
        raise AttributeError(name)

    return _build_render_function(platform_name)
