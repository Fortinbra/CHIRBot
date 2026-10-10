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
- No ready/IRQ signal is used in prototype v1. Playback reads output status
  on MISO during the same full-duplex transaction.

The last item is a prototype limitation, not the production architecture.
Production input links use the data-ready behavior below.

## Frame

Every transfer is exactly 64 bytes. Multi-byte integers are big-endian.

| Offset | Size | Field |
| ---: | ---: | --- |
| 0 | 2 | Magic, ASCII `CH` |
| 2 | 1 | Protocol version, currently `1` |
| 3 | 1 | Type: `0` empty, `1` live TASD, `2` playback begin, `3` playback state, `4` output status, `5` playback stop, `6` playback config |
| 4 | 4 | Sequence number |
| 8 | 2 | Payload length, 0 through 50 |
| 10 | 50 | Payload followed by zero padding |
| 60 | 4 | IEEE CRC-32 over bytes 0 through 59 |

The input module increments the sequence only when controller state changes.
The core ignores a repeated sequence, logs every TASD packet in a new frame,
and forwards the original validated frame unchanged to the output link.

The shared implementation is in
`firmware/chirbot-common/include/chirbot/link_protocol.h`.

## Prototype playback flow control

Type 6 carries a two-byte big-endian latch-filter interval in microseconds
(0..65535). The core sends and waits for its acknowledgement before each begin,
including zero when a file has no filter. It takes effect only at the next
begin, not during the current report. Duplicate configuration retries are
ignored like other commands. Both images must support type 6; an older output
image times out explicitly rather than silently ignoring the file's filter.

Types 2 and 3 carry the same complete controller-state TASD document as type 1.
Begin selects the validated port/profile, clears any previous playback queue,
and initializes released buttons and the configured LATCH filter.
Playback state appends one report to the
four-entry PIO TX FIFO. It is acknowledged only after validation and enqueue;
a full queue leaves the acknowledgement unchanged. Live type-1 reports accept
input-module port 0; playback types 2/3 accept movie player-1 port 1. Both route
to the same physical output connector; other ports are ignored. Profiles must
not change within playback.

The output returns type 4 in every transaction. Its sequence is the last
accepted command sequence. Its two-byte payload is:

| Byte | Meaning |
| ---: | --- |
| 0 | `0` no command acknowledged yet; `1` sequence is valid |
| 1 | Free playback FIFO slots, 0 through 4 (4 outside playback mode) |

Status is prepared before receiving the current command, so acknowledgement
arrives on a later transaction. The core waits at least 1 ms between transactions
and retries an unacknowledged command with the same sequence. The output ignores
duplicates of the last accepted command (same type and sequence), including
begin and stop; retries cannot reset or duplicate the queue. Core sequences
continue across file selections.

Type 0 polls status without enqueueing. End-of-file waits until all four slots
are free, meaning the final report has been latched. Type 5 has no payload,
discards queued reports, releases buttons, and returns to live mode. A new
type-1 live frame also returns to latest-state mode.

A stopped console is allowed to leave playback blocked indefinitely, with
SELECT still serviced. Missing/corrupt output status or an unacknowledged command
with an empty queue times out after three seconds and reports a playback error.
Both core and output firmware must be updated together; older output firmware
does not acknowledge playback commands.

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
