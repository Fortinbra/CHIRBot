# output-usb

> **Status:** planned — no code yet.

USB output module. Receives TASD-packetized SPI data from the core and presents
it to the target device as a native USB controller, supporting multiple HID
modes for interoperability (including quirky hosts like the original Xbox) and
controller spoofing (e.g., presenting as a Hori pad to a Nintendo Switch,
possibly with a secondary port for an authentication device).

See [firmware/modules/README.md](../README.md) and
[docs/ARCHITECTURE.md](../../../docs/ARCHITECTURE.md).
