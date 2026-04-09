# Riscv64 H3 Baremetal Platform Generator for HALO

Bare-metal application running on riscv 64 U74

## Installation

```bash
pip install -e .
```

## Usage

Once installed, the generator is discovered through the `halo.generators` entry point:

```bash
halo generate --composer-output-dir ./output
```

## Project Layout

```text
halo_gen_riscv64-h3-baremetal/
├── pyproject.toml
├── README.md
├── LICENSE
├── .gitignore
└── src/
    └── halo_gen_riscv64_h3_baremetal/
        ├── __init__.py
        ├── config.py
        ├── render.py
        └── templates/
```

## Customization

1. Implement your generator logic in `src/halo_gen_riscv64_h3_baremetal/render.py`.
2. Adjust `src/halo_gen_riscv64_h3_baremetal/config.py` if you need different discovery metadata.
3. Edit the Jinja2 templates under `src/halo_gen_riscv64_h3_baremetal/templates/`.

## Notes

- The scaffold uses bundled Jinja2 templates to generate this package.
- The selected template engine is recorded as `jinja2` in the scaffold metadata, but the starter implementation targets Jinja2.

## License

MIT License