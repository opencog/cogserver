Demo MCP checker
----------------
The code here provides a simple Model Context protocol (MCP) test
client that can be used to verify that an MCP network server is
available, and is responding to commands.  It will list the tools
and resources provided by the MCP server. By default, it connects to
`localhost:18888`, which is where the CogServer MCP port is located.
Use the `--host` and `--port` flags to specify a different host and
port.

The binary is located at [build/examples/mcp](../../build/examples/mcp).


