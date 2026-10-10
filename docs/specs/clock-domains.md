# Clock domains

> **Status:** Prototype NES/SNES live and console-clocked playback implemented; general
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

For live input, the output module caches the most recent complete, valid TASD input state. A
console LATCH snapshots that cached value into the PIO serializer. A new SPI
frame received during a native poll is queued for the next LATCH and does not
change the in-progress shift sequence. Repeated console polls return the same
state until the core forwards a newer sequence.

File playback uses a separate four-report PIO TX FIFO, not the live latest-state
cache. Each unfiltered rising LATCH consumes one queued report; holding LATCH high
does not consume more reports. CLOCK shifts that report without waiting for the
CPU or SPI. A full queue rejects additional states without acknowledging them,
so the core retries the same sequence until space becomes available. SPI retries
of an acknowledged sequence do not enqueue a second copy.

No console polls means no playback advancement, regardless of NTSC/PAL region
or elapsed wall-clock time. An empty queue repeats the last report (released
buttons before the first queued state). Playback finishes only after the final
queued report has been latched, not merely sent over SPI. SELECT cancels pending
reports and releases buttons through an acknowledged stop command.

NES/SNES `LATCH_FILTER` metadata establishes a minimum interval between report
advancements, measured from a console LATCH. A second PIO state machine counts
microseconds and clears the serializer's IRQ gate; polls during that interval
re-latch the same complete report without consuming the queue. Expiry alone
never advances playback. IRQ0 is the hold gate; IRQ1 starts the timer (WAIT
automatically clears its request flag, so the two flags must be separate).
The 28-instruction serializer and four-instruction timer occupy all 32 PIO
instruction slots, without additional GPIO wiring.
The 1 MHz timer includes request detection, countdown loading and IRQ clearing:
the hold interval is conservatively up to about 3 us longer than the configured
value. This is a minimum-interval filter, not a video-frame clock.

Input chunks are poll instances, not necessarily video frames. Filtering groups
repeated polls; it does not synthesize missing video frames or apply lag metadata.
Clock-filter metadata is diagnosed but not yet implemented; electrical glitches
and clock filtering remain an outstanding limitation.
Only frame-indexed input moments and complete NES/SNES input chunks are supported;
other input-moment timebases are rejected rather than guessed.
Playback advances only for selected player-1 reports; interleaved reports and
controller declarations for other ports are ignored.
