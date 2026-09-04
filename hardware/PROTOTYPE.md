# Three-MCU architecture prototype

> **Status:** Planning document, not a production hardware specification.
> Product availability was researched on 2026-09-03 and should be checked
> again before ordering. Electrical and mechanical decisions discovered during
> this prototype must be captured in [specifications](../docs/specs/) or
> [design decisions](../docs/design/) before they become project requirements.

This prototype uses existing development hardware to exercise the intended
CHIRBot architecture with three independent RP2350B nodes:

```text
SNES controller
    |
    v
PGA2350 input module
    |
    | dedicated 3.3 V SPI link
    v
PGA2350 core + microSD + USB
    |
    | dedicated 3.3 V SPI link
    v
PGA2350 output module
    |
    v
SNES console
```

The first vertical slice targets SNES because its serial protocol and console
poll edge are well understood. NES uses a closely related protocol and can be
added after the three-node SNES path works. Prototype wiring stands in for the
eventual direct board-to-board docking interface.

## Prototype goals

- Validate independent input, core, and output firmware responsibilities
- Exercise two point-to-point SPI links rather than a multidrop bus
- Read a physical controller and emulate one to a physical console
- Record and replay TASD data through the same path
- Measure latency, jitter, electrical margins, and behavior during SD writes
- Test node reset, link recovery, and benign startup states
- Produce evidence for the module-interface, SPI, TASD, and clock-domain specs

Display hardware, an enclosure, hot-swap, generalized discovery, USB host
controllers, and the host GUI are intentionally outside this prototype.

## Primary purchase list

