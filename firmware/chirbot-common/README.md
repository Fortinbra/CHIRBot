# chirbot-common

> **Status:** planned — no code yet.

Shared firmware libraries consumed by the core and all module firmware, so the
wire protocol can never drift between components:

- **SPI matrix link protocol** — framing, timing, main/subnode roles
  (spec: [docs/specs/spi-matrix-protocol.md](../../docs/specs/spi-matrix-protocol.md))
- **Module discovery / handshake / config** — protocol versioning, capability
  reporting (spec: [docs/specs/module-interface.md](../../docs/specs/module-interface.md))
- **TASD wrappers** — CHIRBot-specific conventions (SPI packetization, storage
  layout) on top of the [TASD library](../TASD/); this library does not
  reimplement TASD serialization

Pure-logic code here should be host-buildable for unit testing on desktop.

Will be promoted to its own semver-tagged repo (`chirbot-common`) once it has
real content. See [docs/ARCHITECTURE.md](../../docs/ARCHITECTURE.md).
