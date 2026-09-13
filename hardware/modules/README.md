# NES input and output module hardware

> **Status:** The NES input fixture is assembled on wire-wrap protoboard.
> Breakout models and exact native-interface channel mapping still require
> verification. The output fixture, dedicated PCBs, production docking
> connectors, and physical-console validation are not complete. Cost target:
> less than $40 per module.

This document covers the input and output modules used by the three-board NES
vertical slice. Each module uses a Pimoroni PGA2350 as an SPI subnode. The
assembled input fixture uses a hiBCTR BSS138-based bidirectional level-shifter;
the target output design uses fixed-direction 3.3 V/5 V buffers.

The pin assignments below are prototype assignments, not the final CHIRBot
dock pinout. Temporary point-to-point harnesses stand in for the board-to-board
carrier slots that remain open in the
[module-interface specification](../../docs/specs/module-interface.md).

The current input firmware supports a standard NES controller. The output
firmware supports standard NES and SNES TASD states, but this document only
specifies the wired NES prototype. Accessories and additional controller data
lines are outside the current scope.

## Input module pinout

The input module drives the controller latch and clock and samples its
active-low serial data. The SPI link remains entirely at 3.3 V.

| PGA2350 pin | Function | Direction at module | As-built interface |
| ---: | --- | --- | --- |
| GP2 | NES LATCH | Output | hiBCTR BSS138 shifter; physical channel and pull-ups TBD |
| GP3 | NES CLOCK | Output | hiBCTR BSS138 shifter; physical channel and pull-ups TBD |
| GP4 | NES DATA0 | Input | hiBCTR BSS138 shifter; physical channel and pull-ups TBD |
| GP16 | SPI0 MISO | Output | Direct 3.3 V to SPIDriver or future core GP16 |
| GP17 | SPI0 CSn | Input | Direct 3.3 V from SPIDriver or future core GP17 |
| GP18 | SPI0 SCLK | Input | Direct 3.3 V from SPIDriver or future core GP18 |
| GP19 | SPI0 MOSI | Input | Direct 3.3 V from SPIDriver or future core GP19 |
| VB / USB | Module supply | Power input | USB-C breakout wiring TBD and must be continuity-verified |
| 3V3 | Shifter low-side rail | Power output | Never apply an external voltage to this pin |
| GND | Common reference | Power | Controller, SPIDriver, and module ground |

Firmware sources of truth:

- [NES GPIO assignments](../../firmware/modules/input-nes-snes/src/nes_protocol.cpp)
- [Input SPI assignments](../../firmware/modules/input-nes-snes/src/spi_subnode.cpp)

## Output module pinout

The output module samples latch and clock from the console and drives emulated,
active-low controller data. The PIO program addresses CLOCK relative to LATCH,
so GP3 must immediately follow GP2 unless the PIO configuration is changed.

| PGA2350 pin | Function | Direction at module | Electrical interface |
| ---: | --- | --- | --- |
| GP2 | Console LATCH | Input | 5 V to 3.3 V through LVC125A |
| GP3 | Console CLOCK | Input | 5 V to 3.3 V through LVC125A |
| GP4 | Emulated DATA0 | Output | 3.3 V to 5 V through AHCT125 |
| GP16 | SPI0 MOSI | Input | Direct 3.3 V from core GP15 |
| GP17 | SPI0 CSn | Input | Direct 3.3 V from core GP13 |
| GP18 | SPI0 SCLK | Input | Direct 3.3 V from core GP14 |
| GP19 | SPI0 MISO | Output | Direct 3.3 V to core GP12 |
| VB | System supply | Power input | Regulated 5 V through branch fuse |
| 3V3 | Translator supply | Power output | LVC125A only |
| GND | Common reference | Power | Console, core, and system ground |

Firmware source of truth:

- [Output GPIO and SPI assignments](../../firmware/modules/output-nes-snes/include/output_module_config.hpp)
- [Console PIO implementation](../../firmware/modules/output-nes-snes/src/controller_output.pio)

## Core SPI harnesses

SPI uses mode 0 at 2 MHz with one fixed 64-byte transaction per poll. The
prototype has no ready or interrupt signal and therefore does not satisfy the
production input-dock contract. Production input modules require a dedicated,
level-sensitive data-ready signal to the core; its final contact and GPIO are
pending in the [module-interface specification](../../docs/specs/module-interface.md).
Production modules share SPI signals with the other slots in their carrier
bank and therefore must keep MISO high-impedance whenever their CS is inactive.
Keep both prototype harnesses short and use at least one ground return beside
the clock/data group.

### Input module to core

The input link uses matching GPIO numbers on the two PGA2350 boards.

| Core PGA2350 | Input PGA2350 | Signal | Direction |
| ---: | ---: | --- | --- |
| GP18 | GP18 | SCLK | Core to input |
| GP19 | GP19 | MOSI | Core to input |
| GP16 | GP16 | MISO | Input to core |
| GP17 | GP17 | CSn | Core to input |
| GND | GND | Ground | Common reference |

