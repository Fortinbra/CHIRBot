# CHIRBot documentation

This directory is the source of truth for CHIRBot's current technical design.
The public website introduces the project and preserves useful early design
discussion, but it may lag decisions recorded here.

## Start here

| Document | Purpose |
|---|---|
| [Architecture](ARCHITECTURE.md) | System boundaries, component responsibilities, repository strategy, and open questions |
| [Roadmap](ROADMAP.md) | Ordered project milestones and near-term priorities |
| [Design decisions](design/) | Architecture decision records (ADRs) for accepted choices and their rationale |
| [Technical specifications](specs/) | Implementable contracts shared by firmware, hardware, and host software |

## Document lifecycle

- **Architecture** describes the current system-level model and links to the
  decisions that support it.
- **ADRs** capture significant choices. Accepted ADRs are historical records;
  replace a decision with a new superseding ADR rather than rewriting its
  rationale.
- **Specifications** define testable interfaces. A spec can begin as a draft,
  but hardware or firmware should not treat it as stable until its status says
  so explicitly.
- **Component READMEs** describe scope, dependencies, status, and component-
  specific setup. They should link to shared specs rather than duplicate them.

When these documents disagree, the newest accepted ADR controls the decision;
the architecture and website should then be updated to match.