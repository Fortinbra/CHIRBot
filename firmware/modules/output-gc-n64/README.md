# output-gc-n64

> **Status:** planned — no code yet.

GameCube/N64 output module. Receives TASD-packetized SPI data from the core and
answers console Joybus polls as a native controller. The console's poll is the
clock-domain source
(see [docs/specs/clock-domains.md](../../../docs/specs/clock-domains.md)).

See [firmware/modules/README.md](../README.md) and
[docs/ARCHITECTURE.md](../../../docs/ARCHITECTURE.md).
