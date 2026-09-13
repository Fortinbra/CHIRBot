# 0003 - Carrier and Multislot Bus Topology

- **Status:** Accepted
- **Date:** 2026-09-12
- **Supersedes:** The point-to-point and direct-to-core docking portions of
  [design decision 0001](0001-module-link-bus-and-connector.md)
- **Relates to:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md) section 2.3,
  [module interface](../specs/module-interface.md), and
  [SPI matrix protocol](../specs/spi-matrix-protocol.md)

## Context

The direct-dock design provides only one input and one output module and makes
the core PCB responsible for every module connector. CHIRBot instead needs a
cable-free way to install several modules while preserving the core's role as
the sole bus controller. Future hardware may also need a low-speed management
or expansion bus that does not replace SPI for real-time TASD traffic.

## Decision

CHIRBot uses a passive module carrier/backplane between the core controller
board and the modules. The carrier distributes buses, per-slot control signals,
power, and ground; it does not contain a routing MCU and does not permit
peer-to-peer module communication.

The production carrier provides two independent module banks:

| Bank | Capacity | Shared SPI signals | Per-slot signals |
| --- | ---: | --- | --- |
| Input | 4 modules | SCLK, MOSI, MISO | CS0-CS3 and IRQ0-IRQ3 |
| Output | 4 modules | SCLK, MOSI, MISO | CS0-CS3 |

The core is the sole SPI controller/main and selects exactly one module in a
bank by asserting that slot's chip select. Each module is an SPI
peripheral/subnode and must leave MISO high-impedance whenever its chip select
is inactive, including during reset. Input IRQ signals remain dedicated and
level-sensitive so the core can identify and service pending data without
scanning every slot. The output connector may reserve the corresponding
contact for a future alert function, but output TASD delivery does not require
an IRQ.

Each bank also carries an electrically independent 3.3 V I2C segment, SDA and
SCL, to all four slots for possible future management or expansion. I2C is
reserved and disabled in the current protocol; it is not the TASD data path or
the module discovery mechanism. The carrier owns the one pull-up set for each
segment. Modules must not fit fixed pull-ups by default, drive either line
high, back-power an unpowered bus, or assume an address until a future
specification defines addressing, isolation, recovery, and enablement.

The core and all modules connect to the carrier without module-link cables.
Every module slot uses a board-to-board connector. The exact connector family,
pinout, module outline, core-to-carrier interface, and mechanical retention
remain open in the module-interface specification.

## Consequences

- The production system has physical capacity for four input and four output
  modules. Supported simultaneous routes remain a firmware and performance
  contract, not an implication of slot count.
- The two SPI banks isolate input traffic and faults from time-sensitive output
  traffic while using the RP2350B's two SPI controllers naturally.
- The core requires eight chip-select outputs and four input IRQs in addition
  to the two shared SPI signal sets and the reserved I2C signals.
- Shared-bus signal integrity, inactive MISO behavior, aggregate latency, and
  per-slot power protection become production acceptance requirements.
- A module that holds MISO or a future I2C line in the wrong state can impair
  its bank. The carrier should provide isolation or mitigation footprints where
  practical, with final requirements set during electrical design.
- The carrier is a passive interconnect PCB and is not subject to the RP2350B
  requirement for active core and module boards.
- The one-input/one-output point-to-point prototype remains valid bench
  hardware but is not the production topology.

## Open implementation decisions

- Connector family, contact assignment, keying, insertion life, and retention
- Core-to-carrier mechanical and electrical interface
- Per-slot power budget, switching, protection, and presence detection
- SPI clock, loading, source termination, and aggregate service-time budget
- I2C pull-up values, address policy, optional isolation, and stuck-bus recovery
- Whether output-slot reserved alert contacts are routed to core GPIO
