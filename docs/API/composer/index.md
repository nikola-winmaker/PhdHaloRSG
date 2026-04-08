# Composer API Reference

The **Composer** module is responsible for constructing the **Unified HALO Model** from architecture descriptions (HADL or UML XMI) and organizing it into a structured, machine-readable format for downstream code generation.

## Core Components

### Python APIs

```python
from composer.halo_composer import compose_from_hadl, compose_from_xmi

# Compose from HADL files
exit_code = compose_from_hadl(
    hadls_root='./architecture',
    output_dir='./output',
    include_stdlib_profiles=True,
    log_level='INFO',
)

# Or compose from UML XMI
exit_code = compose_from_xmi(
    xmi_path='./model.xmi',
    output_dir='./output',
    include_stdlib_profiles=True,
    log_level='INFO',
)
```

**Parameters:**

- `hadls-root`: Root directory containing HADL description files
- `from-xmi`: Path to UML XMI model file (alternative to HADL)
- `output-dir`: Output directory for generated unified model and artifacts
- `include-stdlib-profiles`: Include built-in profile templates (SharedMemory, Blackboard, EventChannel)
- `user-types`: Optional path to a JSON file defining custom model-domain values that extend HALO built-in enums (`componentType`, `componentPlatform`, `componentFunction`)
- `log-level`: Logging verbosity (DEBUG, INFO, WARNING, ERROR)

**Outputs:**

- `halo_unified_model.pkl` - Pickled unified model (binary format for code generation)
- `halo_unified_model.json` - JSON representation of the model
- `halo_unified_model.dot` - Graphviz DOT visualization
- `halo_unified_model.svg` - Rendered architecture diagram (if Graphviz installed)
- `halo_model.uml` - UML representation
- `halo_profile.xmi` - Profile stereotypes

---


### HADL Processing

Exports HALO unified model back to HADL format for model traversal and validation.

```python
from composer.hadl_exporter import HADLExporter

exporter = HADLExporter(unified_model)
hadl_text = exporter.to_hadl()
with open("./output/model.hadl", "w") as f:
    f.write(hadl_text)
```

---

### Utilities (`composer/tools/`)

#### `unified_json_to_dot.py`
Converts JSON unified model to Graphviz DOT format for visualization.

```python
from composer.tools.unified_json_to_dot import json_to_dot

dot_text = json_to_dot(unified_model_dict)
with open("./output/model.dot", "w") as f:
    f.write(dot_text)
```

#### `compare_pickles.py`
Compares two pickled unified models for differences (useful for regression testing).

```python
from composer.tools.compare_pickles import compare_models

differences = compare_models("./old_model.pkl", "./new_model.pkl")
print(differences)
```

---

## Unified Model Structure

The unified HALO model is a dictionary with the following top-level structure:

```python
{
    "schemaVersion": 1,
    "components": [
        {
            "name": "Core1",
            "platform": "linux",
            "type": "process",
            # ... component properties
        },
        # ... more components
    ],
    "connections": [
        {
            "from_component": "Core1",
            "to_component": "Core2",
            "interface": "TstIf_1",
            "profile": "SharedMemory_Profile1",
            # ... connection properties
        },
        # ... more connections
    ],
    "interfaces": {
        "TstIf_1": {
            "name": "TstIf_1",
            "data": {
                # ... interface data definition
            },
            # ... interface properties
        },
        # ... more interfaces
    },
    "profiles": {
        "SharedMemory_Profile1": {
            "name": "SharedMemory_Profile1",
            "type": "shmem",
            # ... profile configuration
        },
        # ... more profiles
    },
    "dataTypes": {
        "structs": {
            # ... struct definitions
        },
        # ... other type definitions
    },
    "valueDomains": {
        "schemaVersion": 1,
        "enums": {
            # ... enumeration definitions
        },
        "componentKeyToEnum": {
            # ... mappings
        },
        "profileKeyToEnum": {
            # ... mappings
        }
    }
}
```

---

## CLI Usage

From the user-facing HALO CLI, composer behavior is exposed through `halo compose`.

**Parameters:**

