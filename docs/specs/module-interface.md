# Module interface

> **Status:** Outline. Not yet implementable.

This specification will define the electrical, mechanical, power, and
identification contract for every module docked to a CHIRBot core. Board-to-
board docking is accepted in
[design decision 0001](../design/0001-module-link-bus-and-connector.md), but the
connector and physical envelope remain open.

## Required decisions

- Connector family, manufacturer-qualified parts, orientation, and pinout
- Supply voltage, steady-state and inrush budgets, protection, and grounding
- Logic levels, reserved pins, signal integrity constraints, and test points
- Module PCB outline, keep-outs, connector datum, mounting, and retention
- Input/output slot keying and behavior when a module is in the wrong slot
- Power-off-only or hot-swap policy and the hardware that enforces it
- Identity, capability, compatibility, and firmware-version fields
- Mechanical and electrical compliance checklist for community modules

The selected connector and power policy should be recorded in a superseding or
follow-up ADR before this specification is marked draft.