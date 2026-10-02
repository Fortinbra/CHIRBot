# Project Guidelines

## Embedded Firmware

- Prefer clear, direct implementations that make ownership, bounds, and hardware behavior easy to verify.
- Treat RAM, flash, stack, CPU time, and power as constrained resources. Avoid dynamic allocation in runtime paths unless its need and lifetime are justified.
- Keep buffers bounded and out of small task or main stacks; use static storage when a buffer's lifetime and ownership permit it.
- Prefer fixed-width types for hardware and protocol values, and validate lengths and ranges at input boundaries.
- Keep code self-documenting. Add comments only to explain non-obvious hardware constraints, invariants, or tradeoffs.
- Preserve existing interfaces and architecture unless the change requires otherwise; avoid unrelated refactoring.

## Build Quality

- Project-owned firmware targets and tests must compile with strict warnings treated as errors; builds must complete with no project-code warnings or errors.
- After firmware changes, run the `Compile Project` task and the narrowest relevant tests. Resolve warnings rather than suppressing them, unless a targeted suppression is necessary and explained.
- Keep changes consistent with existing formatting and naming conventions.
