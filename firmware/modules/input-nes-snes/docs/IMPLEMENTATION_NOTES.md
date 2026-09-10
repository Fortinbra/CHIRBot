# Implementation Notes

## Current Status

Project skeleton created with Pico SDK structure. Core files in place:
- CMakeLists.txt
- Main module class
- PIO, SPI, TASD stubs

## Next Steps

1. Implement PIO program for NES/SNES controller reading
2. Implement SPI subnode protocol
3. Integrate TASD encoding
4. Test with logic analyzer

## Dependencies

- Pico SDK
- TASD library (submodule)
- chirbot-common (to be created)

## Notes

NES and SNES use same electrical protocol, different physical pinouts.
First version supports standard controllers only, one controller per module.
