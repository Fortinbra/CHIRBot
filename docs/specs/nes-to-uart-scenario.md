# NES controller to core UART

> **Status:** Prototype end-to-end acceptance scenario.

## Goal

Pressing or releasing a button on an NES controller connected to the input
module produces a decoded TASD event on the core module UART. The core also
forwards each validated frame to an output module, which presents the latest
state to an NES or SNES console when the console polls it.

## Firmware

Flash these images after a top-level build:

| Board | UF2 |
| --- | --- |
| Input PGA2350 | `build/firmware/modules/input-nes-snes/input_nes_snes.uf2` |
| Core PGA2350 | `build/firmware/chirbot-core/chirbot_core.uf2` |
| Output PGA2350 | `build/firmware/modules/output-nes-snes/output_nes_snes.uf2` |

## Bench wiring

PGA2350 and SPI GPIO signals are 3.3 V; the native NES side is nominally 5 V.
Connect grounds between boards. The assembled input fixture currently uses an
unqualified [hiBCTR four-channel BSS138 level shifter](https://www.amazon.com/dp/B0DSZ5KFKL)
(`H1-HBB0064-20`, ASIN `B0DSZ5KFKL`). Connect its `LV` rail to PGA2350
`3V3`, its `HV` rail to regulated controller-side 5 V, and its `GND` to common
ground. The seller's generic SPI compatibility claim is not a measured speed or
rise-time rating and does not qualify the NES LATCH/CLOCK waveforms.
Fixed-direction buffers remain required for final electrical acceptance as described in
[hardware/PROTOTYPE.md](../../hardware/PROTOTYPE.md).

### NES controller to input module

| Input GPIO | Direction | NES signal |
| ---: | --- | --- |
| GP2 | Output through 3.3 V to 5 V translator | LATCH |
| GP3 | Output through 3.3 V to 5 V translator | CLOCK |
| GP4 | Input through 5 V to 3.3 V translator | DATA |

The NES cable reaches the input fixture through an RJ45/8P8C breakout. This is
only a cable adapter, not Ethernet and not the CHIRBot module link. Verify the
contact-to-signal mapping and connector orientation by continuity; do not rely
on cable colors.

### SPIDriver to input module

An Excamera Labs SPIDriver can replace the core during initial SPI subnode
testing. Use mode 0 and transfer exactly 64 bytes while CS remains asserted.

| SPIDriver signal | Input PGA2350 | Direction |
| --- | ---: | --- |
| SCLK | GP18 | SPIDriver to input |
| MOSI | GP19 | SPIDriver to input |
| MISO | GP16 | Input to SPIDriver |
| CS | GP17 | SPIDriver to input, active low |
| GND | GND | Common reference |

This table is the firmware-expected mapping, not a verified record of wire
colors or breakout contacts. Confirm every connection by continuity. When the
PGA2350 is powered through its USB-C breakout, leave both SPIDriver power
outputs disconnected; connect only the four SPI signals and ground.

SPIDriver's published sustained SPI rate is approximately 500 kbit/s, below
the CHIRBot prototype contract of 2 MHz. A 64-byte transfer therefore takes at
least 1.024 ms before USB and command overhead. Use it to validate slave
framing, full-duplex data, chip-select behavior, and CRC contents at reduced
speed. It cannot validate the 2 MHz clock, 1 ms poll interval, latency, or
signal-integrity requirements.

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

### Core to output module

The data pins cross because core SPI1 TX drives output SPI0 RX, and output SPI0
TX drives core SPI1 RX.

| Core | Output module | Signal |
| --- | --- | --- |
| GP14 | GP18 | SCLK |
| GP15 | GP16 | Core MOSI to output RX |
| GP12 | GP19 | Output TX to core MISO |
| GP13 | GP17 | CS, active low |
| GND | GND | Common reference |

### Output module to console

The console owns LATCH and CLOCK. The module only drives DATA. Use the
fixed-direction translators from `hardware/PROTOTYPE.md`; RP2350 GPIO is not
5 V tolerant.

| Output GPIO | Direction | Console signal |
| ---: | --- | --- |
| GP2 | Input through 5 V to 3.3 V translator | LATCH |
| GP3 | Input through 5 V to 3.3 V translator | CLOCK |
| GP4 | Output through 3.3 V to 5 V translator | DATA |

Hold the DATA translator disabled during reset and provide a weak pull-up on
the console-facing DATA line so reset represents no buttons pressed. Do not
join console 5 V to system 5 V during this prototype test; connect grounds.

## Data flow

1. The input module latches and clocks eight NES bits approximately once per
   core poll.
2. Active-low wire data is converted to the active-high TASD button byte.
3. A changed state becomes a TASD `INPUT_MOMENT` in a versioned, CRC-protected
   64-byte SPI frame.
4. The core polls the input link at 1 kHz, validates and parses a new frame,
   prints each TASD packet, and forwards the frame to SPI1.
5. The output module validates the frame and caches its state. On each console
   latch it snapshots that state and shifts active-low DATA locally from PIO.

## Automated transport check

The desktop controller-pipeline test executes the production TASD codec and
CHIRBot link framing for both NES and SNES states. It verifies the input-side
document, core-style frame forwarding, output-side decoding, CRC rejection,
and controller input-size validation:

```powershell
cmake -S firmware/chirbot-common -B build/host-chirbot-common
cmake --build build/host-chirbot-common --config Debug
ctest --test-dir build/host-chirbot-common -C Debug --verbose
```

This check covers the digital transport contract. The bench test below remains
required to validate GPIO timing, level shifting, and console compatibility.

Example UART output after pressing A:

```text
CHIRBot core ready: SPI 2000000 Hz, UART stdio
[TASD seq=1] key=0x00f0 len=3 port=0 controller=0x0101
[TASD seq=1] key=0xfe02 len=12 port=0 hold=0 index_type=3 index=42 data=01 buttons=A
```

Releasing A produces another sequence with `data=00 buttons=none`. The test
passes when each press and release produces exactly one new input-moment line
with the expected button names and no repeated sequence while idle. With the
output module and console attached, the same presses must appear at the console
on its next controller poll.

## Prototype limits

- GPIO assignments are bench defaults, not the final dock connector pinout.
- There is no discovery, handshake, ready/IRQ, retry, or output acknowledgement.
- The output module retains the last valid state indefinitely.
- Console timing and level shifting require logic-analyzer and scope validation.
- Attach modules only while powered off.
