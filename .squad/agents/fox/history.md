# Fox History

## Learnings
- Project requested by Fortinbra.
- CHIRBot is an RP2350B/Pico SDK firmware project for the core relay node between input and output nodes.

## Decisions Made (2026-04-18)

### Tighten Copilot Instructions
- Rewrote `.github/copilot-instructions.md` to be concise and repository-specific, removing generic sections
- **Critical Fix**: Corrected false claim that "main loop feeds watchdog continuously." Reality: watchdog is updated once at startup, main loop does NOT feed it. Adding operations >100ms will reboot the system.
- Kept only factual architecture info from code review and actual config files (CMakeLists.txt, main.cpp, blink.pio, .vscode/settings.json)
- Reduced from 218 lines to 72 lines; eliminated "Integration Notes" (team-internal), removed PIO program details (too low-level), removed "Copilot Guidelines" (generic advice)
