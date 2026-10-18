# Module interface

> **Status:** Outline. Not yet implementable.

This specification defines the shared electrical, mechanical, power, and
identification contract for protocol-specific modules that can operate as
either input or output. Each module has a physical role selector sampled at
boot/reset and one shared native-device connector, used with a role-appropriate
cable or adapter where necessary. The production carrier topology, connector,
and physical envelope remain open. The earlier four-input/four-output
topology is recorded in
[design decision 0003](../design/0003-carrier-and-multislot-bus-topology.md);
module consolidation is recorded in
[design decision 0004](../design/0004-dual-role-module-architecture.md).

## Accepted requirements

- The core is the sole SPI controller/main, and every module is an SPI
  peripheral/subnode. The carrier remains passive; whether it uses separate
  input/output banks or universal slots is undecided.
- A module must keep MISO high-impedance whenever its chip select is inactive,
  including during reset and power transitions. The carrier provides an
  external inactive-state pull for each chip select.
- Every compliant module returns a self-description record when queried by the
  core. The record contains at least:
  - selected role: input or output, reflecting the physical selector sampled
    during startup
  - stable vendor and product identifiers
  - hardware revision and firmware version
  - minimum and maximum supported SPI link-protocol versions
  - native protocol or device-family identifier
  - a length-delimited capability set for protocol-specific features
- A serial number or other instance identifier may be supplied when a product
  needs per-unit configuration, but is not required for basic compatibility.
- The core validates the selected role against slot/routing configuration and
  negotiates a compatible link-protocol version before TASD exchange. Unknown
  or incompatible modules remain inactive and are reported to the user.
- Changing the role selector while powered does not change the running role.
  The selector takes effect only after reset/restart; firmware reports the
  latched role during discovery.
- A module in input role asserts data-ready/IRQ while one or more unread TASD
  records are pending, and deasserts it only after the core retrieves or
  explicitly discards all pending data. Its physical routing depends on the
  selected carrier topology. Output-role TASD delivery is core-initiated and
  does not require data-ready/IRQ.
- I2C SDA/SCL may be reserved for a future 3.3 V management or expansion bus.
  Segment organization depends on the carrier topology. I2C is currently
  disabled; discovery and TASD transport remain on SPI.
- If I2C is carried, the carrier owns each segment's pull-up set. Modules must
  not populate fixed pull-ups by default, drive SDA or SCL high, back-power an
  unpowered segment, or assume an address before a future revision defines
  the I2C contract.

## Logical slot signals

Every module slot reserves these functions. Exact contact numbers remain open.

| Signal | Scope | Requirement |
| --- | --- | --- |
| SCLK | Shared within bank | Driven only by the core |
| MOSI | Shared within bank | Core-to-module SPI data |
| MISO | Shared within bank | Selected module only; high-impedance otherwise |
| CS | Dedicated per slot | Core-controlled module selection |
| IRQ/alert | Role-dependent; final routing open | Required data-ready in input role; not required for normal output delivery |
| I2C SDA, SCL | Carrier-topology dependent | Reserved 3.3 V open-drain future bus |
| Module supply | Distributed per slot | Voltage and budget pending |
| Ground | Multiple contacts | Signal return and power return |

The binary descriptor encoding, discovery commands, response framing, and IRQ
service sequence belong to the
[SPI matrix protocol](spi-matrix-protocol.md). This specification owns the
descriptor field meanings and the connector's electrical realization.

## Required decisions

- Module-slot and core-to-carrier connector families, manufacturer-qualified
  parts, orientation, and pinouts
- Supply voltage, steady-state and inrush budgets, protection, and grounding
- Logic levels, data-ready active level and pull state, reserved pins, signal
  integrity constraints, and test points
- Shared-SPI loading, trace topology, termination, and per-slot fault isolation
- I2C pull-up values, address registry, isolation, and stuck-bus recovery
- Module PCB outline, keep-outs, connector datum, mounting, and retention
- Carrier topology: separate role-specific banks or universal slots, including
  how role-dependent IRQ and I2C contacts are routed
- Behavior when a module's selected role is incompatible with its slot
- Physical selector type, circuit, GPIO assignment, boot sampling, and reset
  safety
- Native connector family per protocol module and the required role-specific
  cable/adapter
- Power-off-only or hot-swap policy and the hardware that enforces it
- Numeric registries and compatibility rules for identity and capability fields
- Mechanical and electrical compliance checklist for community modules

The selected connector and power policy should be recorded in a superseding or
follow-up ADR before this specification is marked draft.
