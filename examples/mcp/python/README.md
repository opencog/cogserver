Python MCP Examples
===================
Minimal, runnable MCP examples that exercise core AtomSpace operations from Python. Tested against CogServer MCP 0.2.1.

Prerequisites
-------------
- CogServer running locally with MCP enabled (default HTTP endpoint: http://localhost:18080/mcp).
- Python 3 with `requests` installed, for example:
  ```
  python -m pip install requests
  ```

Run the examples
----------------
From the repository root:

1) Create a few atoms and list Concepts:
```
python examples/mcp/python/01_create_atoms.py
```

2) Inspect incoming links for an atom:
```
python examples/mcp/python/02_get_incoming.py
```

3) Attach and read values on an atom:
```
python examples/mcp/python/03_set_values.py
```

What they demonstrate
---------------------
- MCP handshake (`initialize` + `notifications/initialized`) and `tools/list`.
- Creating atoms with `makeAtom` and querying with `getAtoms`.
- Finding links that reference a target atom with `getIncoming`.
- Setting values (`setValue`) and reading them back (`getKeys`, `getValues`).

Notes
-----
- Scripts are idempotent: re-running them will reuse existing atoms if present.
- Output will reflect the current AtomSpace contents; examples assume an otherwise empty workspace.