### Core to output module

The output link crosses GPIO numbers because the core uses SPI1 and the module
uses SPI0.

| Core PGA2350 | Output PGA2350 | Signal | Direction |
| ---: | ---: | --- | --- |
| GP14 | GP18 | SCLK | Core to output |
| GP15 | GP16 | MOSI | Core to output |
| GP12 | GP19 | MISO | Output to core |
| GP13 | GP17 | CSn | Core to output |
| GND | GND | Ground | Common reference |

Do not carry console 5 V on either SPI harness. System power distribution is
described in [hardware/PROTOTYPE.md](../PROTOTYPE.md).

## Native NES connectors

The input module needs a female NES controller receptacle. The output module
needs a male NES controller plug. A continuity-tested NES extension lead may be
cut to provide both ends.

Connect these signals:

| NES signal | Input module | Output module |
| --- | --- | --- |
| +5 V | System 5 V after input branch fuse | Console 5 V interface rail only |
| GND | System common ground | Console/system common ground |
| LATCH / OUT0 | AHCT125 output from GP2 | LVC125A input to GP2 |
| CLOCK | AHCT125 output from GP3 | LVC125A input to GP3 |
| DATA0 | LVC125A output to GP4 | AHCT125 output from GP4 |
| DATA1 | Not connected | Not connected |
| DATA2 | Not connected | Not connected |

Do not assign contact numbers from cable colors or an unverified online
drawing. Before assembly, map each harvested connector by continuity and record
whether the view is into the receptacle, into the plug, or from its solder
side. The unused DATA1 and DATA2 contacts must remain isolated.

## As-built input carrier and BOM

The current input fixture is a build record, not a manufacturer-qualified BOM.
Unknown model numbers and mappings are intentionally marked `TBD` rather than
inferred from appearance.

