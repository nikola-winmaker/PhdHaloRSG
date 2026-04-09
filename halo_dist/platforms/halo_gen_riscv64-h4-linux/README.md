# Riscv64 H4 Linux Platform Generator for HALO

Linux OS running on riscv 64 U74

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
halo_gen_riscv64-h4-linux/
├── pyproject.toml
├── README.md
├── LICENSE
├── .gitignore
└── src/
    └── halo_gen_riscv64_h4_linux/
        ├── __init__.py
        ├── config.py
        ├── render.py
        └── templates/
```

## Customization

1. Implement your generator logic in `src/halo_gen_riscv64_h4_linux/render.py`.
2. Adjust `src/halo_gen_riscv64_h4_linux/config.py` if you need different discovery metadata.
3. Edit the Jinja2 templates under `src/halo_gen_riscv64_h4_linux/templates/`.

## Notes

- The scaffold uses bundled Jinja2 templates to generate this package.
- The selected template engine is recorded as `jinja2` in the scaffold metadata, but the starter implementation targets Jinja2.

## License

MIT License