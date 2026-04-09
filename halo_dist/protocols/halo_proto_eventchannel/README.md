# Eventchannel Protocol Generator for HALO

Event Channel Protocol

## Installation

```bash
pip install -e .
```

## Usage

Once installed, the protocol generator is discovered through the `halo.protocol_generators` entry point.

The starter scaffold includes render functions for:
- stm32
- linux
- freertos
- baremetal

## Project Layout

```text
halo_proto_eventchannel/
├── pyproject.toml
├── README.md
├── LICENSE
├── .gitignore
└── src/
    └── halo_proto_eventchannel/
        ├── __init__.py
        ├── config.py
        ├── render.py
        └── templates/
```

## Customization

1. Implement your generator logic in `src/halo_proto_eventchannel/render.py`.
2. Adjust `src/halo_proto_eventchannel/config.py` if you need different discovery metadata.
3. Edit the Jinja2 templates under `src/halo_proto_eventchannel/templates/`.

## Notes

- The scaffold uses bundled Jinja2 templates to generate this package.
- The selected template engine is recorded as `jinja2` in the scaffold metadata, but the starter implementation targets Jinja2.

## License

MIT License