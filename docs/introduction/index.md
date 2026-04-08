# Introduction

HALO stands for `Harmonizing Asymmetric Multi-core Architectures with Linked communication for Optimal design`.

It is a model-driven workflow for describing heterogeneous systems, validating communication intent across domains, and generating portable plus platform-specific implementation artifacts from one unified model.

## Why HALO exists

Modern embedded and heterogeneous systems often combine Linux applications, RTOS tasks, bare-metal firmware, and accelerator logic. These parts all need to exchange data reliably, but the communication contract usually gets split across architecture diagrams, memory maps, handwritten code, and protocol assumptions.

HALO addresses that by making the contract explicit:

- architecture is modeled separately from interfaces,
- interfaces are modeled separately from data definitions,
- hardware mapping is modeled separately from communication intent,
- composition brings those views together into one inspectable system model.

The central idea is to stop rewriting the same integration knowledge in multiple places and let the model become the source of truth.

## HALO model layers

HALO is organized around several connected description layers:

- `ADL` (Architecture Description Language) describes Components, platforms, functions, and logical connections between endpoints.
- `IDDL` (Interface Definition Description Library) Provides a library of pre-defined data structures used by interfaces. Definitions can be reused across multiple interfaces.
- `IDL` (Interface Description Language) defines the interfaces between components, focusing on the data being exchanged with access and integrity rules.
- `HMML` (Hardware Module Mapping Language) provides a library of pre-defined hardware mappings used by HML Descriptions.
- `HML` (Hardware Mapping Language) defines the profile of components data exchange, focusing on the data placement, synchronization and transfer behaviour. 
HML profile is mapped uniquely to the ”communication” type defined in ADL.

HALO is strongest when these concerns stay separate during authoring and become unified only during composition.

## DSL and UML inputs

HALO supports two equivalent modeling entry points.

### Textual DSL workflow

You can author HALO systems directly in textual files:

- `*.adl`
- `*.idl`
- `*.iddl`
- `*.hmml`
- `*.hml`

These inputs are composed into one unified model by the HALO composer.

### UML workflow

You can also represent the same system in UML/XMI form:

- components become UML classes with HALO-specific attributes,
- interfaces and referenced structs become UML interface and data-type views,
- profiles become stereotyped UML classes,
- connections become relationship artifacts between source and destination components.

This gives HALO a round-trip path between textual DSL authoring and UML-based modeling and review.

## Composer flow

At a high level, the HALO composer works like this:

1. Author HADL files or import a UML XMI model.
2. Parse ADL, IDL, IDDL, HMML, and HML into structured models.
3. Resolve cross-references and compose one unified HALO model.
4. Validate interfaces, profiles, connection semantics, and referential integrity.
5. Emit machine- and human-consumable artifacts.

Typical outputs include:

- unified JSON and Pickle model artifacts,
- DOT and SVG views,
- PlantUML diagrams,
- UML/XMI exports,
- DSL exports for round-trip workflows.

The composer also supports built-in standard profile libraries through `--include-stdlib-profiles`, including shared-memory, blackboard, and event-channel style profiles.

## The unified model

The unified model is HALO's core artifact.

It brings together:

- components,
- connections,
- interfaces,
- profiles,
- data types,
- value-domain metadata.

This unified representation is the handoff point between composition, analysis, visualization, and code generation. It is meant to be understandable by humans and stable enough for downstream tooling.

## Code generation model

HALO uses the unified model to drive a layered generation flow:

- a core HALO generator emits the common HALO API and shared runtime layer,
- platform generators emit target-specific initialization and integration code,
- protocol generators emit protocol-specific transport code based on the selected profiles.

This design lets HALO support multiple environments without hard-coding all target logic into one generator.

## Plugin ecosystem

HALO is designed as an extensible plugin ecosystem.

- `halo create-generator` can scaffold new platform generators.
- `halo create-generator` can scaffold new protocol generators.
- generators are auto-discovered through Python entry points.
- Jinja2 templates are used to shape emitted source, headers, and config files.

The goal is to let users extend HALO for new platforms and communication styles without modifying the core framework every time.

## Integration mindset

HALO is intended to fit into an existing application project rather than replace it.

HALO typically owns:

- generated communication headers and sources,
- generated platform initialization files,
- generated protocol support files.

The application project still owns:

- business logic,
- scheduling and control behavior,
- hardware handles and RTOS objects,
- build-system wiring,
- custom code placed in preserved user-code blocks.

## Related sections

- See `Getting Started` for installation and first-run commands.
- See `Integration` for how generated outputs fit into an application build.
- See `Examples` for end-to-end usage flows.
- See `API` for the generated Python and C symbol reference.
