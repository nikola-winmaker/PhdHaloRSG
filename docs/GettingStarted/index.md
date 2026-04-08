# Getting Started

Start here if you want to install HALO, compose a model, generate code, and extend the framework with your own generators.

## Install HALO

To install HALO directly from the wheel hosted in GitHub:

```bash
python -m pip install "https://github.com/nikola-winmaker/PhdHaloRSG/blob/main/halo/halo-0.1.1-py3-none-any.whl?raw=1"
```

After installation, verify the CLI:

```bash
halo --help or python -m halo --help
```

## Basic workflow

The typical HALO workflow has two main steps:

1. `halo compose` reads HALO DSL files, or imports a UML/XMI model, and creates the unified model.
2. `halo generate` uses that unified model plus installed platform and protocol generators to emit code.

A common first command looks like this:

```bash
halo compose --hadls-root ./hadls --output-dir ./halo_out --include-stdlib-profiles
halo generate --composer-output-dir ./halo_out --output-dir ./generated
```

## Example

Let's look into one example of heterogenious environment:

In system.adl we can define components. The ADL grammar requires:

- **Component name**: unique identifier
- **Type**: required attribute (Application, Acceleration, etc.)
- **Platform**: required attribute (Linux, FreeRTOS, Zephyr, BareMetal, etc.)
- **Function**: required attribute (GeneralProcessing, RealTimeProcessing, OffloadProcessing, etc.)

A generic ADL file follows this structure:

```adl
HALOFramework {
    components {
        <ComponentName> : Component { Type: <Type>, Platform: <Platform>, Function: <Function> }
        // ... more components
    }
    
    connections {
        connection <ConnectionName> {
            from <SourceComponent> to <DestinationComponent>
            interface: <InterfaceName>
            profile: <ProfileName>
        }
        // ... more connections
    }
}
```

Here's a concrete example:

```adl
HALOFramework {
    components {
        Core1 : Component { Type: Application,  Platform: Linux,     Function: GeneralProcessing }
        Core2 : Component { Type: Application,  Platform: FreeRTOS,  Function: RealTimeProcessing }
        Core3 : Component { Type: Application,  Platform: Zephyr,    Function: RealTimeProcessing }
        Core4 : Component { Type: Application,  Platform: BareMetal, Function: RealTimeProcessing }
    }
}
```

Connections bind components together. The ADL grammar requires:

- **connection name**: unique identifier
- **from** and **to**: source and destination component names (cross-references)
- **interface**: interface name that governs the data contract
- **profile**: profile name that specifies the hardware mapping strategy

Here's a concrete example:

```adl
connections {
    connection LinuxToRTOS {
        from Core1 to Core2
        interface: TstIf_1
        profile: sharedMemoryProfile1
    }
    
    connection RTOSToLinux {
        from Core2 to Core1
        interface: TstIf_2
        profile: sharedMemoryProfile1
    }
}
```

One of the simplest paths in that sample is:

- connection `LinuxToRTOS`
- interface `TstIf_1`
- profile `sharedMemoryProfile1`

Interfaces define the data contract. The IDL grammar requires:

- **interface name**: unique identifier  
- **access block**: specifies read and write access to data types
  - **read**: list of type names that can be read (or `*` for all)
  - **write**: list of type names that can be written (or `*` for all)
- **integrity block** (optional): specifies integrity checks
  - **crc16**, **crc32**, etc.: list of types protected by the corresponding check

```idl
interface TstIf_1 {
    access {
        read {
            SharedData
            SharedData2 
            SharedData3
        } 
        write {
            SharedData 
            SharedData2
            SharedData3
        }
    }

    integrity {
        crc16 {
            SharedData2
        }

        crc32 {
            SharedData
        }
    }
}
```

Data types are defined in IDDL (Interface Data Definition Language) files. IDDL provides reusable data structure definitions that are referenced by interfaces. The IDDL grammar supports:

- **dataStructures** block: contains all data type definitions
- **StructDef**: named structure with typed fields
  - **Field types**: `byte`, `int`, `integer`, `unsigned int`
  - **Arrays**: `fieldName[size]` with optional initializers
  - **Initializers**: `fieldName = value`

A generic IDDL file follows this structure:

```iddl
dataStructures {
    <StructName> {
        <Type> <FieldName>
        <Type> <FieldName>[<Size>] = <InitValue>
        // ... more fields
    }
    // ... more structures
}
```

Here's a concrete example:

```iddl
dataStructures {
    SharedData {
        integer cnt
        integer dataId
        byte dataPayload[1024] = 0
    }
    SharedData2 { 
        integer dataPayload[1024] 
    }
    SharedData3 { 
        integer someConst = 5 
    }
    FastData {
        integer messageId
        byte payload[512]
        unsigned int timestamp
    }
    EventData {
        byte Ready
        byte Start
        byte Done
    }
}
```

These data structures are included in IDL files via `include` statements and referenced in interface access and integrity blocks (like `SharedData` in the `TstIf_1` interface above).

 The HML grammar supports:

