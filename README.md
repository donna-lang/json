# json

<img src="https://img.shields.io/badge/Donna-json-FF6347?style=for-the-badge" alt="Donna json"/> <a href="https://donna-lang.github.io/json/"><img src="https://img.shields.io/badge/Docs-Read-2F81F7?style=for-the-badge" alt="Docs - Read"/></a> <img src="https://img.shields.io/github/actions/workflow/status/donna-lang/json/test.yml?branch=main&label=Test&style=for-the-badge" alt="Test status"/>

JSON parsing and encoding for the [Donna](https://github.com/donna-lang/donna) programming language.

## Overview

`json` parses JSON into Donna values, validates JSON quickly, and can query C-backed JSON documents without building a full Donna tree.

The parser is backed by [yyjson](https://github.com/ibireme/yyjson), with two ways to work:

- Use `json.parse` when you want a normal Donna `Json` value.
- Use `json.open` and `json.query_*` when you want fast access to a few fields.

## Installation

Add to your `donna.toml` as a dependency:

```toml
[dependencies]
json = { git = "https://github.com/donna-lang/json", version = ">=0.1.2 and <1.0.0" }
```

Then import the module:

```donna
import json
```

## Quick start

Parse into Donna values:

```donna
import json

pub fn read_name() -> String:
  let src = "{\"name\": \"Donna\", \"version\": 1}"
  case json.parse(src):
    json.Err(_) -> ""
    json.Ok(root) ->
      case json.get(root, "name"):
        json.Err(_) -> ""
        json.Ok(name) -> json.unwrap_string(json.as_string(name))
```

Query without materializing the whole document:

```donna
import json

pub fn read_title(src: String) -> String:
  case json.open(src):
    json.Err(_) -> ""
    json.Ok(doc) ->
      let title = json.unwrap_string(json.query_string(doc, "items.0.title"))
      let _ = json.close(doc)
      title
```

Create and encode JSON:

```donna
import json

pub fn make_payload() -> String:
  json.Object([
    #("name", json.Str("Donna")),
    #("ok", json.Bool(True)),
    #("count", json.Int(3))
  ])
  |> json.encode
```

Validate JSON:

```donna
import json

pub fn valid_config(src: String) -> Bool:
  json.is_valid(src)
```

Run tests:

```sh
donna test
```

## API

For API Reference visit the generated docs [here](https://donna-lang.github.io/json/)

## Supported JSON

```json
{
  "name": "Donna",
  "version": 1,
  "ratio": 3.5,
  "ok": true,
  "items": ["compiler", "stdlib", null]
}
```

Supported values: objects, arrays, strings, integers, floats, booleans, and null.

## Fast document handles

`json.parse` gives you a Donna `Json` tree. That is comfortable, but it means Donna has to allocate the whole document.

For larger documents, open a C-backed document and query paths:

```donna
case json.open(src):
  json.Err(_) -> ""
  json.Ok(doc) ->
    let name = json.unwrap_string(json.query_string(doc, "user.name"))
    let first = json.unwrap_string(json.query_string(doc, "items.0.title"))
    let _ = json.close(doc)
    name <> ": " <> first
```

Path syntax is intentionally small:

```text
user.name
items.0.title
```

Use `json.is_valid` when you only need to check whether input is valid JSON. It calls yyjson directly and avoids allocating Donna `Json` values.

Use `json.open`, `json.query`, and the typed `query_*` helpers when you want fast access to a few values without materializing the whole JSON document.

## Licence

MIT
