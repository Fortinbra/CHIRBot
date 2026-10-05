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

### Proposed TXS0108E replacement: unqualified bench option

The TXS0108E proposal is not an as-built update or approval to connect a console.
TI explicitly supports both push-pull and open-drain operation; automatic
direction sensing alone is not a reason to reject it. However, it is a
pass-gate translator with edge accelerators and internal pull-ups, not a
fixed-direction cable-driving buffer. The existing AHCT125/LVC125A acceptance
path remains the recommendation.

Use the [TI TXS0108E datasheet](https://www.ti.com/lit/ds/symlink/txs0108e.pdf)
(SCES642L, November 2024), especially sections 5.3, 7.3.1-7.3.5, and 8.3.
If evaluated, connect port A / VCCA to the module's 3.3 V domain and port B /
VCCB to the native 5 V domain. Use three separate channel pairs:

| Module | Signal | Source and translation | Assessment |
| --- | --- | --- | --- |
| Input | LATCH, CLOCK | Module GPIO drives A to B, 3.3 V to 5 V | Supported in principle; controller cable loading and edge quality must be qualified. |
| Input | DATA0 | Controller drives B to A, 5 V to 3.3 V; GPIO is input | Controller output levels and drive strength must meet TXS input requirements. |
| Output | LATCH, CLOCK | Console drives B to A, 5 V to 3.3 V; GPIOs are inputs | Console output levels and edge rates must meet TXS input requirements. |
| Output | DATA0 | Module GPIO drives A to B, 3.3 V to 5 V | Console load, cable, and reset-safe enable require qualification before console attachment. |

Each channel has exactly one external driver during normal operation. Mixed
directions across channels are permitted; there is no shared direction pin.
Do not enable GPIO outputs on the receiving sides.

**Critical electrical constraints:**

- VCCA is 1.4-3.6 V and VCCB is 1.65-5.5 V; VCCA must not exceed VCCB during
  operation. On the input module, VCCB uses regulated controller-side 5 V.
  On the output module, it uses console 5 V, not system 5 V. Keep grounds
  common and the two 5 V rails isolated.
- At nominal 3.3 V / 5 V, TI specifies data-input VIH of at least 2.9 V on A
  and 4.6 V on B, and VIL of at most 0.15 V on either port. These are
  near-rail requirements, not ordinary TTL thresholds. A controller or console
  output described only as "5 V logic" is not sufficient evidence; verify its
  guaranteed VOH/VOL under the actual load. This is a particular concern for
  older controller shift-register outputs and console drivers.
- Push-pull input transition rise/fall rate must not exceed 10 ns/V.
  TI's published timing/data-rate values assume external driver output
  impedance below 50 ohms. Slow NES polling does not relax these edge or
  driver requirements; a headline 110 Mbit/s rating does not qualify a cable.
- TI describes approximately 30 ns one-shot duration in this revision and
  requires reflection round-trip delay shorter than that duration. Excessive
  capacitance can outlast the accelerator; reflections can retrigger it,
  causing oscillation or extra CLOCK/LATCH edges. Include cable, wire-wrap,
  connectors, receiver inputs, and probes in the load assessment; do not
  infer an allowed cable length from the data-rate rating.
- Sustained high-level drive is modest (hundreds of microamps), unlike an
  AHCT output buffer. Internal pull-ups are nominally 4 kohms when high and
  40 kohms when low. Account for existing controller/console pulls and any
  added bias or series resistors; do not copy BSS138 pull-up assumptions.
- OE is active high and referenced to VCCA. Hold it low with a hardware
  pull-down during reset and supply transitions; enable only after both
  rails are stable and GPIO directions/defaults are safe. OE low disables
  all eight channel pairs and their internal pull-ups. An external
  console-side DATA pull-up is still needed for the disabled no-buttons state.
  Permanently tying OE to 3.3 V does not satisfy output reset safety.

**Breakout-specific evidence:**

| Candidate | Verified documentation | Qualification caveat |
| --- | --- | --- |
| SparkFun BOB-19626 | [Hookup guide](https://learn.sparkfun.com/tutorials/level-shifter---8-channel-txs0108e-hookup-guide/all), [schematic](https://cdn.sparkfun.com/assets/9/a/5/a/a/TXS0108_Breakout.pdf), and [hardware sources](https://github.com/sparkfun/8_channel_level_shifter_TXS0108E) identify the TXS0108E, expose OE, and document a default OE pull-down. | The guide says 110 kohms while the repository schematic labels R1 100 kohms; verify the actual board revision/value. SparkFun reports oscillation even with a 6-inch jumper during testing, so this is not a qualified NES cable interface. |
| Amazon TXS0108E breakout, unspecified | No specific product link, board revision, or schematic supplied. | No board-level voltage, speed, OE bias, decoupling, or component-authenticity claim is established. Identify and inspect the actual board before evaluation; the existing hiBCTR link above is for BSS138, not TXS0108E. |

Retain the as-built BSS138 record, GPIO/RJ45 mappings, fixed-direction buffer
recommendation, RP2350 5 V prohibition, power isolation, and reset-safe DATA
requirements. The blanket TXS/TXB rejection in the linked hardware documents
should be read as the project's conservative selection rule, not as a TI
claim that TXS0108E cannot handle push-pull signals; TXS and TXB are not
electrically interchangeable. Those documents are not changed by this review.

Before accepting any TXS bench substitution, scope both sides of all three
signals using the actual controller/console and cable. Check driven input
levels against the limits above, edge rate, receiver-level margins, settling
before DATA sampling, overshoot/undershoot, and absence of duplicate edges.
Use a current-limited console-equivalent fixture first and prove benign DATA
during reset, disconnect, and every partial-power sequence. A successful
button read or logic-analyzer decode alone is not electrical qualification,
and NES results do not establish SNES, PAL, or accessory compatibility.

### NES controller to input module

| RJ45 pin | Input GPIO | Direction | NES signal |
| ---: | ---: | --- | --- |
| 6 | GP2 | Output through 3.3 V to 5 V translator | PXLatch / LATCH |
| 8 | GP0 | Output through 3.3 V to 5 V translator | PXClock / CLOCK |
| 4 | GP4 | Input through 5 V to 3.3 V translator | PXD0 / DATA0 |
| 5 | NC | Not connected | PXd1 / DATA1 |

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

| RJ45 pin | Output GPIO | Direction | Console signal |
| ---: | ---: | --- | --- |
| 6 | GP2 | Input through 5 V to 3.3 V translator | PXLatch / LATCH |
| 8 | GP0 | Input through 5 V to 3.3 V translator | PXClock / CLOCK |
| 4 | GP4 | Output through 3.3 V to 5 V translator | PXD0 / DATA0 |
| 5 | NC | Not connected | PXd1 / DATA1 |

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
