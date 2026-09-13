# Module interface

> **Status:** Outline. Not yet implementable.

This specification will define the electrical, mechanical, power, and
identification contract for every module connected to the passive CHIRBot
carrier. The carrier and its four-input/four-output topology are accepted in
[design decision 0003](../design/0003-carrier-and-multislot-bus-topology.md),
but the connector and physical envelope remain open.

## Accepted requirements

- The core is the SPI controller/main (master), and every module is an SPI
  peripheral/subnode. Modules cannot initiate SPI transactions.
- The carrier provides one shared SPI bank for four input slots and another
  shared SPI bank for four output slots. SCLK, MOSI, and MISO are shared within
  a bank; each slot receives one dedicated chip-select signal from the core.
- A module must keep MISO high-impedance whenever its chip select is inactive,
  including during reset and power transitions. The carrier provides an
  external inactive-state pull for each chip select.
- Every compliant module returns a self-description record when queried by the
  core. The record contains at least:
  - module type: input or output
  - stable vendor and product identifiers
  - hardware revision and firmware version
  - minimum and maximum supported SPI link-protocol versions
  - native protocol or device-family identifier
  - a length-delimited capability set for protocol-specific features
- A serial number or other instance identifier may be supplied when a product
  needs per-unit configuration, but is not required for basic compatibility.
- The core validates module type against the dock and negotiates a compatible
  link-protocol version before TASD exchange. Unknown or incompatible modules
  remain inactive and are reported to the user.
- Every input dock provides a dedicated module-to-core data-ready/IRQ contact.
  The signal is level-sensitive: an input module keeps it asserted while one
  or more unread TASD records are pending and deasserts it only after the core
  has retrieved or explicitly discarded all pending data.
- Output modules do not use data-ready/IRQ for normal TASD delivery because the
  core initiates output writes.
- Each input and output slot carries 3.3 V SDA and SCL for a future I2C
  management or expansion bus. Input and output banks are separate I2C
  segments. I2C is currently reserved and disabled; module discovery and TASD
  transport remain on SPI.
- The carrier owns the single pull-up set for each I2C segment. Modules must
  not populate fixed pull-ups by default, drive SDA or SCL high, back-power an
  unpowered segment, or assume an address before a future revision defines the
  I2C contract.

## Logical slot signals

Every module slot reserves these functions. Exact contact numbers remain open.

| Signal | Scope | Requirement |
| --- | --- | --- |
| SCLK | Shared within bank | Driven only by the core |
| MOSI | Shared within bank | Core-to-module SPI data |
| MISO | Shared within bank | Selected module only; high-impedance otherwise |
| CS | Dedicated per slot | Core-controlled module selection |
| IRQ/alert | Dedicated per slot | Required data-ready on inputs; reserved on outputs |
| I2C SDA, SCL | Shared within bank | Reserved 3.3 V open-drain future bus |
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
- Input/output slot keying and behavior when a module is in the wrong slot
- Power-off-only or hot-swap policy and the hardware that enforces it
- Numeric registries and compatibility rules for identity and capability fields
- Mechanical and electrical compliance checklist for community modules

The selected connector and power policy should be recorded in a superseding or
follow-up ADR before this specification is marked draft.
