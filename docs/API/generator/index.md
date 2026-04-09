# Generator API Reference

The **Generator** module consumes the Composer Unified HALO Model and produces:

- HALO initialization and runtime files
- Platform-specific code
- Protocol-specific code
- Analysis and deployment artifacts

This page explains how to use Generator from both CLI and Python APIs, and how to extend it with your own templates and plugins.

## End-to-End Flow

The Generator performs the following tasks:

1. Load `halo_unified_model.pkl` from Composer output.
2. Run model analysis and outputs `analysis_report.json`
3. Discover platform generators
4. Discover protocol generators
5. Generate HALO portable files
6. For each selected platform:
   - Generate HALO code
   - Generate platform code
   - Generate protocol code
7. Write deployment information per platform.


### Platform-scoped model filtering

Before rendering a target platform, Generator derives a filtered model containing only data relevant to that platform:

- `components`: only components where `component.platform == environment`
- `connections`: only connections touching those components
- `interfaces`: only interfaces referenced by those connections
- `profiles`: only profiles referenced by those connections
- `dataTypes.structs`: only structs referenced by selected interfaces
- Connection lists:
    - `send_connections`
    - `recv_connections`

This filtered model is what platform and protocol templates receive.

## CLI Usage

You can run Generator through the user CLI (`halo generate`)

```bash
halo generate \
    --input-dir ./composer_output \
    --output-dir ./output/gen
```

Useful flags:

- `--composer-output-dir`, `-i`: path containing unified model output from Composer
- `--input-dir`: alias for `--composer-output-dir`
- `--output-dir`, `-o`: destination for generated artifacts
- `--platform`, `-p`: generate only one platform; default all platforms are generated
- `--pkl`: explicit PKL path
- `--log-level`: `DEBUG|INFO|WARNING|ERROR`

Examples:

```bash
# Generate all platforms present in model
halo generate -i ./composer_output -o ./output/gen

# Generate only Linux and FreeRTOS
halo generate -i ./composer_output -o ./output/gen -p linux -p freertos

# Use explicit PKL file if there are multiple PKL files in the directory
halo generate -i ./composer_output -o ./output/gen --pkl ./composer_output/halo_unified_model.pkl
```

## Python Module Usage

### Public API (Composer-style)

```python
from generator.halo_generator import generate_from_model

exit_code = generate_from_model(
    composer_output_dir="./output",
    output_dir="./output/gen",
    platforms=["linux"],           # optional platform filter
    pkl_path=None,                 # optional explicit path
    log_level="INFO",            # optional
)

if exit_code != 0:
    raise RuntimeError("Generation failed")
```

Parameters:

- `composer_output_dir` (required): Composer output directory
- `output_dir` (optional): destination for generated artifacts
- `platforms` (optional): list of platforms; default is all platforms
- `pkl_path` (optional): explicit path to `halo_unified_model.pkl`
- `log_level` (optional): `DEBUG|INFO|WARNING|ERROR` or numeric logging level

## Output Layout

Typical output structure:

```text
output/
    analysis/
        analysis_report.json
    codegen/
        portable/
            include/
            src/
        <ComponentName>_<platform>/
            include/
            src/
    deployment/
        <platform>/
            deployment_manifest.json
```

Notes:

- Platform output directories use component+platform naming.
- Deployment manifests are placeholders for downstream packaging.

## Scaffolding New Generators

Generator scaffolding creates installable Python packages with entry points and starter templates.

### Interactive scaffold

```bash
halo create-generator
```

### Non-interactive platform scaffold

```bash
halo create-generator \
    --no-interactive \
    --generator-type platform \
    --name esp32 \
    --description "HALO generator for ESP32" \
    --author "Your Name" \
    --email "you@example.com" \
    --platform esp32 \
    --os True \
    --template-engine jinja2 \
    --output-dir ./generated_plugins
```

### Non-interactive protocol scaffold

```bash
halo create-generator \
    --no-interactive \
    --generator-type protocol \
    --name eventchannel \
    --description "EventChannel protocol generator" \
    --author "Your Name" \
    --email "you@example.com" \
    --protocol eventchannel \
    --supported-platforms linux,freertos,zephyr \
    --template-engine jinja2 \
    --output-dir ./generated_plugins
```

### Update scaffolded generator metadata

If you need to update generator after it is already created:

