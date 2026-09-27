# FmuToolkit - Architecture

> FmuToolkit - lib-shared

This document describes how FmuToolkit is structured and why. Keep it in
sync with the code: update it in the same change that alters a layer, a port
or a deployment unit. Individual decisions and their rationale live in
[`../adr/adr.md`](../adr/adr.md).

## 1. Context

<What the system does, who uses it, and which external systems it talks to
(users, services, files, databases, hardware).>

```text
 [ User / caller ] --> [ FmuToolkit ] --> [ external systems ]
```

## 2. Architectural style

FmuToolkit follows a **hexagonal architecture (ports and adapters)**
(Alistair Cockburn, 2005): the business core sits in the middle and talks
to the outside world only through **ports** (interfaces it owns);
**adapters** implement those ports for a concrete technology.

```text
        driving side (who calls us)                    driven side (what we call)
  CLI / GUI / HTTP / MCP / tests                     database / files / HTTP APIs / logs
            │                                                   ▲
      adapter/in/*  ──►  application/port/in  ──►  application/service  ──►  application/port/out  ◄──  adapter/out/*
                                                        │
                                                        ▼
                                                     domain/
```

**Dependency rule:** source-code dependencies point inwards only. `domain`
depends on nothing; `application` depends on `domain`; adapters depend on
`application`/`domain`; only `config` knows every concrete class.

| Folder | What belongs there | Must not contain |
|---|---|---|
| `domain/model/` | Entities and aggregates: objects with identity and lifecycle, enforcing their invariants | I/O, framework types, DTOs |
| `domain/valueobject/` | Immutable values compared by value (Money, Email, Range), validated on construction | Identity, setters |
| `domain/event/` | Domain events: facts that happened (`OrderPlaced`), raised by aggregates | Delivery mechanics |
| `domain/service/` | Business rules spanning several entities, pure functions of domain types | Persistence, logging |
| `domain/exception/` | Business rule violations (`InsufficientFunds`) | Technical errors |
| `application/port/in/` | **Driving ports**: use-case interfaces offered to the outside | Implementations |
| `application/port/out/` | **Driven ports**: interfaces the application needs (repositories, gateways, notifiers, clock) | Technology details |
| `application/service/` | Use-case implementations: orchestrate domain + driven ports, own transactions | Business rules (they live in the domain), I/O |
| `application/command/`, `query/` | Input models of the use cases (write / read side) | Behaviour |
| `application/dto/` | Output models returned to adapters | Domain objects leaking out |
| `application/mapper/` | Domain <-> DTO translation | Business rules |
| `application/exception/` | Use-case failures (`NotFound`, `Unauthorized`) | Technical errors |
| `adapter/in/*` | **Inbound adapters**: CLI, GUI widgets/view models, System Tray, HTTP, MCP - translate a request into a driving-port call | Business rules |
| `adapter/out/*` | **Outbound adapters**: persistence (SQLite, memory), filesystem, HTTP clients, logging, event publishing, OS shell (Taskbar progress) - implement driven ports | Use-case logic |
| `config/` | Composition root: builds adapters, injects them into services, exposes driving ports | Logic of any kind |

Categories that are not generated with this tree (Python services, web
apps) apply the same principle with their own folders - see the README.

## 3. Ports

| Port | Direction | Purpose | Adapter(s) |
|---|---|---|---|
| `<PortName>` | in / out | <what it abstracts> | `<AdapterName>` |

## 4. Runtime view

<The main flows, e.g. "user action -> inbound adapter -> use case -> domain
service -> outbound port -> adapter". Add a sequence diagram for any flow that
crosses more than two components.>

## 5. Deployment view

<Artifacts produced by the build (executables, libraries, images), where they
are deployed (`project.json` -> `deploy_folder`) and their runtime
dependencies.>

## 6. Cross-cutting concerns

| Concern | Approach |
|---|---|
| Error handling | <exceptions / error codes / result types> |
| Logging | <adapter, format, levels> |
| Configuration | <`project.json`, environment, settings files> |
| Testing | unit tests on domain/application, integration tests on adapters (`tests/`) |
| Versioning | single source of truth in `project.json` / `pyproject.toml` |

## 7. Quality attributes and constraints

<Performance, portability, security, maintainability targets and hard
constraints (platforms, toolchains, licensing).>

## 8. Risks and technical debt

| Item | Impact | Mitigation |
|---|---|---|
| <risk> | <low / medium / high> | <plan> |
