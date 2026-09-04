# SPI matrix protocol

> **Status:** Outline. Not yet implementable.

This specification will define the point-to-point SPI contract between the
CHIRBot core (main) and each docked module (subnode). The bus and topology are
accepted in [design decision 0001](../design/0001-module-link-bus-and-connector.md).

## Required decisions

- SPI mode, clock range, transaction ownership, and chip-select behavior
- Packet envelope, length limits, integrity checks, and byte order
- Module discovery, identity, capabilities, and protocol version negotiation
- Input, output, configuration, response, and asynchronous-ready semantics
- Timeouts, malformed-packet handling, recovery, and fault isolation
- Latency and jitter budget, including the RP2350 PL022 subnode clock limit
- Conformance vectors and host-buildable protocol tests

Live relay framing must not be assumed to be identical to a TASD file packet;
that relationship is defined in [TASD usage](tasd-usage.md).