# 0004 — Dual-Role Protocol Modules

- **Status:** Accepted (module architecture); carrier topology remains open
- **Date:** 2026-10-17
- **Relates to:** [architecture](../ARCHITECTURE.md),
  [system specification](../SYSTEM_SPECIFICATION.md),
  [module interface](../specs/module-interface.md), and
  [SPI matrix protocol](../specs/spi-matrix-protocol.md)

## Context

The input and output boards for a given native protocol use the same class of
controller, module-link interface, and native signaling, with the primary
difference being whether the module samples a device or presents a state to
one. Separate input and output firmware and hardware specifications duplicate
the shared protocol and electrical requirements.

The existing design also gives input and output separate carrier banks.
Consolidating each protocol pair into one selectable product affects how the
carrier identifies and connects a module's active role, so its physical layout
must be reconsidered independently.

## Decision

1. For each supported native protocol family, define one module product and
   one module firmware image that supports both input and output roles. This
   does not mean one universal firmware image for unrelated native protocols.
2. Each module has a physical input/output selector. Firmware samples it at
   startup and after reset; the reported role remains fixed until the next
   reset. Moving the selector while powered does not change the active role.
3. A module operates in exactly one role at a time. Input role samples the
   attached native device and exposes TASD data to the core. Output role
   receives TASD from the core and presents it to the attached native device.
   The module is not a simultaneous input/output bridge.
4. Each protocol-family module has one shared native-device connector
   interface. The role-appropriate cable or adapter connects that interface
   to the controller or target device. Connector family and adapter details
   are protocol-specific and require electrical validation.
5. The selector and role-specific electrical paths must fail safe. Before
   role initialization and during reset, the module must not contend with
   native-device drivers or assert an unsafe output. Only circuits needed by
   the latched role may drive native signals.
6. The existing passive-carrier architecture remains. The final carrier
   topology is not decided by this ADR. Candidate approaches include retaining
   separate input and output banks with a module docked according to its
   selected role, or defining a universal slot type whose contacts support
   either role. The choice affects slot capacity, IRQ routing, power/fault
   isolation, and how a role/slot mismatch is handled; record it in a
   follow-up decision before production carrier design.

The previous four-input/four-output carrier topology in
[decision 0003](0003-carrier-and-multislot-bus-topology.md) remains historical
design context, not a settled topology for the dual-role module system. Its
passive carrier, core-controlled SPI, inactive-MISO, and disabled-I2C
principles remain valid unless superseded by a later decision.

## Consequences

- Input and output behavior for the same native protocol is specified,
  implemented, tested, and versioned together.
- The module descriptor reports the boot-latched role. The core must validate
  this role against the carrier slot and routing configuration before
  enabling TASD traffic.
- The physical role switch is not a runtime mode control. A role change
  requires a reset/restart and a fresh module discovery.
- The hardware must include safe role-dependent native signal paths and
  reset behavior. A common connector does not imply that its external wiring
  is electrically interchangeable without the specified adapter/cable.
- Existing separate input/output prototype boards and firmware images remain
  useful bring-up fixtures. They are not evidence that the consolidated
  design or its role-switch safety has been implemented or validated.
- Carrier capacity, bank grouping, connector contacts, role-mismatch behavior,
  and role-dependent IRQ routing remain open.

## Required follow-up

- Define the switch electrical circuit, input assignment, startup sampling,
  and reset-safe behavior.
- Define the shared native connector and adapter/cable pinouts for each
  protocol family.
- Select universal-slot versus role-specific-bank carrier topology and
  specify slot compatibility, IRQ routing, and fault isolation.
- Consolidate each protocol family's firmware and hardware documentation;
  preserve separate prototype wiring only as clearly labeled historical or
  bench reference material.
- Verify both role paths and reset/power transitions on hardware before
  treating the module as production-ready.
