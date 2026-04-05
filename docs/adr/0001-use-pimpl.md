# ADR 0001: Use PIMPL for the public library API

## Status
Accepted

## Context
A reusable library scaffold should provide a pattern for reducing rebuild impact and isolating private implementation details.

## Decision
Use PIMPL for the primary example type in the scaffold.

## Consequences
- Lower header coupling
- Better separation between interface and implementation
- Extra indirection and dynamic allocation cost in some designs
