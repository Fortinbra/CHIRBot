# SPI matrix protocol

> **Status:** Prototype v1. Implemented for bench testing; not accepted as the
> final module interface.

This specification defines the initial point-to-point SPI contract between the
CHIRBot core (main) and each prototype module (subnode). SPI was accepted in
[design decision 0001](../design/0001-module-link-bus-and-connector.md). The
production shared-bank topology is defined by
[design decision 0003](../design/0003-carrier-and-multislot-bus-topology.md).

## Electrical and transaction contract

- SPI mode 0, MSB first, 8-bit words
- 2 MHz clock
- Core is the main and initiates one 64-byte transaction every 1 ms
- Chip select is active low for exactly one frame
- Input and output links are point-to-point and use independent chip selects
- No ready/IRQ signal or response transaction is used in prototype v1

The last item is a prototype limitation, not the production architecture.
Production input links use the data-ready behavior below.

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

## Production discovery and data-ready requirements

The production protocol must preserve these rules when its frame types are
defined:

1. The core is the sole SPI controller/main (master) and initiates every transaction. A module only returns data while clocked by the core.
2. Production uses two independent shared SPI banks: four input slots and four output slots. SCLK, MOSI, and MISO are shared within each bank, and every slot has a dedicated chip select. The core asserts exactly one chip select in a bank for a transaction. All unselected modules keep MISO high-impedance.
3. On startup, after a module reset, and whenever presence detection indicates a newly available module, the core issues a descriptor query before any TASD exchange.
4. Every input and output module responds with the descriptor defined by the [module-interface specification](module-interface.md). Discovery must allow the core to validate module type, identity, link-version compatibility, and capabilities without interpreting TASD payloads.
5. An input module asserts its dedicated per-slot data-ready/IRQ signal whenever unread TASD data is pending. The assertion is level-sensitive and remains active until the core clocks transactions that retrieve or explicitly discard all pending data. The core may poll as a bounded recovery mechanism, but normal operation must not depend on continuous polling.
6. An IRQ assertion only requests service; it does not transfer data or change SPI ownership. The core decides when to select the module and clock each read.
7. Output TASD delivery is core-initiated and requires no output-module data-ready signal in the normal path.
8. The reserved I2C segments are outside this protocol. They remain disabled and cannot be required for discovery, compatibility checks, or TASD exchange.

Exact command values, descriptor encoding, queue-depth reporting, IRQ
electrical polarity, per-bank scheduling, timeouts, and recovery behavior
remain to be specified before this document can advance beyond prototype
status. The production timing budget must cover four pending input modules and
time-sensitive output writes; the prototype's 2 MHz fixed-frame cadence is not
automatically a valid production limit.

## Still required before acceptance

- Discovery command and descriptor encoding, capability registries, and
  protocol-version negotiation details
- Configuration, response, queueing, and data-ready recovery semantics
- Per-bank arbitration, fairness, timeouts, malformed-packet handling,
  recovery, and fault isolation
- Latency and jitter budget, including the RP2350 PL022 subnode clock limit
- Conformance vectors and host-buildable protocol tests
