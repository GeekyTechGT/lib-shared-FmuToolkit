# Design Notes

## Why The Scaffold Uses A Placeholder API

The scaffold needs to compile, install, test, and demonstrate consumption before a real library is added.
For that reason it ships a neutral placeholder API in `include/yourlib/yourlib.hpp` and `src/yourlib.cpp`.

That placeholder API is intentionally lightweight and easy to replace.

## Why PIMPL Is Kept In The Scaffold

The scaffold models a conservative distribution boundary:

- small public install surface
- private implementation hidden under `src/`
- better downstream rebuild isolation
- easier replacement with a real exported ABI boundary

## Why Recursive Source Discovery Is Acceptable Here

For production libraries, explicit source lists are often preferred.
For a scaffold, automatic discovery is more valuable because it allows users to add sources and tests without modifying infrastructure files.

That matches the main requirement of this repository.
