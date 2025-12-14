#
# cogserver.py
#
# Pure Python wrapper for starting/stopping the CogServer.
# This module provides a convenient Python API for controlling
# the CogServer network server.
#

import ctypes
import os

# Load the servernode library to register CogServerNode type
# This must happen before we can use types.CogServerNode
_lib_paths = [
    # Build directory (for testing without install)
    os.path.join(os.path.dirname(__file__), "../../../../build/opencog/cogserver/atoms/libservernode.so"),
    # Installed locations
    "/usr/local/lib/opencog/libservernode.so",
    "/usr/lib/opencog/libservernode.so",
]
_servernode_lib = None
for _path in _lib_paths:
    if os.path.exists(_path):
        _servernode_lib = ctypes.CDLL(_path, mode=ctypes.RTLD_GLOBAL)
        break

if _servernode_lib is None:
    raise ImportError("Could not find libservernode.so - is cogserver installed?")

from opencog.type_constructors import get_thread_atomspace, FloatValue, VoidValue, Predicate
from opencog.atomspace import types

def start_cogserver(console_port=17001, web_port=18080, mcp_port=18888,
                    enable_console=True, enable_web=True, enable_mcp=True):
    """Start the CogServer with the specified configuration.

    Args:
        console_port (int, optional): Port for telnet console server. Default: 17001
        web_port (int, optional): Port for web server. Default: 18080
        mcp_port (int, optional): Port for MCP server. Default: 18888
        enable_console (bool, optional): Enable telnet console server. Default: True
        enable_web (bool, optional): Enable web server. Default: True
        enable_mcp (bool, optional): Enable MCP server. Default: True

    Returns:
        CogServerNode: The CogServerNode atom that was created.

    Raises:
        ValueError: If invalid ports are specified.
    """
    # Validate ports
    for port, pname in [(console_port, "console"), (web_port, "web"), (mcp_port, "mcp")]:
        if not (1 <= port <= 65535):
            raise ValueError(f"Invalid {pname} port: {port}. Must be between 1 and 65535")

    # Compute effective port values (0 disables a server)
    effective_console = console_port if enable_console else 0
    effective_web = web_port if enable_web else 0
    effective_mcp = mcp_port if enable_mcp else 0

    # Construct name from port configuration
    name = f"cogserver://{effective_console}:{effective_web}:{effective_mcp}"

    # Get the thread atomspace and add the CogServerNode to it
    atomspace = get_thread_atomspace()
    server_handle = atomspace.add_node(types.CogServerNode, name)

    if effective_console != 17001:
        key = Predicate("*-telnet-port-*")
        atomspace.set_value(server_handle, key, FloatValue(float(effective_console)))
    if effective_web != 18080:
        key = Predicate("*-web-port-*")
        atomspace.set_value(server_handle, key, FloatValue(float(effective_web)))
    if effective_mcp != 18888:
        key = Predicate("*-mcp-port-*")
        atomspace.set_value(server_handle, key, FloatValue(float(effective_mcp)))

    # Start all servers (this also starts the server thread)
    atomspace.set_value(server_handle, Predicate("*-start-*"), VoidValue())

    return server_handle

def stop_cogserver(server_node):
    """Stop the specified CogServer.

    Args:
        server_node: The CogServerNode to stop.
    """
    atomspace = get_thread_atomspace()
    atomspace.set_value(server_node, Predicate("*-stop-*"), VoidValue())
