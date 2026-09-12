# Clock domains

> **Status:** Prototype NES/SNES latest-state behavior implemented; general
> policy remains an outline.

This specification will define how CHIRBot samples inputs and presents,
records, and replays state when the output device owns the poll edge. The
system-level rule is described in [the architecture](../ARCHITECTURE.md).

## Required decisions

- Meaning of input sample time, output poll time, and recorded time
- Latest-state buffering and behavior for transitions between output polls
- Output-module notification and core response deadlines
- Protocols with no explicit poll edge and external synchronization inputs
- Multiple polls per frame, filtering, overread, and protocol-specific quirks
- Recording semantics for missed, duplicate, delayed, or partial polls
- Playback behavior when source and target clocks differ
- Measurement method and pass/fail bounds for latency and jitter

Examples should include at least NES/SNES serial polling and one asynchronous
or USB-based protocol before this specification is accepted.

## NES/SNES prototype behavior

The output module caches the most recent complete, valid TASD input state. A
console LATCH snapshots that cached value into the PIO serializer. A new SPI
frame received during a native poll is queued for the next LATCH and does not
change the in-progress shift sequence. Repeated console polls return the same
state until the core forwards a newer sequence.
