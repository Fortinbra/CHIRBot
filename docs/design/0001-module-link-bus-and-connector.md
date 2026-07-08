# 0001 — Module Link: Bus Evaluation and Connector Alternatives

- **Status:** Accepted (2026-07-08) — SPI confirmed as the module link bus;
  cabled connectors superseded by **board-to-board docking** (see Addendum).
  Specific connector family selection pending.
- **Date:** 2026-07-07 (evaluation), 2026-07-08 (decision)
- **Relates to:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md) §2.3 (decisions 2 and 3), open questions §5

## Context

The initial CHIRBot design uses SPI as the core↔module transport ("SPI matrix")
carried over USB-C connectors repurposed via Debug Accessory Mode (DAM). Two
concerns prompted this evaluation:

1. Is SPI actually the best bus for the core↔module link, versus I2C or other
   options?
2. USB connectors should be reserved for actual USB. Repurposing USB-C invites
   users to plug modules into chargers/PCs (and USB devices into module ports),
   and DAM-safe behavior adds design complexity. What connector should carry
   the module link instead?

### Requirements derived from the architecture

| Requirement | Value | Source |
|---|---|---|
| End-to-end relay latency | ~1 ms total (input capture → output presentation) | ARCHITECTURE.md §2.3 |
| Link topology | Point-to-point: core↔input module, core↔output module (2 links, possibly a vis link) | ARCHITECTURE.md §2.2 |
| Payload per poll | Small TASD packets; assume ≤ 64 bytes/poll for budgeting (**assumption — to be validated against TASD packet sizes**) | tasd.io |
| Cable length | Short external cable or direct attach, ≤ ~1 m assumed |
| Module power | Delivered over same connector (budget TBD) | ARCHITECTURE.md §5 |
| Cost | Counts against < $40/module and < $100/core |
| Duplex | Bidirectional: input data upstream, config/handshake both ways; output modules also need rumble/response data downstream |

A useful observation: the "matrix" is a **routing function inside the core
firmware**, not an electrical multi-drop bus. Electrically there are only two
or three point-to-point links. This weakens the case for shared-bus protocols
(I2C, CAN) whose main advantage is multi-drop addressing.

## Bus options

### Throughput/latency budget (64-byte packet, per link)

| Bus | Rate | Time for 64 B | Fits ~1 ms budget? |
|---|---|---|---|
| SPI @ 8 MHz | 8 Mbit/s | ~64 µs | Yes, ~6 % of budget |
| SPI @ 2 MHz (conservative over cable) | 2 Mbit/s | ~256 µs | Yes |
| UART 8N1 @ 3 Mbaud | 3 Mbaud (10 bits/byte) | ~213 µs | Yes |
| I2C Fast-mode | 400 kbit/s (9 bits/byte + addressing) | ~1.5 ms | **No — exceeds entire budget** |
| I2C Fast-mode Plus | 1 Mbit/s | ~590 µs | Marginal — over half the budget on the link alone |
| CAN 2.0B @ 1 Mbit/s | 8-byte data frames, ~110+ bits/frame | > 1 ms (needs ≥ 8 frames) | **No** |
| USB FS (core as host to modules) | 12 Mbit/s, 1 ms frame period | ≥ 1 ms scheduling granularity | **No — frame period consumes the budget** |