| Qty | As-built item | Identification and notes |
| ---: | --- | --- |
| 1 | PGA2350 | Pimoroni `PIM722` |
| 1 set | Extra-tall pin headers | Manufacturer and height TBD |
| 1 | Large protoboard | Manufacturer and dimensions TBD |
| As needed | Wire-wrap wire | Gauge, insulation, and color mapping TBD |
| 1 | RJ45/8P8C breakout | MPN and contact orientation TBD; NES cable adapter only |
| 1 | Existing NES cable | RJ45 contact-to-NES-signal map must be recorded by continuity |
| 1 | [hiBCTR 4-channel bidirectional level shifter](https://www.amazon.com/dp/B0DSZ5KFKL) | Seller part `H1-HBB0064-20`, ASIN `B0DSZ5KFKL`; BSS138 per seller; pull-ups and channel map TBD |
| 1 | USB-C breakout board | MPN, CC resistors, and connected pins TBD |

The Excamera Labs SPIDriver is external test equipment and is not part of the
module BOM. See the
[NES acceptance scenario](../../docs/specs/nes-to-uart-scenario.md) for its
temporary SPI hookup.

### Input-fixture electrical qualification

The installed BSS138 bidirectional shifter restores a translated high level
through a pull-up rather than actively driving it across the translator.
Unknown pull-up values and wire-wrap capacitance can slow rising edges, distort
CLOCK duty cycle, and reduce setup time. The seller claims SPI compatibility
but publishes no SPI frequency, load, or rise-time limit. Before relying on
this fixture:

- Connect shifter `LV` to PGA2350 `3V3`, `HV` to regulated controller-side
  5 V, and `GND` to the common controller/PGA2350 ground.
- Verify the installed BSS138 devices and document resistor markings, measured
  pull-up values, physical channel labels, and LATCH/CLOCK/DATA channel mapping.
- Scope both sides of LATCH, CLOCK, and DATA under load.
- Verify valid levels and rise time before every sampling edge.
- Check reset, controller disconnect, USB disconnect, and one-side-unpowered
  conditions for contention or back-powering.
- Never configure both endpoints of one shifter channel as push-pull outputs.
- Never connect `HV`, 5 V, or a SPIDriver power output to PGA2350 `3V3`.

## Target voltage translation and power

Use fixed-direction buffers for the native clocked push-pull signals:

- TI `SN74AHCT125N`, powered from the applicable 5 V interface rail, translates
  a 3.3 V module output to 5 V.
- TI `SN74LVC125AD`, powered from the PGA2350 3.3 V rail, accepts a 5 V input
  and produces a 3.3 V module input.
- Do not substitute BSS138 I2C boards or TXS/TXB automatic-direction parts.
- Give every active-low output-enable pin a defined state and tie every unused
  buffer input to a defined level.

The system and native-interface domains are:

| Domain | Loads and restrictions |
| --- | --- |
| System 5 V | PGA2350 VB, input AHCT125, and NES controller power |
| PGA2350 3.3 V | RP2350 GPIO and both LVC125A devices |
| Console 5 V | Output AHCT125 and native console interface only |
| Ground | Common reference between all three boards and console |

Never join console 5 V to system 5 V or USB VBUS. USB data may be connected to
the core while using a bench supply, but its VBUS must remain isolated. The
PGA2350 GPIO and 3V3 pins are not 5 V tolerant.

### Output reset-state requirement

The console-facing DATA buffer must remain disabled during module reset and
partial-power conditions, with a weak console-side pull-up representing no
buttons pressed. The current output firmware does not allocate a GPIO for the
AHCT125 output-enable signal. Before attaching a console, either:

1. Allocate and implement a dedicated output-enable GPIO with a hardware-safe
   default, or
2. Validate a passive enable circuit that remains benign for every power-up,
   reset, disconnect, and partial-power sequence.

This is a blocking hardware bring-up requirement, not an optional enhancement.

## Target module BOM

The following quantities describe the intended fixed-buffer input/output
prototype, not the assembled input fixture. Purchase development spares and
recheck stock before ordering; pricing and availability are not normative.

### Common parts

| Qty per module | Part | Manufacturer / identifier | Purpose |
| ---: | --- | --- | --- |
| 1 | PGA2350 development module | Pimoroni `PIM722` | RP2350B, flash, PSRAM, and 3.3 V regulator |
| 1 | Quad AHCT buffer, PDIP-14 | TI `SN74AHCT125N` | 3.3 V to 5 V translation |
| 1 | Quad LVC buffer, SOIC-14 | TI `SN74LVC125AD` | 5 V-tolerant input to 3.3 V translation |
| 1 | SOIC-14 breakout | Adafruit `1210` | Hand-wired LVC125A adapter |
| 2 | 100 nF ceramic capacitor | KEMET `C315C104M5U5TA` | One at each translator supply pair |
| 1 | 100 uF, 10 V electrolytic | Panasonic `EEU-FR1A101B` | Bulk capacitance near PGA2350 VB |
| 4 | 10 kohm resistor | Yageo `CFR-25JB-52-10K` | OE and signal default-state pulls |
| 3 DNP | 33 ohm resistor | Yageo `CFR-25JB-52-33R` | Optional source damping after measurement |
| 1 | 500 mA resettable fuse | Bourns `MF-R050` | Module power-branch protection |
| 1 | Isolated-pad perfboard section | Adafruit `2670` or equivalent | Prototype carrier |
| 1 set | PGA2350 socket headers | 2.54 mm Swiss-pin headers/sockets | Removable module mounting |
| 1 | SPI jumper harness | Short 2.54 mm leads plus ground returns | Temporary core connection |

`DNP` means do not populate until measurements show that damping is needed.
One 100 nF capacitor belongs directly at each translator package; it is not a
substitute for the bulk capacitor at VB.

### Input-specific parts

| Qty | Part | Notes |
| ---: | --- | --- |
| 1 | Female NES controller receptacle | Use a continuity-tested harvested lead |
| 1 | Weak DATA input bias resistor, value TBD | Fit only after validating controller disconnect behavior |

Two AHCT125 channels drive LATCH and CLOCK. One LVC125A channel receives DATA0.
Disable and terminate the unused channels according to the device datasheets.

### Output-specific parts

| Qty | Part | Notes |
| ---: | --- | --- |
| 1 | Male NES controller plug | Use a continuity-tested harvested lead |
| 1 | Weak console-side DATA pull-up, value TBD | Establishes no-buttons state while output is disabled |
| 1 | DATA output-enable network, design TBD | Required before console attachment |

One AHCT125 channel drives DATA0. Two LVC125A channels receive LATCH and CLOCK.
Disable and terminate the unused channels according to the device datasheets.

The core BOM is intentionally not duplicated here. See
[hardware/chirbot-core-hw/README.md](../chirbot-core-hw/README.md) for core
ownership and [hardware/PROTOTYPE.md](../PROTOTYPE.md) for shared bench tools,
carrier parts, power distribution, and development spares.

## Bring-up checks

Do not connect a real console until all of these checks pass:

- Verify harvested connector orientation and every contact by continuity.
- Confirm no continuity among system 5 V, console 5 V, USB VBUS, 3.3 V, and
  GPIO except at documented paths.
- Power each module independently from a current-limited 5 V supply.
- Verify all translator enables and unused inputs have defined reset states.
- Confirm 3.3 V and 5 V logic levels with an oscilloscope.
- Verify the input controller waveform and both SPI links with a logic analyzer.
- Test the output first with a current-limited logic fixture, not a console.
- Prove DATA remains high-impedance or benign-high during reset, disconnect,
  and every partial-power sequence.
- Connect and disconnect modules only while all participating hardware is off.

The complete firmware and bench acceptance procedure is in the
[NES-to-UART scenario](../../docs/specs/nes-to-uart-scenario.md).
