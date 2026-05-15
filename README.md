# json

<img src="https://img.shields.io/badge/Donna-json-FF6347?style=for-the-badge" alt="Donna json"/>
<a href="https://donna-lang.github.io/json/"><img src="https://img.shields.io/badge/Docs-Read-2F81F7?style=for-the-badge" alt="Docs - Read"/></a><img src="https://img.shields.io/github/actions/workflow/status/donna-lang/json/test.yml?branch=main&label=Test&style=for-the-badge" alt="Test status"/>

JSON parsing and encoding for the [Donna](https://github.com/donna-lang/donna) programming language.

## Overview

`json` parses JSON into Donna values and can encode Donna JSON values back to strings.

The parser is backed by [yyjson](https://github.com/ibireme/yyjson), with a small Donna API on top.

## Installation

Add to your `donna.toml` as a dependency:

```toml
[dependencies]
json = { git = "https://github.com/donna-lang/json", version = ">=0.1.0 and <1.0.0" }
```

Then import the module:

```donna
import json
```

## Quick start

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

## Licence

MIT
