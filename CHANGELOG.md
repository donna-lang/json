# Changelog

All notable changes to `json` will be documented in this file.

## [0.1.0] — 2026-05-15

Initial release.

### Added

- `json.parse` — parse a JSON document string into a `Json` value
- `json.encode` — encode a `Json` value back to a JSON string
- `json.get` — read object fields by key
- `json.at` — read array items by index
- `json.as_string`, `as_int`, `as_float`, `as_bool`, `as_array`, and `as_object` extractors
- `json.is_null`, `is_ok`, and `is_err` helpers
- `json.unwrap_string`, `unwrap_int`, and `unwrap_bool` convenience helpers
- Support for objects, arrays, strings, integers, floats, booleans, and null
- yyjson-backed FFI parser
- Self tests covering parsing, encoding, extraction, nested values, and error cases
