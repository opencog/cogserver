CogServer Python Interface
--------------------------

The CogServer can be started from Python using the following code:

```python
from opencog.atomspace import AtomSpace
from opencog.cogserver import start_cogserver, stop_cogserver

# Option 1: Start with default AtomSpace
start_cogserver()

# Option 2: Start with custom AtomSpace
my_atomspace = AtomSpace()
start_cogserver(atomspace=my_atomspace)

# Option 3: Start with custom ports
start_cogserver(
    console_port=17001,  # Telnet console port
    web_port=18080,      # Web interface port
    mcp_port=18888,      # MCP server port
    enable_console=True, # Enable telnet console
    enable_web=False,    # Disable web interface
    enable_mcp=False     # Disable MCP server
)

# Stop the server when done
stop_cogserver()
```

## Available Functions ##

The `opencog.cogserver` module provides:

* `start_cogserver(atomspace=None, console_port=17001, web_port=18080, mcp_port=18888, enable_console=True, enable_web=False, enable_mcp=False)` - Start the CogServer
* `stop_cogserver()` - Stop the running CogServer
* `is_cogserver_running()` - Check if server is running
* `get_server_atomspace()` - Get the AtomSpace used by the server

## Tutorial ##

The OpenCog wiki contains the Python tutorial:

http://wiki.opencog.org/w/Python