- **SharedMemory**: shared-memory based profile with attributes like `memSize`, `baseAddress`, etc.
- **CustomProfile**: user-defined profiles

Each profile instance has a **kind** (type) and optional attributes:

```hml
Profiles {
    SharedMemory sharedMemoryProfile1 {
        memSize: 64MB
        baseAddress: 0x80000000
    }
}
```

Profiles are defined in HMML (Hardware Mapping Markup Language) template files. HMML defines reusable profile templates with common attributes for each profile type. These templates are then instantiated in HML files (e.g., mapping the `sharedMemoryProfile1` instance to use attributes from the `SharedMemory` template).

The HMML grammar supports several profile kinds:

- **SharedMemory**: with attributes like `memSize`, `baseAddress`, `cacheable`, `policy`, `coherence`, `syncType`, `permissions`, `priority`, etc.

A generic HMML file follows this structure:

```hmml
Profiles {
    <ProfileType> {
        <AttributeName>: <Value>
        <AttributeName>: <Value>
        // ... more attributes
    }
    // ... more profile templates
}
```

Here's a concrete example showing the SharedMemory profile template that backs `sharedMemoryProfile1`:

```hmml
Profiles {
    SharedMemory {
        memSize: 64MB
        baseAddress: 0x80000000
        cacheable: True
        policy: WriteBack
        cacheLine: 64B
        coherence: Software
        syncType: AcquireRelease
        permissions: RW
        priority: High
        syncBlocking: True
        syncPrimitive: mutex
    }
}
```

Built-in profiles (via `--include-stdlib-profiles`) come preconfigured and ready to use. HALO provides three standard protocol generators with the following platform support:


| **SharedMemory** - Direct shared-memory access with configurable caching, coherence, and synchronization 
| **Blackboard**   - Sampled-data port pattern: one writer updates the latest value, readers always see the newest snapshot 
| **EventChannel** - Event-driven pattern: components communicate by sending/receiving timestamped events 

Select the appropriate profile based on your communication semantics needs and target platform availability.

The interface declares what data is accessible (read/write) and what integrity checks apply.

You can run the example like this:

```bash
halo compose --hadls-root _path_to_you_hadls_descriptions_ --output-dir ./output --include-stdlib-profiles
halo generate --composer-output-dir ./output --output-dir ./generated
```

## What HALO generates

After composition, HALO writes the unified model and analysis artifacts into the composer output directory.

After generation, HALO emits platform-scoped packages. A typical layout in this repo looks like:

```text
output/gen/codegen/
├── Core1_linux/
│   ├── include/
│   └── src/
├── Core2_freertos/
├── Core3_zephyr/
├── Core4_baremetal/
└── portable/
```

- generated HALO API headers such as `halo_api.h`, `halo_cfg.h`, `halo_interfaces.h`, and `halo_structs.h`,
- generated HALO sources such as `halo_api.c`, `halo_channels.c`, `halo_bindings.c`, and `halo_services.c`,
- platform-specific integration files such as `Linux_init.h` and `Linux_init.c`,
- protocol-specific files such as `sharedmemory_linux.h`, `sharedmemory_linux.c`, and `sharedmemory_crc.h`.

The `portable/` package contains the common portable support layer, such as `halo_types.h` and `halo_lib.h`.

In practice, users add the generated portable and platforms `include/` and `src/` trees into their application build and call the generated init plus typed send/receive APIs from application code.

## Protocol example

The repository includes a shared-memory protocol generator built in HALO Framework.

That generator supports multiple platforms and is responsible for emitting files such as:

- `sharedmemory_linux.c`
- `sharedmemory_linux.h`
- `sharedmemory_crc.h`

In the sample model, the `LinuxToRTOS` connection uses `sharedMemoryProfile1`, so the shared-memory protocol generator participates in code generation for the affected targets.

## Create a new platform generator

HALO can scaffold a new platform generator for you.

Interactive mode:

```bash
halo create-generator
```

Non-interactive example:

```bash
halo create-generator --generator-type platform --no-interactive \
  --name my_platform \
  --description "My Platform Generator" \
  --author "Your Name" \
  --email "you@example.com" \
  --platform my_platform \
  --output-dir ./generators
```

Then install the generated package with pip:

```bash
cd ./generators/halo-generator-my_platform
pip install -e .
```

Once installed, HALO discovers it automatically through Python entry points and you can target it with:

```bash
halo generate --platform my_platform --composer-output-dir ./halo_out
```

## Create a new protocol generator

HALO can also scaffold a new protocol generator.

Example:

```bash
halo create-generator --generator-type protocol --no-interactive \
  --name my_protocol \
  --description "My Protocol Generator" \
  --author "Your Name" \
  --email "you@example.com" \
  --platform my_protocol \
  --supported-platforms linux,freertos \
  --output-dir ./generators
```

Then install it:

```bash
cd ./generators/halo-proto-my_protocol
pip install -e .
```

After that, HALO can use the new protocol generator whenever the unified model contains profiles handled by that generator.
