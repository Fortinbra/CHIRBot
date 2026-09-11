# NES controller to core UART

> **Status:** Prototype end-to-end acceptance scenario.

## Goal

Pressing or releasing a button on an NES controller connected to the input
module produces a decoded TASD event on the core module UART. The core also
forwards each validated frame to its output SPI link.

## Firmware

Flash these images after a top-level build:

| Board | UF2 |
| --- | --- |
| Input PGA2350 | `build/firmware/modules/input-nes-snes/input_nes_snes.uf2` |
| Core PGA2350 | `build/firmware/chirbot-core/chirbot_core.uf2` |

## Bench wiring

All GPIO signals are 3.3 V. Connect grounds between boards. Connect the NES
controller only through the fixed-direction 3.3 V/5 V level shifters described
in `hardware/PROTOTYPE.md`.

### NES controller to input module

| Input GPIO | Direction | NES signal |
| ---: | --- | --- |
| GP2 | Output through 3.3 V to 5 V translator | LATCH |
| GP3 | Output through 3.3 V to 5 V translator | CLOCK |
| GP4 | Input through 5 V to 3.3 V translator | DATA |

### Input module to core

| Core | Input module | Signal |
| --- | --- | --- |
| GP18 | GP18 | SCLK |
| GP19 | GP19 | MOSI |
| GP16 | GP16 | MISO |
| GP17 | GP17 | CS, active low |
| GND | GND | Common reference |

### Core UART

Connect a 3.3 V UART adapter at 115200 baud, 8 data bits, no parity, one stop
bit. Connect core GP0 TX to adapter RX and connect ground. GP1 is UART RX but
is not required for this scenario.

## Data flow

1. The input module latches and clocks eight NES bits approximately once per
   core poll.
2. Active-low wire data is converted to the active-high TASD button byte.
3. A changed state becomes a TASD `INPUT_MOMENT` in a versioned, CRC-protected
   64-byte SPI frame.
4. The core polls the input link at 1 kHz, validates and parses a new frame,
   prints each TASD packet, and forwards the frame to SPI1.

Example UART output after pressing A:

```text
CHIRBot core ready: SPI 2000000 Hz, UART stdio
[TASD seq=1] key=0x00f0 len=3 port=0 controller=0x0101
[TASD seq=1] key=0xfe02 len=12 port=0 hold=0 index_type=3 index=42 data=01 buttons=A
```

Releasing A produces another sequence with `data=00 buttons=none`. The test
passes when each press and release produces exactly one new input-moment line
with the expected button names and no repeated sequence while idle.

## Prototype limits

- GPIO assignments are bench defaults, not the final dock connector pinout.
- There is no discovery, handshake, ready/IRQ, retry, or output acknowledgement.
- Attach modules only while powered off.
