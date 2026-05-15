# Changelog

All notable changes to `json` will be documented in this file.

## [0.1.0] — 2026-05-15

Initial release of `json`, a yyjson-backed JSON package for Donna.

### Added

- JSON value model with `Null`, `Bool`, `Int`, `Float`, `Str`, `Array`, and `Object`.
- `Parsed(a)` result type with `Ok` and `Err`.
- `json.parse` for parsing JSON strings into Donna `Json` values.
- `json.encode` for rendering Donna `Json` values back to JSON.
- `json.is_valid` for fast JSON validation without building a Donna tree.
- C-backed document handles with `json.open` and `json.close`.
- Path-based document queries with `json.query`.
- Typed document query helpers: `query_string`, `query_int`, `query_float`, and `query_bool`.
- Object and array helpers: `json.get` and `json.at`.
- Extractors: `as_string`, `as_int`, `as_float`, `as_bool`, `as_array`, and `as_object`.
- Predicates: `is_null`, `is_ok`, and `is_err`.
- Convenience unwrap helpers for strings, integers, and booleans.
- yyjson-backed FFI implementation with direct wire decoding in Donna.
- Self tests for parsing, encoding, extraction, document handles, nested values, and error cases.