| Qty | Item | Vendor / identifier | Purpose |
|---:|---|---|---|
| 3 | [Pimoroni PGA2350](https://shop.pimoroni.com/products/pga2350) | Pimoroni `PIM722` | Input node, core, and output node |
| 1 | [MicroSD SPI/SDIO breakout](https://www.adafruit.com/product/4682) | Adafruit `4682` | Native 3.3 V storage on the core |
| 1 | [USB-C data breakout](https://www.adafruit.com/product/4090) | Adafruit `4090` | Core USB data and VBUS connection; includes CC resistors |
| 1 | [USB-A to USB-C cable](https://www.adafruit.com/product/4474) | Adafruit `4474` | Core data connection; substitute to match the host port |
| 1 | [8-32 GB microSDHC card](https://www.digikey.com/en/products/filter/memory-cards/501) | DigiKey search | Select a reputable, currently orderable card |
| 1 | [Raspberry Pi Debug Probe](https://www.adafruit.com/product/5699) | Adafruit `5699` | SWD and UART debugging |
| 1 | [SparkFun 8-channel logic analyzer](https://www.digikey.com/en/products/detail/sparkfun-electronics/18627/15842546) | SparkFun `TOL-18627`; DigiKey `1568-18627-ND` | Capture one native interface or SPI link at a time |
| 2 | [SNES controller extension cable](https://console5.com/store/snes-controller-extension-cable-6-foot-1-8m.html) | Console5 | Female controller receptacle, male console plug, and one spare |

The PGA2350 includes 16 MB QSPI flash and 8 MB PSRAM but no USB connector,
headers, buttons, or status LED. PSRAM uses GP47 through a cuttable chip-select
trace. The board accepts power at `VB`, while all GPIO remains 3.3 V and is not
5 V tolerant.

## Native-interface translation

Buy one spare of each translator. Two of each are installed: one on the input
node and one on the output node.

| Qty | Part | Vendor / identifier | Role |
|---:|---|---|---|
| 3 | [SN74AHCT125N](https://www.digikey.com/en/products/detail/texas-instruments/SN74AHCT125N/375798) | TI `SN74AHCT125N`; DigiKey `296-4655-5-ND` | PDIP buffer powered at 5 V; accepts 3.3 V logic-high inputs |
| 3 | [SN74LVC125AD](https://www.digikey.com/en/products/detail/texas-instruments/SN74LVC125AD/377409) | TI `SN74LVC125AD`; DigiKey `296-8453-5-ND` | SOIC buffer powered at 3.3 V with 5 V-tolerant inputs |
| 1 pack | [SOIC-14 breakout PCBs](https://www.adafruit.com/product/1210) | Adafruit `1210` | Adapt the LVC125AD for hand-wired prototyping |

Signal assignments:

| Node | 3.3 V to 5 V through AHCT125 | 5 V to 3.3 V through LVC125 |
|---|---|---|
| Input module | Controller `LATCH`, `CLOCK` | Controller `DATA` |
| Output module | Emulated-controller `DATA` | Console `LATCH`, `CLOCK` |

Use fixed-direction translators. Generic BSS138 I2C level-shifter modules and
TXS/TXB automatic-direction translators are not appropriate for these clocked
push-pull signals. Tie every active-low output enable to a deliberate state,
disable unused gates, and give every unused input a defined level.

Standard NES/SNES `DATA` is driven push-pull. Open-collector details for PAL
hardware and accessory pins must be investigated separately before claiming
broad compatibility.

## Carrier and interconnect hardware

| Qty | Item | Vendor / identifier | Purpose |
|---:|---|---|---|
| 1 pack | [Isolated-pad perfboard](https://www.adafruit.com/product/2670) | Adafruit `2670` | Three carrier boards |
| 2 packs | [36-pin Swiss male headers](https://www.adafruit.com/product/3647) | Adafruit `3647` | PGA2350 carrier contacts |
| 2 packs | [36-pin Swiss female sockets](https://www.adafruit.com/product/3646) | Adafruit `3646` | Removable PGA2350 mounting |
| 1 pack | [6 mm tactile switches](https://www.adafruit.com/product/367) | Adafruit `367` | RUN/reset and BOOTSEL controls |
| 2 | [20-way female/female jumper ribbon](https://www.adafruit.com/product/1950) | Adafruit `1950` | Temporary point-to-point SPI harnesses |
| 1 set | [22 AWG solid-core hookup wire](https://www.adafruit.com/product/1311) | Adafruit `1311` | Carrier wiring and power distribution |

PGA2350 is a 25.4 mm square, 64-position module on a 2.54 mm grid, not a
standard two-row Pico module. Dry-fit one complete socket and verify underside
component clearance before soldering the other carriers. Use isolated-pad
perfboard rather than a breadboard-pattern board with internally connected
groups.

Expose the following on every carrier:

- `VB`, `3V3`, and multiple grounds
- `RUN` and BOOTSEL controls
- SWDIO, SWCLK, and ground
- UART TX and RX
- Assigned SPI signals and a ready/IRQ GPIO
- Native protocol signals on the input and output carriers

Keep each SPI harness short and provide multiple ground returns. These
harnesses are development fixtures, not the final module connector.

## Passives and protection

Quantities include development spares.

| Qty | Part | Vendor / identifier | Use |
|---:|---|---|---|
| 20 | [100 nF ceramic capacitor](https://www.digikey.com/en/products/detail/kemet/C315C104M5U5TA/817927) | KEMET `C315C104M5U5TA` | Local IC bypassing |
| 10 | [100 uF, 10 V electrolytic capacitor](https://www.digikey.com/en/products/detail/panasonic-industry/EEU-FR1A101B/2504105) | Panasonic `EEU-FR1A101B`; DigiKey `P15316CT-ND` | Bulk capacitance at nodes and SD card |
| 20 | [10 kohm resistor](https://www.digikey.com/en/products/detail/yageo/CFR-25JB-52-10K/338) | Yageo `CFR-25JB-52-10K` | Enable and default-state pulls |
| 20 | [33 ohm resistor](https://www.digikey.com/en/products/detail/yageo/CFR-25JB-52-33R/1687) | Yageo `CFR-25JB-52-33R`; DigiKey `33QBK-ND` | Optional source-series damping |
| 5 | [500 mA resettable fuse](https://www.digikey.com/en/products/detail/bourns-inc/MF-R050/259965) | Bourns `MF-R050` | Separately protect each node branch |
| 2 | [1.1 A resettable fuse](https://www.digikey.com/en/products/detail/bourns-inc/MF-R110/259970) | Bourns `MF-R110` | Main prototype input protection and spare |

Place one 100 nF capacitor directly at each translator supply pair, at least
one 100 uF capacitor near each PGA2350 power input, and another bulk capacitor
near the microSD breakout. Provide unpopulated 33 ohm footprints near SPI and
native clock drivers, then select values from scope measurements rather than
assuming every line needs damping.

External controller and console connectors should eventually receive suitable
ESD protection. Select those parts after the native-interface voltage and
capacitance limits are documented.

## Power topology

Do not assume an ordinary PC USB port can power three PGA2350 boards, microSD
writes, translators, and a controller reliably. Start with a regulated,
current-limited 5 V bench supply:

```text
regulated 5 V
    |
 main resettable fuse
    |
    +-- branch fuse --> core VB
    +-- branch fuse --> input VB + controller 5 V
    +-- branch fuse --> output VB
```

Connect USB `D+`, `D-`, and ground to the core. Do not parallel USB VBUS with
the external supply. Do not join PC USB VBUS, bench 5 V, and console 5 V, and
never apply 5 V to PGA2350 GPIO or `3V3`. Treat console 5 V as a separately
sensed interface rail, not as another source for the prototype.

The PGA2350 onboard 3.3 V regulator is rated at 300 mA. That budget includes
the MCU, PSRAM, and all loads attached to the board's 3.3 V output. Measure
node and total current before connecting a console.

Until the module-interface specification says otherwise, all module wiring is
power-off-only. Confirm that translator enables default benignly and that no
node is back-powered through a signal pin before joining the three nodes.

## Core bus allocation

The core has three independent high-traffic relationships:

1. Input-module SPI
2. Output-module SPI
3. microSD SPI

RP2350 provides two hardware SPI controllers. Assign two relationships to
hardware SPI and implement the third with PIO, based on measured timing and
firmware complexity. Sharing an SPI peripheral with independent chip selects
is acceptable for initial bring-up, but the final prototype should isolate SD
traffic from real-time module traffic so card latency or a stuck MISO cannot
disturb relay behavior.

## Assembly sequence

1. Build one PGA2350 carrier and verify socket fit, power, RUN, BOOTSEL, SWD,
   UART, and USB before duplicating it.
2. Build and power each remaining carrier independently with a current limit.
3. Add the core microSD breakout and test initialization, sustained writes,
   full-card behavior, removal, and supply droop.
4. Add input-node translation and verify controller reads with a logic analyzer.
5. Add output-node translation and test it first against a logic generator or
   current-limited fixture, not a console.
6. Bring up one SPI link at a conservative rate, including error counters and
   reset recovery.
7. Bring up the second independent SPI link and repeat fault testing.
8. Connect the complete path, record a known sequence, replay it, and compare
   native input, stored events, and console-facing output captures.
9. Measure worst-case latency and jitter during SD writes.
10. Repeat on additional SNES and NES hardware before broad compatibility
    claims.

## Bring-up gates

Do not connect a real console until all of the following are true:

- Harvested connector pins have been mapped by continuity, independent of
  cable colors
- USB 5 V, console 5 V, system 5 V, 3.3 V, and GPIO domains have been checked
  for unintended continuity
- Each carrier powers and resets independently without excessive current
- SWD access and UART logs work on all three nodes
- Translator output enables are benign during reset and partial-power states
- Output `DATA` voltage and timing are correct and cannot contend with the
  console
- SPI links recover from node reset and disconnected harnesses
- SD current transients do not reset nodes or corrupt link traffic

## Sources

- [Pimoroni PGA2350 product page](https://shop.pimoroni.com/products/pga2350)
- [PGA2350 schematic](https://cdn.shopify.com/s/files/1/0174/1800/files/Pimoroni_PGA2350_Schematic.pdf?v=1724926880)
- [PGA2350 pinout](https://cdn.shopify.com/s/files/1/0174/1800/files/pga2350_pinout_diagram.pdf?v=1723124465)
- [RP2350 documentation](https://www.raspberrypi.com/documentation/microcontrollers/microcontroller-chips.html)
- [RP2350 datasheet](https://pip.raspberrypi.com/documents/RP-008373-DS-rp2350-datasheet.pdf)
- [SN74AHCT125 product page](https://www.ti.com/product/SN74AHCT125)
- [SN74LVC125A product page](https://www.ti.com/product/SN74LVC125A)
