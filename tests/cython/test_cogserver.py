#!/usr/bin/env python3
"""
Unit tests for the CogServer Python bindings.

This test suite verifies the functionality of the CogServer Python API.
"""

import unittest
import time
import sys
import os

# Add the build directory to Python path if needed
sys.path.insert(0, os.path.join(os.path.dirname(__file__), '../../build/opencog/cython'))

try:
    from opencog.atomspace import AtomSpace
    from opencog.cogserver import (
        start_cogserver,
        stop_cogserver,
        is_cogserver_running,
        get_server_atomspace
    )
except ImportError as e:
    print(f"Error importing modules: {e}")
    print("Make sure the opencog modules are properly built and installed.")
    sys.exit(1)


class TestCogServer(unittest.TestCase):
    """Test cases for CogServer Python bindings."""

    def setUp(self):
        """Ensure server is stopped before each test."""
        if is_cogserver_running():
            stop_cogserver()
        # Small delay to ensure clean state
        time.sleep(0.1)

    def tearDown(self):
        """Ensure server is stopped after each test."""
        if is_cogserver_running():
            stop_cogserver()
        # Small delay to ensure clean shutdown
        time.sleep(0.1)

    def test_basic_start_stop(self):
        """Test basic server start/stop functionality."""
        # Check server is not running initially
        self.assertFalse(is_cogserver_running(),
                        "Server should not be running initially")

        # Start server with custom port to avoid conflicts
        result = start_cogserver(console_port=17550, enable_web=False, enable_mcp=False)
        self.assertTrue(result, "start_cogserver should return True")

        # Check server is running
        self.assertTrue(is_cogserver_running(),
                       "Server should be running after start")

        # Get the server's atomspace
        server_as = get_server_atomspace()
        self.assertIsNotNone(server_as,
                           "Should be able to get server atomspace")

        # Stop the server
        result = stop_cogserver()
        self.assertTrue(result, "stop_cogserver should return True")

        # Check server is stopped
        self.assertFalse(is_cogserver_running(),
                        "Server should not be running after stop")

    def test_custom_atomspace(self):
        """Test server with custom AtomSpace."""
        # Create a custom atomspace
        custom_as = AtomSpace()

        # Start server with custom atomspace on different port
        result = start_cogserver(atomspace=custom_as, console_port=17551,
                                enable_web=False, enable_mcp=False)
        self.assertTrue(result, "Server should start with custom atomspace")

        # Verify server is running
        self.assertTrue(is_cogserver_running(), "Server should be running")

        # Get server atomspace
        server_as = get_server_atomspace()
        self.assertIsNotNone(server_as, "Should get server atomspace")

        # Stop the server
        stop_cogserver()

    def test_custom_ports(self):
        """Test server with custom port configuration."""
        # Start server with custom ports
        result = start_cogserver(
            console_port=17552,
            web_port=18581,
            mcp_port=18889,
            enable_console=True,
            enable_web=False,  # Disable web to avoid SSL issues in tests
            enable_mcp=False
        )
        self.assertTrue(result, "Server should start with custom ports")

        # Check server is running
        self.assertTrue(is_cogserver_running(), "Server should be running")

        # Let it run briefly
        time.sleep(0.5)

        # Stop the server
        stop_cogserver()
        self.assertFalse(is_cogserver_running(),
                        "Server should be stopped")

    def test_double_start_error(self):
        """Test that starting server twice raises error."""
        # Start server
        start_cogserver(console_port=17553, enable_web=False, enable_mcp=False)
        self.assertTrue(is_cogserver_running(), "Server should be running")

        # Try to start again - should raise RuntimeError
        with self.assertRaises(RuntimeError) as context:
            start_cogserver()

        self.assertIn("already running", str(context.exception))

        # Clean up
        stop_cogserver()

    def test_invalid_port_error(self):
        """Test that invalid port raises error."""
        # Try invalid port - should raise ValueError
        with self.assertRaises(ValueError) as context:
            start_cogserver(console_port=99999)

        self.assertIn("Invalid", str(context.exception))
        self.assertIn("port", str(context.exception))

        # Ensure server is not running
        self.assertFalse(is_cogserver_running(),
                        "Server should not be running after error")

    def test_stop_when_not_running(self):
        """Test stopping server when not running."""
        # Ensure server is not running
        self.assertFalse(is_cogserver_running(),
                        "Server should not be running")

        # Try to stop - should return False
        result = stop_cogserver()
        self.assertFalse(result,
                        "stop_cogserver should return False when not running")

    def test_get_atomspace_when_not_running(self):
        """Test getting atomspace when server not running."""
        # Ensure server is not running
        self.assertFalse(is_cogserver_running(),
                        "Server should not be running")

        # Try to get atomspace - should return None
        server_as = get_server_atomspace()
        self.assertIsNone(server_as,
                         "Should return None when server not running")

    def test_multiple_start_stop_cycles(self):
        """Test multiple start/stop cycles."""
        for i in range(3):
            # Start server
            result = start_cogserver(console_port=17560 + i,
                                    enable_web=False, enable_mcp=False)
            self.assertTrue(result, f"Server should start on cycle {i}")
            self.assertTrue(is_cogserver_running(),
                          f"Server should be running on cycle {i}")

            # Stop server
            result = stop_cogserver()
            self.assertTrue(result, f"Server should stop on cycle {i}")
            self.assertFalse(is_cogserver_running(),
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
