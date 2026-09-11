# SPI matrix protocol

> **Status:** Prototype v1. Implemented for bench testing; not accepted as the
> final module interface.

This specification defines the initial point-to-point SPI contract between the
CHIRBot core (main) and each docked module (subnode). The bus and topology are
accepted in [design decision 0001](../design/0001-module-link-bus-and-connector.md).

## Electrical and transaction contract

- SPI mode 0, MSB first, 8-bit words
- 2 MHz clock
- Core is the main and initiates one 64-byte transaction every 1 ms
- Chip select is active low for exactly one frame
- Input and output links are point-to-point and use independent chip selects
- No ready/IRQ signal or response transaction is used in prototype v1

## Frame

Every transfer is exactly 64 bytes. Multi-byte integers are big-endian.

| Offset | Size | Field |
| ---: | ---: | --- |
| 0 | 2 | Magic, ASCII `CH` |
| 2 | 1 | Protocol version, currently `1` |
| 3 | 1 | Type: `0` empty, `1` TASD document |
| 4 | 4 | Sequence number |
| 8 | 2 | Payload length, 0 through 50 |
| 10 | 50 | Payload followed by zero padding |
| 60 | 4 | IEEE CRC-32 over bytes 0 through 59 |

The input module increments the sequence only when controller state changes.
The core ignores a repeated sequence, logs every TASD packet in a new frame,
and forwards the original validated frame unchanged to the output link.

The shared implementation is in
`firmware/chirbot-common/include/chirbot/link_protocol.h`.

## Still required before acceptance

- Module discovery, identity, capabilities, and protocol version negotiation
- Configuration, response, and asynchronous-ready semantics
- Timeouts, malformed-packet handling, recovery, and fault isolation
- Latency and jitter budget, including the RP2350 PL022 subnode clock limit
- Conformance vectors and host-buildable protocol tests
