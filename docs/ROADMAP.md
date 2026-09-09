# CHIRBot roadmap

> **Status:** Working plan. Ordering may change as prototypes invalidate
> assumptions; accepted technical choices belong in [design decisions](design/).

The immediate goal is not to implement every planned component. It is to prove
one complete input-to-output path and stabilize the contracts that let later
modules interoperate.

## 0. Align the project baseline

- [ ] Update chirbot.com to reflect RP2350B and board-to-board module docking;
      keep the longer design discussions as clearly labeled historical context
- [ ] Choose hardware, documentation, and contribution licenses explicitly
- [ ] Add contribution and security guidance before inviting broad contributions
- [ ] Establish lightweight CI for documentation links and the TASD host tests

## 1. Stabilize shared interfaces

- [ ] Define the [module interface](specs/README.md): connector pinout, power
      budget, mechanical envelope, retention, and power-off-only/hot-swap policy
- [ ] Define the [SPI matrix protocol](specs/README.md): framing, transactions,
      discovery, capabilities, version negotiation, error handling, and timing
- [ ] Define CHIRBot's [TASD usage](specs/README.md), separating live transport
      needs from file-storage semantics
- [ ] Define the [clock-domain model](specs/README.md), including output poll
      edges, buffering, timestamps, and recording behavior
- [ ] Turn the approximately 1 ms objective into a measurable latency budget
      allocated across input sampling, transport, routing, and output response

## 2. Prove one vertical slice

- [ ] Select the simplest representative input/core/output combination
- [ ] Build a host-testable `chirbot-common` framing and handshake library
- [ ] Build the [three-MCU architecture prototype](../hardware/PROTOTYPE.md)
      using an RP2350B core and two modules on development hardware
- [ ] Demonstrate live relay, recording, and playback through the same path
- [ ] Measure latency, jitter, electrical margins, and replay behavior
- [ ] Feed prototype findings back into specs and superseding ADRs

NES/SNES is a strong candidate for this milestone because its signaling and
poll edge are well understood, but that selection should be recorded before
implementation begins.

## 3. Productize the proven path

- [ ] Design the core and first module PCBs from the stabilized interface
- [ ] Add repeatable firmware builds, tests, release artifacts, and flashing docs
- [ ] Implement the minimum host configuration and TASD inspection workflow
- [ ] Validate cost targets with sourced BOMs and small production quantities
- [ ] Document assembly, enclosure, recovery, and troubleshooting workflows

## 4. Expand deliberately

- [ ] Add modules one protocol family at a time using the stable shared contracts
- [ ] Add visualization and richer host application workflows
- [ ] Propose required rumble and motion extensions upstream to TASD
- [ ] Promote components to standalone repositories only when independent
      versioning, releases, or ownership provide a concrete benefit

## Definition of baseline readiness

The project has a credible baseline when the four shared specifications are
reviewable, one end-to-end hardware path meets its measured timing budget, its
build and tests run in CI, and a new module author can implement against the
published electrical, mechanical, and protocol contracts without copying an
existing module blindly.