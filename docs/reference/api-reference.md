# API Reference

## Public Headers

- `yourlib/yourlib.hpp`
- `yourlib/export.hpp`
- generated `yourlib/version.hpp`

## Namespace

```cpp
namespace yourlib
```

## Class `yourlib::LibraryInfo`

Purpose:

- provide a neutral working scaffold API
- demonstrate exported symbols, PIMPL, copy/move support, and public-package consumption

### Constructor

```cpp
explicit LibraryInfo(std::string library_name = "your_library");
```

Creates a placeholder library descriptor.

### `name`

```cpp
std::string name() const;
```

Returns the current placeholder library name.

### `version`

```cpp
std::string version() const;
```

Returns the generated scaffold version string.

### `describe`

```cpp
std::string describe(std::string_view consumer) const;
```

Returns a consumer-oriented description string.

### `set_name`

```cpp
void set_name(std::string library_name);
```

Updates the placeholder library name stored by the object.
