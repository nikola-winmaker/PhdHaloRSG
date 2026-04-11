#!/usr/bin/env python3
import argparse
import json
import re
import shlex
from pathlib import Path


ROOT_DIR = Path(__file__).resolve().parents[2]
LAYOUT_PATH = ROOT_DIR / "config" / "amp_layout.json"


def load_layout():
    return json.loads(LAYOUT_PATH.read_text())


def parse_int(value):
    if isinstance(value, int):
        return value
    return int(value, 0)


def hex64_cells(value):
    return f"0x{value >> 32:08x} 0x{value & 0xffffffff:08x}"


def memory(layout):
    return layout["memory"]


def emit_shell_vars(layout):
    mem = memory(layout)
    pairs = {
        "AMP_BOOTARGS": layout["bootargs"],
        "AMP_HART0_COORD_ADDR": mem["hart0_coord_addr"],
        "AMP_HART0_COORD_SIZE": mem["hart0_coord_size"],
        "AMP_HART0_SLOT_ADDR": mem["hart0_slot_addr"],
        "AMP_HART0_SLOT_SIZE": mem["hart0_slot_size"],
        "AMP_HART1_ADDR": mem["hart1_addr"],
        "AMP_HART2_ADDR": mem["hart2_addr"],
        "AMP_HART3_ADDR": mem["hart3_addr"],
        "AMP_SMALL_APP_MAX_SIZE": mem["small_app_max_size"],
        "AMP_LINUX_ADDR": mem["linux_addr"],
        "AMP_LINUX_MAX_SIZE": mem["linux_max_size"],
        "AMP_OPENSBI_BRIDGE_ADDR": mem["opensbi_bridge_addr"],
        "AMP_OPENSBI_BRIDGE_MAX_SIZE": mem["opensbi_bridge_max_size"],
        "AMP_LINUX_INITRD_ADDR": mem["linux_initrd_addr"],
        "AMP_LINUX_DTB_ADDR": mem["linux_dtb_addr"],
        "AMP_SHARED_ADDR": mem["amp_shared_addr"],
        "AMP_SHARED_SIZE": mem["amp_shared_size"],
    }
    for key, value in pairs.items():
        print(f"{key}={shlex.quote(str(value))}")


def emit_c_header(layout):
    mem = memory(layout)
    lines = [
        "/* Auto-generated from config/amp_layout.json. */",
        "#ifndef AMP_LAYOUT_H",
        "#define AMP_LAYOUT_H",
        "",
        f'#define HART0_COORD_ADDR {mem["hart0_coord_addr"]}UL',
        f'#define HART0_COORD_SIZE {mem["hart0_coord_size"]}UL',
        f'#define HART0_ADDR {mem["hart0_slot_addr"]}',
        f'#define HART0_SLOT_SIZE {mem["hart0_slot_size"]}UL',
        f'#define HART1_ADDR {mem["hart1_addr"]}',
        f'#define HART2_ADDR {mem["hart2_addr"]}',
        f'#define HART3_ADDR {mem["hart3_addr"]}',
        f'#define SMALL_APP_MAX_SIZE {mem["small_app_max_size"]}U',
        f'#define LINUX_ADDR {mem["linux_addr"]}UL',
        f'#define LINUX_MAX_SIZE {mem["linux_max_size"]}UL',
        f'#define OPENSBI_BRIDGE_ADDR {mem["opensbi_bridge_addr"]}UL',
        f'#define OPENSBI_BRIDGE_MAX_SIZE {mem["opensbi_bridge_max_size"]}U',
        f'#define LINUX_INITRD_ADDR {mem["linux_initrd_addr"]}UL',
        f'#define LINUX_DTB_ADDR {mem["linux_dtb_addr"]}UL',
        f'#define AMP_SHARED_ADDR {mem["amp_shared_addr"]}UL',
        f'#define AMP_SHARED_SIZE {mem["amp_shared_size"]}UL',
        "",
        "#endif",
    ]
    print("\n".join(lines))