```bash
# List supported platforms in a protocol generator
halo update-generator --path ./generated_plugins/halo_proto_eventchannel --list-platforms

# Add/remove supported platforms
halo update-generator --path ./generated_plugins/halo_proto_eventchannel --add-platforms linux, rtos, my_platform
halo update-generator --path ./generated_plugins/halo_proto_eventchannel --remove-platforms linux, rtos, my_platform
```

After scaffolding:

1. Default render logic in `render.py` is provided, user can extend the logic if needed.
2. Customize templates under `src/<module>/templates`.
3. Install package in editable mode (`pip install -e .`).
4. Run generation and verify output.

## Jinja2 Template Context

HALO generator templates use Jinja2 syntax and rendering semantics. Reference:
[Jinja2 documentation](https://jinja.palletsprojects.com/).

### Context keys available to templates

Core keys passed by HALO rendering pipeline:

- `model`: platform-scoped unified model
- `analysis`: analyzer report dict
- `environment`: target platform name
- `type_map`: default HALO C type mapping

Scaffolded platform renderers also pass convenience keys:

- `components`
- `connections`
- `interfaces`
- `profiles`
- `has_os`

### Common Jinja filters/globals used in HALO templates

These helpers are injected by HALO so templates can safely transform values
from unified model data into C-friendly output.

Filters:

- `c_ident`: converts arbitrary text into a valid/safe C identifier.
- `crc_ret_type`: maps CRC kind names (for example `crc16`, `crc32`) to C return types.
- `size_bytes`: parses size-like values (`64MB`, `1kB`, `128B`) to byte counts.
- `c_uint`: formats numeric values into C unsigned literals.
- `c_str`: safely converts values into escaped C string literals.
- `str_lower`: converts any value to lowercase string.

Globals:

- `maybe_size_bytes`: like `size_bytes`, but returns no value when input is not clearly size-like.
- `looks_like_hex`: checks if a value is a hex literal string (for example `0x1000`).
- `looks_like_uint`: checks if a value looks like an unsigned integer literal.
- `infer_enum_domain`: infers enum-domain name for a literal when mapping is unambiguous.
- `_is_protocol_generator_available`: checks whether a protocol generator plugin is installed/available.

### Example template snippet using model context

{% raw %}
```jinja2
/* {{ environment }} connections */
{% for conn in model.get("connections", []) %}
/* {{ conn.name }}: {{ conn.from_component }} -> {{ conn.to_component }} */
{% endfor %}

{% for iname, iface in model.get("interfaces", {}).items() %}
/* interface {{ iname }} */
{% endfor %}
```
{% endraw %}

## Adding More Code Templates

Scaffolding gives a starter set. To add more generated files:

1. Add a new `.j2` template under your generator package `templates/` directory.
2. Update your `render.py` to render that template.
3. Write output to the correct generated folder (`include/` or `src/`).

Examples:

```python
_render_template(
        jenv,
        "diagnostics.c.j2",
        context,
        platform_src_dir / "diagnostics.c",
)

_render_template(
        jenv,
        "diagnostics.h.j2",
        context,
        platform_include_dir / "diagnostics.h",
)
```

Add a custom platform template that reads profiles

{% raw %}
```jinja2
/* profile overview */
{% for name, prof in model.get("profiles", {}).items() %}
/* {{ name }} => type={{ prof.get("type", "unknown") }} */
{% endfor %}
```
{% endraw %}

## User Code BEGIN/END Blocks (Code Preservation)

HALO supports preserving manual edits across regeneration when files include named user-code blocks:

```c
/* HALO USER CODE BEGIN: platform_init.includes */
// your custom include(s)
/* HALO USER CODE END: platform_init.includes */
```

How it works:

- On regeneration, HALO reads existing generated file.
- It finds named BEGIN/END blocks.
- For matching block names in new output, it keeps existing block body.
- This lets you regenerate safely without losing custom code in those blocks.

Important rules:

- Keep block names identical between template versions.
- Do not remove END markers.
- Use unique block names per file region.

## How Per-Platform Generation Is Selected

For each selected platform:

1. HALO selects the platform generator by generator name.
2. HALO invokes `render_<platform>(...)` from that generator module.
3. HALO invokes protocol generators only for protocol profile types used in that platform's connections.

If a platform generator is missing, HALO logs a warning and skips that platform.


## Troubleshooting

- Error: unified model pickle not found
    - Ensure Composer already ran and output directory is correct.
- Requested platform not found in unified model
    - Check platform names in model (`components[].platform`) and CLI spelling.
- No generators discovered
    - Ensure generator packages are installed in active Python environment.
- Platform generator missing for discovered platform
    - Install package providing that platform entry point.