- `--hadls-root`: Root directory containing HADL description files
- `--from-xmi`: Path to UML XMI model file (alternative to HADL)
- `--output-dir`: Output directory for generated unified model and artifacts
- `--include-stdlib-profiles`: Include built-in profile templates (SharedMemory, Blackboard, EventChannel)
- `--user-types`: Optional path to a JSON file defining custom model-domain values that extend HALO built-in enums (`componentType`, `componentPlatform`, `componentFunction`)
- `--log-level`: Logging verbosity (DEBUG, INFO, WARNING, ERROR)


### HALO Command Mapping

| User command |
|------|
| `halo compose --hadls-root DIR --output-dir DIR [--log-level LEVEL] [--include-stdlib-profiles] [--user-types JSON FILE]` 
| `halo compose --from-xmi FILE --output-dir DIR [--log-level LEVEL] [--include-stdlib-profiles] [--user-types JSON FILE]` 

| Composer module call |
|------|
| `compose_from_hadl(hadls_root=DIR, output_dir=DIR, ...)` |
| `compose_from_xmi(xmi_path=FILE, output_dir=DIR, ...)` |


## Value Domains (Custom Types)

User types --user-types are for extending the model with custom domain values.

Expected keys in `user_types.json`:

- `componentType`
- `componentPlatform`
- `componentFunction`

Example `user_types.json`:

```bash
{
    "componentType": ["MyType", "MyType2"],
    "componentPlatform": ["MyPlatform", "MyPlatform2"],
    "componentFunction": ["MyFunction", "MyFunction2"]
}
```

Load when composing:

```bash
halo compose \
  --hadls-root ./architecture \
  --output-dir ./output \
  --user-types ./custom_types.json
```

These values are merged with HALO defaults and validated as one effective enum set.

### Defaults Available Without `--user-types`

The following defaults are available from Composer out of the box.

#### 1) Component Domains (used in ADL component metadata)

| Domain key | Default values |
|---|---|
| `componentType` | `Application`, `Acceleration` |
| `componentPlatform` | `Linux`, `FreeRTOS`, `Zephyr`, `BareMetal`, `FPGA` |
| `componentFunction` | `RealTimeProcessing`, `GeneralProcessing`, `OffloadProcessing` |

#### IDL Built-in Scalar/Data Types

These are the built-in field types accepted by the IDL grammar.

| IDL type | Notes |
|---|---|
| `byte` | 8-bit byte-like scalar |
| `integer` | Integer scalar |
| `int` | Integer scalar alias |
| `unsigned int` | Unsigned integer scalar |

Notes:

- `read`/`write` sections reference named data structures.


#### Available Protocol/Profile Kinds 

| Kind | Source | Notes |
|---|---|---|
| `SharedMemory` | Default HADL profile files | Available in shipped HADL examples
| `EventChannel` | Default HADL profile files | Available in shipped HADL examples/templates |
| `BlackBoard` | Default HADL profile files | Available in shipped HADL examples/templates |
| `<CustomKind>` | HML grammar | Custom profile kinds are supported by grammar |

#### Built-in HML/Profile Value Domains

These enums are validated/used by Composer value-domain logic by default.

| Domain key | Built-in values |
|---|---|
| `memPolicyEnumeration` | `WriteBack`, `WriteThrough`, `NonCacheable`, `None` |
| `memCoherence` | `Software`, `Hardware`, `None` |
| `SynchronizationTypes` | `AcquireRelease`, `FullFence`, `None` |
| `memPermissions` | `R`, `W`, `RW` |
| `PriorityTypes` | `Low`, `Medium`, `High`, `Critical` |
| `SynchronizationPrimitive` | `mutex`, `semaphore`, `None` |
| `SynchronizationBlocking` | `True`, `False` |
| `StreamTypes` | `single`, `burst`, `continuous` |
| `TransferEndpoint` | `deviceToMemory`, `memoryToDevice`, `memoryToMemory` |
| `errorType` | `Interrupt`, `Callback`, `None` |
| `profileType` | `SharedMemory` |

#### Standard Library Profile Templates

If `--include-stdlib-profiles` is enabled for HADL composition, Composer injects packaged default profile templates/instances from the stdlib profile file.

This includes default template attributes for:

- `SharedMemory`
- `EventChannel`
- `BlackBoard`

---

## Integration with Code Generation

The unified model (pickled as `.pkl`) is consumed by the **Generator** module to produce platform-specific code.

See [Generator API](../generator/index.md) for downstream usage.