Sources: I2C mode rates and 9-bits-per-byte framing per the I2C specification
(NXP [UM10204](https://www.nxp.com/docs/en/user-guide/UM10204.pdf); summary:
[Wikipedia: I2C](https://en.wikipedia.org/wiki/I%C2%B2C)); CAN frame format and
1 Mbit/s ceiling per [can2040](https://github.com/KevinOConnor/can2040) (CAN
2.0B software implementation for RP2040/RP2350).

### Qualitative comparison

| Criterion | SPI | I2C | UART | CAN (can2040 + transceiver) | RS-422/485 (differential UART/SPI) |
|---|---|---|---|---|---|
| Speed headroom | Excellent (MHz-class) | Poor–marginal | Good (multi-Mbaud on RP2040 UART) | Poor (1 Mbit/s cap) | Good |
| Full duplex | Yes | No (half-duplex) | Yes | No | Yes (RS-422) |
| Wires (excl. power) | 4–5 (SCLK, MOSI, MISO, CS, optional IRQ) | 2 | 2 (TX/RX) | 2 + transceivers | 4 (2 pairs) + transceivers |
| Robustness over ~1 m cable | Good at moderate clocks; single-ended, needs ground integrity | **Poor**: open-drain, 400 pF total bus capacitance limit, rise-time sensitive | Good; self-clocked per byte, tolerant of skew | Excellent (differential) | Excellent (differential) |
| Fault modes | CS deselect recovers subnode | A stuck device can hang the shared bus (SDA/SCL held low) | Framing errors self-recover | Built-in arbitration/CRC | Like UART/SPI |
| Extra BOM | None (native PL022 blocks) | Pull-ups; bus buffers (e.g., PCA9605) if cabled | None (native UART or PIO) | 2× CAN transceivers per link | 2–4× RS-422 transceivers per link |
| Pico SDK support | Native (`hardware_spi`) | Native (`hardware_i2c`) | Native (`hardware_uart`), PIO for higher rates | Third-party, **GPL-3.0** (license risk for firmware) | Native UART + transceiver |
| Hardware caveat | RP2040/RP2350 SPI is ARM PL022; subnode (slave) max clock is SSPCLK/12 — verify timing budget against the [PL022 TRM](https://developer.arm.com/documentation/ddi0194/h) and [RP2040 datasheet](https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf) | Capacitance limit effectively rules out connectorized cables | Needs a framing layer (TASD already provides packetization) | GPL-3.0 incompatible with permissive firmware licensing | Transceiver cost/board space vs. ≤ 1 m need |

### Findings — bus

1. **I2C is not viable** for this link: Fast-mode cannot move one 64-byte
   packet within the 1 ms budget; Fast-mode Plus is marginal; and the
   open-drain/400 pF electrical model is explicitly designed for
   same-board use, not connectorized cables
   ([I2C spec limits](https://en.wikipedia.org/wiki/I%C2%B2C)). Its multi-drop
   advantage is irrelevant to point-to-point links.
2. **SPI remains a sound default.** Enormous latency headroom (~6 % of budget
   at 8 MHz), zero added BOM, native Pico SDK support. Verify the PL022 slave
   clock-ratio limit (SSPCLK/12) and cable signal integrity during prototyping;
   at 2–8 MHz over ≤ 1 m shielded cable with good grounding this is well-trodden
   territory.
3. **Full-duplex UART is the strongest alternative**, not I2C: 2 signal wires
   instead of 4–5, self-framing tolerance to clock skew, multi-Mbaud rates, and
   PIO can push it further. It gives up SPI's raw headroom and the natural
   main/subnode CS semantics but simplifies the connector pinout.
4. **CAN and USB-between-boards are rejected** (throughput/latency; can2040's
   GPL-3.0 license is also a problem for permissively-licensed firmware).
5. **Differential signaling (RS-422 class) is unnecessary at ≤ 1 m** but is the
   documented escalation path if cable-length ambitions grow.

**Recommendation:** keep SPI as the baseline; prototype the link over the real
connector/cable and measure. If pin count on the chosen connector gets tight,
full-duplex UART at ≥ 2 Mbaud is an acceptable substitute with the same TASD
packet framing.

## Connector alternatives to USB-C DAM

> **Superseded (2026-07-08):** this section evaluated *cabled* connectors. The
> decision moved to cable-free board-to-board docking — see the Addendum. The
> analysis is retained because it informed the decision (the mis-plug hazards
> below are a large part of why cables were dropped).

Constraint: USB connectors are reserved for actual USB. The replacement must
carry 4–7 signals + module power + ground, survive consumer handling
(latching, reasonable cycle life), use off-the-shelf cables where possible, and
minimize mis-plug hazards.

| Option | Contacts | Latching | Off-the-shelf cables | Mis-plug hazard | Notes |
|---|---|---|---|---|---|
| **RJ45 / 8P8C (shielded)** | 8 + shield | Yes | Ubiquitous (Cat5e/6, incl. shielded) | Looks like Ethernet; plugging into a live Ethernet port is usually benign (Ethernet is transformer-coupled), but **PoE ports can present ~48 V** — needs protection + labeling | Best fit: 8 pins covers SPI (5) + 5 V + GND ×2. Cheap jacks, panel- or PCB-mount |
| 6P6C (RJ12) | 6 | Yes | Common | Looks like phone/RJ11; legacy phone lines carry ring voltage | Enough for UART link (TX, RX, IRQ, 5 V, GND ×2); tight for SPI. Precedent: LEGO NXT used 6P6C for I2C-class links |
| Mini-DIN 6/8 | 6–8 | Friction | PS/2 cables (dwindling supply) | Looks like PS/2 — and CHIRBot has real PS/2 modules; confusing | On-theme but declining availability; higher cost |
| HDMI (repurposed) | 19 | Friction | Ubiquitous, shielded, cheap | High — users will plug modules into TVs; HDMI sources supply +5 V and CEC/DDC signals | Same "repurposed connector" objection as USB-C DAM; rejected on principle |
| D-sub DE-9 | 9 | Screw | Common (straight-through serial) | Low (RS-232 legacy gear is rare now); pins can bend | Bulky but rugged and period-appropriate; genderable to prevent in/out swap |
| Circular GX12/M8-class | 4–8 | Screw | Vendor-specific | Very low (distinctive) | Robust, but cables are not off-the-shelf commodity items; higher cost per mated pair |
| Shrouded IDC 2×5 header | 10 | Friction/latch variants | Ribbon (self-made) | Low | Cheapest, keyed; poor as an external consumer-facing connector |

*Prices deliberately omitted pending BOM-stage quotes from
[DigiKey](https://www.digikey.com/)/[Mouser](https://www.mouser.com/)/[LCSC](https://www.lcsc.com/)
— to be filled in with real part numbers when the connector decision is made.*

### Findings — connector

1. **Shielded RJ45 (8P8C) is the leading candidate.** Exactly enough contacts
   for the full SPI link plus power (SCLK, MOSI, MISO, CS, IRQ, 5 V, 2× GND),
   plus shield. Latching, keyed, high cycle life, and shielded patch cables
   of any length are commodity items — a genuine advantage for an open
   project where users source their own cables.
2. **The mis-plug story is manageable and must be designed in:** an Ethernet
   port is transformer-isolated (no DC damage path), but PoE can inject ~48 V.
   The module-link pins therefore need input protection (TVS, series
   resistance, polyfuse on the 5 V rail) and unambiguous labeling. This is
   simpler than implementing USB-C DAM correctly (CC logic, ensuring safe
   behavior with chargers and normal USB devices).
3. **In/out port confusion** (input module plugged into output port) is not
   electrically hazardous if both ports share a pinout and the handshake
   protocol identifies module direction — a firmware/spec concern for
   [docs/specs/module-interface.md](../specs/module-interface.md).
4. If the bus decision moves to UART (2 signals), **6P6C** becomes viable and
   even cheaper, at the cost of the phone-line lookalike hazard and less
   shielding.
5. HDMI repurposing is rejected for the same reason USB-C DAM is being
   reconsidered; Mini-DIN is rejected because CHIRBot ships actual PS/2
   modules and identical connectors with different electrical roles is a
   user hazard.

## Risks and open questions

- [ ] Validate the 64-byte-per-poll payload assumption against real TASD
      packet sizes (including future rumble/motion packets in the return path).
- [ ] Prototype SPI at 2–8 MHz over ~1 m shielded Cat5e via RJ45 and measure
      signal integrity and error rates (PL022 slave clock-ratio limit:
      verify against RP2040/RP2350 datasheets).
- [ ] Define the PoE-misplug protection circuit and test against an actual
      802.3af/at injector.
- [ ] Module power budget over the connector (how many mA at 5 V per module?)
      — drives contact current rating verification.
- [ ] Confirm connector part selection with real distributor pricing/stock at
      BOM stage (multi-source jacks only).
- [ ] Update ARCHITECTURE.md §2.3 decisions 2–3 and the module-interface spec
      once a decision is confirmed.

## References

- I2C specification (NXP UM10204): <https://www.nxp.com/docs/en/user-guide/UM10204.pdf>
- I2C overview, modes, capacitance limit: <https://en.wikipedia.org/wiki/I%C2%B2C>
- can2040 — software CAN for RP2040/RP2350 (GPL-3.0): <https://github.com/KevinOConnor/can2040>
- ARM PL022 SSP Technical Reference Manual: <https://developer.arm.com/documentation/ddi0194/h>
- RP2040 datasheet: <https://datasheets.raspberrypi.com/rp2040/rp2040-datasheet.pdf>
- RP2350 datasheet: <https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf>
- NXP AN11075 — driving I2C over twisted-pair (what it takes to cable I2C): <https://web.archive.org/web/20170816015822/http://www.nxp.com/docs/en/application-note/AN11075.pdf>

## Addendum (2026-07-08) — Decision: SPI + board-to-board module docking

### Decisions

1. **SPI is confirmed** as the core↔module link bus, per the evaluation above.
2. **Modules dock directly to the core board-to-board — no link cables.** The
   only cables in a working setup are: USB to the PC, and the native
   input/output device cables. This avoids "cable monster" setups and
   eliminates the cable-connector mis-plug hazards analyzed above (PoE
   injection, phone-line lookalikes, HDMI-source confusion) along with most
   cable signal-integrity risk — the SPI link now runs over centimeter-scale
   traces and one mated connector pair.

### Consequences

- **Signal integrity:** with no cable, SPI clock headroom improves further;
  the remaining electrical constraint is the PL022 subnode clock-ratio limit
  (SSPCLK/12 — verify against the RP2040/RP2350 datasheets).
- **Mechanical design becomes the primary concern:** module PCB outline,
  connector position, and mounting-hole pattern must be standardized in
  [docs/specs/module-interface.md](../specs/module-interface.md) so any
  community module physically fits. Docking orientation (input side vs.
  output side of the core) and enclosure cutouts need definition.
- **Retention:** the connector alone should not carry mechanical load;
  standoffs or screw retention should take insertion/removal and handling
  forces.
- **Hot-swap policy must be defined:** typical board-to-board headers have no
  guaranteed contact-mating sequence (no first-mate/last-break ground pins).
  Until a connector with defined mating sequence is chosen or power sequencing
  is added, module swaps should be specified as **power-off only** — record
  this in the module interface spec.

### Docking connector families (selection pending)

Pin budget: ~10–12 contacts — SPI (SCLK, MOSI, MISO, CS, IRQ/ready), 5 V,
multiple GND, and reserved pins for future use.

| Family | Module-side cost | Hand-solderable | Sourcing | Keying | Notes |
|---|---|---|---|---|---|
| **2.54 mm dual-row shrouded (boxed) header + socket** | Very low | Yes (through-hole) | Generic, fully multi-source | Polarized shroud | Baseline candidate. Hackable with standard jumper wires during development — a real benefit for an open project |
| 2.54 mm unshrouded header + socket | Lowest | Yes | Generic | None — offset-insertion risk | Needs external keying (standoff pattern); acceptable fallback |
| Card-edge gold fingers + edge socket (cartridge style) | ≈ zero (fingers are part of the PCB) | Socket: yes (TH available) | Good (PCIe-class sockets) | Slot key | Thematically great (retro cartridge). Requires hard-gold edge plating for durability (adds PCB fab cost); verify socket mating-cycle rating |
| DIN 41612 (Eurocard) | Moderate | Yes (TH) | Excellent — decades-old multi-source standard | Polarized | Very rugged, high cycle life; larger footprint and cost |
| Fine-pitch mezzanine (0.4–0.5 mm stacking) | Low–mid | **No** | Often series-specific | Polarized | Rejected: fragile, hostile to community hand assembly |

*Mating-cycle ratings and unit pricing intentionally deferred to part
selection — to be pulled from manufacturer datasheets and
DigiKey/Mouser/LCSC listings rather than estimated here.*

**Recommendation:** 2.54 mm dual-row **shrouded/polarized** header pair as the
baseline, with the **socket (female) side on the core** so the powered
contacts are recessed and not exposed to shorts when no module is docked.
Card-edge is the preferred alternative if the cartridge form factor wins on
enclosure/UX grounds. Both stay within hobbyist hand-soldering skills and
generic multi-source supply.

### Updated open questions

- [ ] ~~PoE-misplug protection~~ — obsolete (no cable/RJ45)
- [ ] Choose docking connector family (baseline: 2.54 mm shrouded pair) and
      select parts with datasheet-verified mating-cycle and current ratings
- [ ] Standard module PCB outline, connector placement, mounting-hole pattern
      → [docs/specs/module-interface.md](../specs/module-interface.md)
- [ ] Hot-swap: confirm power-off-only policy or design mating
      sequence/power sequencing to permit live docking
- [ ] Module power budget at 5 V per module (drives contact current-rating
      verification)
- [ ] Enclosure concept: how docked modules are supported and covered
