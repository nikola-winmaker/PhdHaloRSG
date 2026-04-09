# Sharedmemory Protocol Generator for HALO

HALO protocol generator for SharedMemory

## Installation

```bash
pip install -e .
```

## Usage

Once installed, the protocol generator is discovered through the `halo.protocol_generators` entry point.

The starter scaffold includes render functions for:
- baremetal
- freertos
- linux
- stm32
- zephyr

## Project Layout

```text
halo-proto-sharedmemory/
├── pyproject.toml
├── README.md
├── LICENSE
├── .gitignore
└── src/
    └── halo_proto_sharedmemory/
        ├── __init__.py
        ├── config.py
        ├── render.py
        └── templates/
```

## Customization

1. Implement your generator logic in `src/halo_proto_sharedmemory/render.py`.
2. Adjust `src/halo_proto_sharedmemory/config.py` if you need different discovery metadata.
3. Edit the Jinja2 templates under `src/halo_proto_sharedmemory/templates/`.

## Notes

- The scaffold uses bundled Jinja2 templates to generate this package.
- The selected template engine is recorded as `jinja2` in the scaffold metadata, but the starter implementation targets Jinja2.

## License

MIT License