def emit_freertos_linker(layout):
    mem = memory(layout)
    origin = mem["hart2_addr"]
    length = mem["small_app_max_size"]
    print(
        f"""MEMORY {{
  APP2_RAM (rwx) : ORIGIN = {origin}, LENGTH = {length}
}}

ENTRY(_start)

SECTIONS {{
  . = ORIGIN(APP2_RAM);

  .text.init : {{
    *(.text.init)
  }} > APP2_RAM AT > APP2_RAM

  .text : {{
    *(.text)
    *(.text.*)
  }} > APP2_RAM AT > APP2_RAM

  .rodata : {{
    *(.rodata)
    *(.rodata.*)
  }} > APP2_RAM AT > APP2_RAM

  .data : {{
    __data_start = .;
    *(.data)
    *(.data.*)
    __data_end = .;
  }} > APP2_RAM AT > APP2_RAM

  .bss : {{
    __bss_start = .;
    *(.bss)
    *(.bss.*)
    *(COMMON)
    __bss_end = .;
  }} > APP2_RAM AT > APP2_RAM

  .stack : {{
    . = ALIGN(16);
    __stack_bottom = .;
    . += 0x4000;
    __stack_top = .;
  }} > APP2_RAM

  .heap : {{
    . = ALIGN(16);
    __heap_start = .;
    __heap_end = .;
  }} > APP2_RAM
}}"""
    )


def emit_bare_linker(layout):
    mem = memory(layout)
    origin = mem["hart3_addr"]
    length = mem["small_app_max_size"]
    print(
        f"""MEMORY {{
  APP3_RAM (rwx) : ORIGIN = {origin}, LENGTH = {length}
}}

ENTRY(_start)

SECTIONS {{
  .text.init : {{ *(.text.init) }} > APP3_RAM AT > APP3_RAM
  .text : {{ *(.text) *(.text.*) }} > APP3_RAM AT > APP3_RAM
  .rodata : {{ *(.rodata) *(.rodata.*) }} > APP3_RAM AT > APP3_RAM
  .data : {{ *(.data) *(.data.*) }} > APP3_RAM AT > APP3_RAM
  .bss : {{ *(.bss) *(.bss.*) *(COMMON) }} > APP3_RAM AT > APP3_RAM
  .stack : {{ . = ALIGN(16); }} > APP3_RAM
}}"""
    )


def emit_zephyr_overlay(layout, target):
    mem = memory(layout)
    addr = parse_int(mem["hart1_addr"])
    size = parse_int(mem["small_app_max_size"])
    if target == "qemu":
        print(
            f"""/ {{
\tchosen {{
\t\tzephyr,sram = &hart1_app_ram;
\t}};

\thart1_app_ram: memory@{addr:08x} {{
\t\tdevice_type = "memory";
\t\treg = <0x{addr:08x} 0x{size:08x}>;
\t}};
}};"""
        )
        return

    if target == "hw":
        print(
            f"""/ {{
\tchosen {{
\t\tzephyr,sram = &hart1_app_ram;
\t}};

\thart1_app_ram: memory@{addr:08x} {{
\t\tdevice_type = "memory";
\t\treg = <0x0 0x{addr:08x} 0x0 0x{size:08x}>;
\t\treg-names = "mem";
\t}};
}};

&clint {{
\tinterrupts-extended = <&cpu1_intc 3 &cpu1_intc 7>;
}};

&plic {{
\tinterrupts-extended = <&cpu1_intc 11>;
}};"""
        )
        return

    raise SystemExit(f"unsupported zephyr overlay target: {target}")


