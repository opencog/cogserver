MCP Tools
---------

### Demo MCP checker
The code in `mcp-checker.cc` provides a simple Model Context protocol
(MCP) test client that can be used to verify that an MCP network server
is available, and is responding to commands.  It will list the tools
and resources provided by the MCP server. By default, it connects to
`localhost:18888`, which is where the CogServer MCP port is located.
Use the `--host` and `--port` flags to specify a different host and
port.

The binary is located at [build/examples/mcp](../../build/examples/mcp).

### Socket Proxies
Due to technical issues, it can be the case that an LLM cannot contact the
CogServer directly. The pair of proxy servers `stdio_to_unix_proxy.py`
and `unix_to_tcp_proxy.py` can be used to overcome/bypass these issues.

For example, if using Claude Code, the proxy can be configured as
```
claude mcp list
clause mcp add cogserv /where/ever/stdio_to_unix_proxy.py
```
Then, in a distinct shell, run
```
./unix_to_tcp_proxy.py --remote-host example.com --remote-port 18888 --verbose
```
and then make sure that the CogServer is running on that host. The
default CogServer MCP port is 18888.
