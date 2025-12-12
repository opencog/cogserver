# distutils: language = c++
# cython: language_level=3

from opencog.cogserver cimport cCogServer, cCogServerNode, cCogServerNodePtr, CogServerNodeCast
from opencog.atomspace cimport cHandle, Atom, handle_cast
from opencog.type_constructors import get_thread_atomspace
from opencog.atomspace import types
from cython.operator cimport dereference as deref
from libcpp.string cimport string
import threading

# Global server state
cdef cCogServerNodePtr _server_ptr
_server_handle = None  # Keep Python reference to prevent GC
_server_thread = None
_server_running = False

def start_cogserver(console_port=17001, web_port=18080, mcp_port=18888,
                    enable_console=True, enable_web=True, enable_mcp=True,
                    name="cogserver"):
    """Start the CogServer with the specified configuration.

    Args:
        console_port (int, optional): Port for telnet console server. Default: 17001
        web_port (int, optional): Port for web server. Default: 18080
        mcp_port (int, optional): Port for MCP server. Default: 18888
        enable_console (bool, optional): Enable telnet console server. Default: True
        enable_web (bool, optional): Enable web server. Default: True
        enable_mcp (bool, optional): Enable MCP server. Default: True
        name (str, optional): Name for the CogServerNode. Default: "cogserver"

    Returns:
        bool: True if server started successfully, False otherwise.

    Raises:
        RuntimeError: If server is already running.
        ValueError: If invalid ports are specified.
    """
    global _server_ptr, _server_handle, _server_thread, _server_running

    if _server_running:
        raise RuntimeError("CogServer is already running")

    # Validate ports
    for port, pname in [(console_port, "console"), (web_port, "web"), (mcp_port, "mcp")]:
        if not (1 <= port <= 65535):
            raise ValueError(f"Invalid {pname} port: {port}. Must be between 1 and 65535")

    # Get the thread atomspace and add the CogServerNode to it
    atomspace = get_thread_atomspace()
    _server_handle = atomspace.add_node(types.CogServerNode, name)

    # Get the C++ CogServerNode pointer from the Python Atom
    cdef Atom atom = <Atom>_server_handle
    cdef cHandle chandle = handle_cast(atom.shared_ptr)
    _server_ptr = CogServerNodeCast(chandle)

    deref(_server_ptr).loadModules()

    # Enable requested services
    try:
        if enable_console:
            deref(_server_ptr).enableNetworkServer(console_port)
        if enable_web:
            deref(_server_ptr).enableWebServer(web_port)
        if enable_mcp:
            deref(_server_ptr).enableMCPServer(mcp_port)
    except Exception as e:
        # Clean up on failure
        if enable_mcp:
            deref(_server_ptr).disableMCPServer()
        if enable_web:
            deref(_server_ptr).disableWebServer()
        if enable_console:
            deref(_server_ptr).disableNetworkServer()
        _server_ptr.reset()
        _server_handle = None
        raise RuntimeError(f"Failed to start server: {e}")

    # Start server loop in a separate thread
    def server_loop():
        """Run the server's main loop."""
        global _server_ptr
        try:
            with nogil:
                deref(_server_ptr).serverLoop()
        except Exception as e:
            print(f"Server loop error: {e}")

    _server_thread = threading.Thread(target=server_loop, daemon=True)
    _server_thread.start()

    # Mark as initialized
    _server_running = True

    # Give the server a moment to start
    import time
    time.sleep(0.1)

    return True

def stop_cogserver():
    """Stop the running CogServer.

    Returns:
        bool: True if server was stopped, False if server was not running.
    """
    global _server_ptr, _server_handle, _server_thread, _server_running

    if not _server_running or not _server_ptr:
        return False

    # Stop the server
    deref(_server_ptr).stop()

    # Disable all services
    deref(_server_ptr).disableMCPServer()
    deref(_server_ptr).disableWebServer()
    deref(_server_ptr).disableNetworkServer()

    # Wait for the server thread to finish (with timeout)
    if _server_thread and _server_thread.is_alive():
        _server_thread.join(timeout=5.0)
        if _server_thread.is_alive():
            import warnings
            warnings.warn("CogServer thread did not stop cleanly")

    # Clean up
    _server_ptr.reset()
    _server_handle = None
    _server_thread = None
    _server_running = False

    return True

def is_cogserver_running():
    """Check if the CogServer is currently running.

    Returns:
        bool: True if server is running, False otherwise.
    """
    global _server_running
    return _server_running
