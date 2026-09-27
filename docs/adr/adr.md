# FmuToolkit - Architecture Decision Records

An Architecture Decision Record (ADR) captures one significant technical
decision: the context that forced it, the option chosen, and the consequences
accepted. ADRs are append-only - a decision is never edited away, it is
**superseded** by a newer record that links back to it.

## Index

| ID | Title | Status | Date |
|---|---|---|---|
| ADR-0001 | Adopt hexagonal architecture | Accepted | YYYY-MM-DD |

Statuses: `Proposed`, `Accepted`, `Deprecated`, `Superseded by ADR-NNNN`.

## ADR-0001 - Adopt hexagonal architecture

- **Status:** Accepted
- **Date:** YYYY-MM-DD
- **Deciders:** <names / roles>

### Context

FmuToolkit must stay testable without its UI, database or operating-system
integrations, and those integrations are expected to change over the
product's lifetime.

### Decision

Structure the code as ports and adapters: a framework-free `domain/`, use
cases in `application/`, and every external interaction implemented as an
adapter in `adapter/` behind a port (interface) owned by the core. Dependencies
always point inwards.

### Consequences

- **Positive:** the domain and application layers are unit-testable in
  isolation; adapters can be replaced without touching business rules.
- **Negative:** more interfaces and wiring code than a layered design;
  contributors must respect the dependency rule.

---

## Template

Copy the block below for every new decision and add a row to the index.

```markdown
## ADR-NNNN - <short title in the imperative>

- **Status:** Proposed
- **Date:** YYYY-MM-DD
- **Deciders:** <names / roles>

### Context

<The forces at play: requirements, constraints, trade-offs. Why a decision
is needed now.>

### Considered options

1. <option A> - <pros / cons>
2. <option B> - <pros / cons>

### Decision

<The option chosen and the reasoning, stated in full sentences.>

### Consequences

- **Positive:** <...>
- **Negative:** <...>
- **Follow-ups:** <tasks, specs or Jira issues this decision creates>
```
