# TASD usage

> **Status:** Outline. Not yet implementable.

This specification will define how CHIRBot uses the
[TASD format](https://tasd.io/) for recordings and how TASD concepts relate to
the live module link. Serialization is provided by the external
[Fortinbra/TASD](https://github.com/Fortinbra/TASD) library.

## Required decisions

- Required and optional metadata for recordings created by CHIRBot
- Input sample representation, controller/port identity, and timebase
- Recording start/stop, incomplete-file recovery, and storage layout
- Playback validation, unsupported packets, and forward compatibility
- Boundary between TASD file packets and the live SPI transport envelope
- Representation of rumble, motion, authentication, and other return data
- Upstream extension process and handling of experimental packet keys
- Golden files and conformance tests against the upstream specification

CHIRBot-specific extensions should be proposed upstream and must not silently
reuse keys assigned by the TASD specification.