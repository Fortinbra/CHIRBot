# chirbot-common

> **Status:** Prototype SPI link framing implemented.

Shared firmware libraries consumed by the core and all module firmware, so the
wire protocol can never drift between components:

- **SPI matrix link protocol** — framing, timing, main/subnode roles
  (spec: [docs/specs/spi-matrix-protocol.md](../../docs/specs/spi-matrix-protocol.md))
- **Module discovery / handshake / config** — query transactions, descriptor
  encoding, protocol negotiation, and data-ready service (wire spec:
  [docs/specs/spi-matrix-protocol.md](../../docs/specs/spi-matrix-protocol.md);
  field meanings and electrical contract:
  [docs/specs/module-interface.md](../../docs/specs/module-interface.md))
- **TASD wrappers** — CHIRBot-specific conventions (SPI packetization, storage
  layout) on top of the [TASD library](../TASD/); this library does not
  reimplement TASD serialization

Pure-logic code here should be host-buildable for unit testing on desktop.

`chirbot_link_protocol` provides the shared fixed-size frame encoder, decoder,
and CRC used by the core and modules. `chirbot_controller_tasd` provides the
shared NES/SNES controller-state TASD encoder and decoder, keeping the two
module implementations on one document contract. Its desktop test exercises
NES and SNES state through TASD encoding, CHIRBot framing, core-style
forwarding, and output decoding. The provisional wire contract is documented in
[spi-matrix-protocol.md](../../docs/specs/spi-matrix-protocol.md).

Will be promoted to its own semver-tagged repo (`chirbot-common`) once it has
real content. See [docs/ARCHITECTURE.md](../../docs/ARCHITECTURE.md).