def render_reserved_memory_block(layout):
    lines = [
        "\treserved-memory {",
        "\t\t#address-cells = <0x02>;",
        "\t\t#size-cells = <0x02>;",
        "\t\tranges;",
        "",
    ]
    for entry in layout["reserved_memory"]:
        addr = parse_int(entry["addr"])
        size = parse_int(entry["size"])
        lines.extend(
            [
                f'\t\t{entry["name"]}@{addr:x} {{',
                f"\t\t\treg = <{hex64_cells(addr)} {hex64_cells(size)}>;",
                "\t\t\tno-map;",
                "\t\t};",
                "",
            ]
        )
    lines.extend(["\t};", ""])
    return "\n".join(lines)


def render_overlay(layout, initrd_start, initrd_end):
    lines = [
        "/ {",
        "\tchosen {",
        f'\t\tbootargs = "{layout["bootargs"]}";',
        f"\t\tlinux,initrd-start = <{hex64_cells(initrd_start)}>;",
        f"\t\tlinux,initrd-end = <{hex64_cells(initrd_end)}>;",
        "\t};",
        "",
    ]
    lines.append(render_reserved_memory_block(layout).rstrip())
    lines.extend(["};", ""])
    return "\n".join(lines)


def patch_dts(layout, input_path, output_path, initrd_start, initrd_size, overlay_out=None):
    initrd_end = initrd_start + initrd_size
    text = Path(input_path).read_text()

    chosen_match = re.search(r'(^|\n)(\s*)chosen\s*\{', text)
    chosen_body = (
        f'\n\t\tbootargs = "{layout["bootargs"]}";\n'
        f"\t\tlinux,initrd-start = <{hex64_cells(initrd_start)}>;\n"
        f"\t\tlinux,initrd-end = <{hex64_cells(initrd_end)}>;\n"
    )
    if chosen_match:
        insert_at = chosen_match.end()
        text = text[:insert_at] + chosen_body + text[insert_at:]
    else:
        model_line = 'model = "riscv-virtio,qemu";\n'
        model_idx = text.find(model_line)
        if model_idx == -1:
            raise SystemExit("missing model line in DTS")
        insert_at = model_idx + len(model_line)
        chosen_node = "\n\tchosen {" + chosen_body + "\t};\n"
        text = text[:insert_at] + chosen_node + text[insert_at:]

    memory_node = "\tmemory@80000000 {"
    mem_idx = text.find(memory_node)
    if mem_idx == -1:
        raise SystemExit("missing memory node in DTS")
    text = text[:mem_idx] + render_reserved_memory_block(layout) + text[mem_idx:]
    Path(output_path).write_text(text)

    if overlay_out:
        Path(overlay_out).write_text(
            "/dts-v1/;\n\n" + render_overlay(layout, initrd_start, initrd_end)
        )


def main():
    parser = argparse.ArgumentParser()
    sub = parser.add_subparsers(dest="cmd", required=True)

    sub.add_parser("shell-vars")
    sub.add_parser("c-header")
    sub.add_parser("freertos-linker")
    sub.add_parser("bare-linker")

    zephyr_overlay = sub.add_parser("zephyr-overlay")
    zephyr_overlay.add_argument("--target", choices=["qemu", "hw"], required=True)

    patch = sub.add_parser("patch-dts")
    patch.add_argument("--input", required=True)
    patch.add_argument("--output", required=True)
    patch.add_argument("--initrd-start", required=True)
    patch.add_argument("--initrd-size", required=True)
    patch.add_argument("--overlay-out")

    args = parser.parse_args()
    layout = load_layout()

    if args.cmd == "shell-vars":
        emit_shell_vars(layout)
    elif args.cmd == "c-header":
        emit_c_header(layout)
    elif args.cmd == "freertos-linker":
        emit_freertos_linker(layout)
    elif args.cmd == "bare-linker":
        emit_bare_linker(layout)
    elif args.cmd == "zephyr-overlay":
        emit_zephyr_overlay(layout, args.target)
    elif args.cmd == "patch-dts":
        patch_dts(
            layout,
            args.input,
            args.output,
            parse_int(args.initrd_start),
            parse_int(args.initrd_size),
            args.overlay_out,
        )


if __name__ == "__main__":
    main()
