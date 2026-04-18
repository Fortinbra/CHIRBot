# Routing

## Default Routes
- **Firmware behavior, peripheral setup, relay logic:** Slippy
- **Build system, board configuration, Pico SDK integration:** Falco
- **Tests, verification plans, regression checks:** Peppy
- **Architecture, repo-wide decisions, multi-domain work:** Fox

## Escalation
- Multi-file changes that affect both runtime code and build configuration start with **Fox**.
- Reviewer or quality-gate work routes to **Peppy** unless the user names someone else.
- Squad state, decisions, and orchestration logging route to **Scribe**.
