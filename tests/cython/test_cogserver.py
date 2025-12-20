#!/usr/bin/env python3
"""
Unit tests for the CogServer Python bindings.

This test suite verifies the functionality of the CogServer Python API.
"""

import unittest
import time
import sys
import os
import ctypes

# Add the source directory to Python path for the pure Python cogserver module
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../../opencog/cogserver/python'))

# Load the servernode library to register CogServerNode type.
# Uses COGSERVER_MODULE_PATH set by CMake for build directory testing.
def _load_servernode_lib():
    lib_name = "libservernode.so"
    module_path = os.environ.get("COGSERVER_MODULE_PATH", "")
    for directory in module_path.split(":"):
        if directory:
            path = os.path.join(directory, lib_name)
            if os.path.exists(path):
                return ctypes.CDLL(path, mode=ctypes.RTLD_GLOBAL)
    raise ImportError(f"Could not find {lib_name} - set COGSERVER_MODULE_PATH")

_servernode_lib = _load_servernode_lib()

try:
    from opencog.atomspace import AtomSpace
    from opencog.type_constructors import get_thread_atomspace, Predicate
    from opencog.cogserver import (
        start_cogserver,
        stop_cogserver,
    )
except ImportError as e:
    print(f"Error importing modules: {e}")
    print("Make sure the opencog modules are properly built and installed.")
    sys.exit(1)


# Global to track the current server node for cleanup
_current_server = None


def is_server_running(server_node):
    """Check if a CogServerNode is running by querying its value."""
    if server_node is None:
        return False
    val = server_node.get_value(Predicate("*-is-running?-*"))
    # val is a BoolValue; check if it's true
    return val.to_list()[0] == 1


class TestCogServer(unittest.TestCase):
    """Test cases for CogServer Python bindings."""

    def setUp(self):
        """Ensure server is stopped before each test."""
        global _current_server
        if _current_server is not None and is_server_running(_current_server):
            stop_cogserver(_current_server)
            _current_server = None
        # Small delay to ensure clean state
        time.sleep(0.1)

    def tearDown(self):
        """Ensure server is stopped after each test."""
        global _current_server
        if _current_server is not None and is_server_running(_current_server):
            stop_cogserver(_current_server)
            _current_server = None
        # Small delay to ensure clean shutdown
        time.sleep(0.1)

    def test_basic_start_stop(self):
        """Test basic server start/stop functionality."""
        global _current_server

        # Start server with custom port to avoid conflicts
        _current_server = start_cogserver(console_port=17550, enable_web=False, enable_mcp=False)
        self.assertIsNotNone(_current_server, "start_cogserver should return CogServerNode")

        # Check server is running
        self.assertTrue(is_server_running(_current_server),
                       "Server should be running after start")

        # Stop the server
        stop_cogserver(_current_server)

        # Check server is stopped
        self.assertFalse(is_server_running(_current_server),
                        "Server should not be running after stop")

    def test_custom_ports(self):
        """Test server with custom port configuration."""
        global _current_server

        # Start server with custom ports
        _current_server = start_cogserver(
            console_port=17552,
            web_port=18581,
            mcp_port=18889,
            enable_console=True,
            enable_web=False,  # Disable web to avoid SSL issues in tests
            enable_mcp=False
        )
        self.assertIsNotNone(_current_server, "Server should start with custom ports")

        # Check server is running
        self.assertTrue(is_server_running(_current_server), "Server should be running")

        # Let it run briefly
        time.sleep(0.5)

        # Stop the server
        stop_cogserver(_current_server)
        self.assertFalse(is_server_running(_current_server),
                        "Server should be stopped")

    def test_invalid_port_error(self):
        """Test that invalid port raises error."""
        # Try invalid port - should raise ValueError
        with self.assertRaises(ValueError) as context:
            start_cogserver(console_port=99999)

        self.assertIn("Invalid", str(context.exception))
        self.assertIn("port", str(context.exception))

    def test_multiple_start_stop_cycles(self):
        """Test multiple start/stop cycles."""
        global _current_server

        for i in range(3):
            # Start server
            _current_server = start_cogserver(console_port=17560 + i,
                                    enable_web=False, enable_mcp=False)
            self.assertIsNotNone(_current_server, f"Server should start on cycle {i}")
            self.assertTrue(is_server_running(_current_server),
                          f"Server should be running on cycle {i}")

            # Stop server
            stop_cogserver(_current_server)
            self.assertFalse(is_server_running(_current_server),
                           f"Server should be stopped on cycle {i}")

            # Small delay between cycles
            time.sleep(0.2)


def run_tests():
    """Run the test suite."""
    # Create test suite
    loader = unittest.TestLoader()
    suite = loader.loadTestsFromTestCase(TestCogServer)

    # Run tests
    runner = unittest.TextTestRunner(verbosity=2)
    result = runner.run(suite)

    # Return exit code based on result
    return 0 if result.wasSuccessful() else 1


if __name__ == "__main__":
    sys.exit(run_tests())
