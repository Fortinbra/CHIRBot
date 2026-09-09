# 0002 — Standard MCU: RP2350B

- **Status:** Accepted
- **Date:** 2026-07-08
- **Relates to:** [docs/ARCHITECTURE.md](../ARCHITECTURE.md) §2.2–2.3, open questions §5

## Decision

All CHIRBot boards — the core and every input/output module — standardize on
the **Raspberry Pi RP2350B** as the "one size fits all" MCU. The Pico SDK
(already the project's only finalized technical decision) fully supports it.

## Context

The architecture previously left "RP2040 vs RP2350 per component" open. A
single standard MCU simplifies firmware (one `PICO_PLATFORM`), one SPI/PIO
capability baseline for the module link spec, shared board layouts, and a
single BOM line item purchased in higher volume.

### Variant comparison

Per the [RP2350 product page](https://www.raspberrypi.com/products/rp2350/)
and [Raspberry Pi microcontroller documentation](https://www.raspberrypi.com/documentation/microcontrollers/microcontroller-chips.html):

| Feature | RP2040 | RP2350A | **RP2350B** | RP2354B |
|---|---|---|---|---|
| Cores | 2× Cortex-M0+ @ 133 MHz | 2× Cortex-M33 (or Hazard3 RISC-V) @ 150 MHz | same | same |
| SRAM | 264 kB | 520 kB | 520 kB | 520 kB |
| Internal flash | none | none | **none — external QSPI required** | 2 MB stacked |
| GPIO | 30 | 30 | **48** | 48 |
| ADC channels | 4 | 4 | **8** | 8 |
| PIO | 2 blocks / 8 SMs | 3 blocks / 12 SMs | **3 blocks / 12 SMs** | 3 blocks / 12 SMs |
| PWM channels | 16 | 16 | **24** | 24 |
| Security | none | TrustZone, signed boot, SHA-256, TRNG, OTP | same | same |
| Package | QFN-56, 7×7 mm | QFN-60, 7×7 mm | **QFN-80, 10×10 mm** | QFN-80, 10×10 mm |

## Rationale

1. **GPIO headroom (48)**: the core needs SPI ×2 (input + output module
   docks), microSD, display, buttons, and USB simultaneously; modules like
   the visualization or multi-connector retro boards benefit from wide
   native-signal fan-out plus the dock without pin gymnastics.
2. **12 PIO state machines**: PIO is the workhorse for USB host, Joybus,
   NES/SNES serial, PS/2, and precise poll-edge timing. The third PIO block
   (vs. RP2040's two) is direct margin for the module protocol adapters.
3. **8 ADC channels**: analog inputs (adaptive controllers, paddles,
   potentiometer-based sticks like the C64/Atari ecosystem) without external
   ADCs.
4. **Security features**: TrustZone, signed boot, and SHA-256/TRNG are useful
   for the USB output module's console-authentication scenarios and give the
   project an integrity story for distributed firmware.
5. **One SDK, one platform**: full Pico SDK support; single firmware target
   (`PICO_PLATFORM=rp2350-arm-s`), no RP2040/RP2350 conditional maintenance.
6. **Longevity**: Raspberry Pi commits to production until **at least
   January 2045** ([product page](https://www.raspberrypi.com/products/rp2350/)) —
   excellent for an open-hardware project.
7. **Availability**: sold via Raspberry Pi's distributor network and stocked
   at turnkey assemblers (JLCPCB carries RP2350 officially, per the product
   page) — supports both self-build and assembled-board distribution.

### Why not the alternatives

- **RP2040 / RP2350A**: 30 GPIO and fewer PIO resources force per-board MCU
  variant decisions — exactly what this ADR eliminates. The cost delta does
  not outweigh the loss of standardization (verify actual deltas at BOM
  stage).
- **RP2354B**: same silicon with 2 MB stacked flash. Rejected as the
  *standard* because 2 MB caps firmware+assets and removes the flash-size
  degree of freedom; external QSPI flash (up to 32 MB supported) is cheap and
  lets the core (which may want larger buffers/UI assets) and modules size
  flash independently. It remains a drop-in option for cost-reduced module
  spins.

## Consequences

- **External QSPI flash** is a required BOM line on every board (RP2350B has
  no internal flash).
- **QFN-80, 0.4 mm pitch is not hand-solderable with an iron.** This tensions
  with the project's hand-assembly preference: community builds will need
  hotplate/hot-air/stencil reflow or assembly services. Mitigation: keep all
  *other* parts hand-friendly where possible, and ensure designs are
  compatible with turnkey assembly (JLCPCB/PCBWay parts availability).
- **Not 5 V tolerant**: as with RP2040, GPIO is 3.3 V — level shifting for
  retro 5 V buses remains a module requirement (unchanged).
- **Errata review required at design time**: notably the RP2350 GPIO
  input-leakage erratum (E9) affecting internal pull-downs — plan for
  external pull-downs where a pin must read low reliably. See the errata
  appendix of the [RP2350 datasheet](https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf).
- The [minimal RP2350A/B KiCad reference design](https://datasheets.raspberrypi.com/rp2350/Minimal-KiCAD.zip)
  and [Hardware design with RP2350](https://datasheets.raspberrypi.com/rp2350/hardware-design-with-rp2350.pdf)
  guide are the starting points for all boards.
- Firmware in this repo targets RP2350 only; RP2040 compatibility is no
  longer a goal (the TASD library remains platform-agnostic C99 regardless).

## References

- RP2350 product page: <https://www.raspberrypi.com/products/rp2350/>
- RP2350 datasheet: <https://datasheets.raspberrypi.com/rp2350/rp2350-datasheet.pdf>
- Hardware design with RP2350: <https://datasheets.raspberrypi.com/rp2350/hardware-design-with-rp2350.pdf>
- Microcontroller chips documentation (variant table): <https://www.raspberrypi.com/documentation/microcontrollers/microcontroller-chips.html>
