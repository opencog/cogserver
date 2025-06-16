#!/usr/bin/env python3
#
# unix_to_tcp_proxy.py
#
# Proxy server that passes messages between a unix domain socket and a
# TCP/IP socket. When used in conjunction with stdio_to_unix_proxy.py
# it can be used to bypass networking restrictions that can appear in
# certain cases. (Bugs, actually. See
# https://github.com/anthropics/claude-code/issues/1536 for details.)
#
# The reads requests coming across the unix domain socket /tmp/echo_socket
# and passes them to the CogServer located at localhost:18888. Override
# the host and port with the --remote-host and --remote-port flags.
#
# Be sure that the CogServer is running on that host at that port.
#
import socket
import os
import sys
import argparse
import threading
import time
import select
import logging
from datetime import datetime

SOCKET_PATH = "/tmp/echo_socket"
BUFFER_SIZE = 65536

def setup_logging(verbose=False):
    """Setup logging configuration"""
    level = logging.DEBUG if verbose else logging.INFO
    logging.basicConfig(
        level=level,
        format='%(asctime)s [%(levelname)s] %(message)s',
        datefmt='%Y-%m-%d %H:%M:%S'
    )
    return logging.getLogger(__name__)

def parse_args():
    parser = argparse.ArgumentParser(
        description='Enhanced Unix to TCP proxy with bidirectional threading'
    )
    parser.add_argument('--remote-host', default='localhost',
                        help='Remote TCP server host')
    parser.add_argument('--remote-port', type=int, default=18888,
                        help='Remote TCP server port')
    parser.add_argument('--socket-path', default=SOCKET_PATH,
                        help='Unix domain socket path')
    parser.add_argument('--buffer-size', type=int, default=BUFFER_SIZE,
                        help='Buffer size for socket reads')
    parser.add_argument('--verbose', '-v', action='store_true',
                        help='Enable verbose logging')
    parser.add_argument('--timeout', type=float, default=None,
                        help='Socket timeout in seconds (None for blocking)')
    parser.add_argument('--keepalive', action='store_true',
                        help='Enable TCP keepalive')
    return parser.parse_args()

class ProxyThread(threading.Thread):
    """Base class for proxy threads with common functionality"""

    def __init__(self, source_socket, dest_socket, direction, buffer_size=BUFFER_SIZE):
        super().__init__(name=direction)
        self.source_socket = source_socket
        self.dest_socket = dest_socket
        self.direction = direction
        self.buffer_size = buffer_size
        self.running = True
        self.bytes_transferred = 0
        self.messages_transferred = 0
        self.logger = logging.getLogger(__name__)

    def stop(self):
        """Signal the thread to stop"""
        self.running = False

    def run(self):
        """Main thread loop"""
        self.logger.info(f"[{self.direction}] Thread started")

        try:
            while self.running:
                # Use select to check if data is available with timeout
                readable, _, _ = select.select([self.source_socket], [], [], 0.1)

                if not readable:
                    continue

                # Receive data from source
                try:
                    data = self.source_socket.recv(self.buffer_size)
                except socket.timeout:
                    continue
                except socket.error as e:
                    if e.errno == 9:  # Bad file descriptor (socket closed)
                        break
                    raise

                if not data:
                    self.logger.info(f"[{self.direction}] Source socket closed")
                    break

                # Process and forward data
                self.process_and_forward(data)

        except Exception as e:
            self.logger.error(f"[{self.direction}] Error: {e}")
        finally:
            self.logger.info(f"[{self.direction}] Thread shutting down")
            self.logger.info(f"[{self.direction}] Transferred {self.bytes_transferred} bytes "
                           f"in {self.messages_transferred} messages")

    def process_and_forward(self, data):
        """Process and forward data (can be overridden in subclasses)"""
        self.bytes_transferred += len(data)
        self.messages_transferred += 1

        # Log the data being forwarded
        try:
            message = data.decode('utf-8', errors='ignore').strip()
            if message:
                self.logger.debug(f"[{self.direction}] {message}")
        except:
            self.logger.debug(f"[{self.direction}] Binary data: {len(data)} bytes")

        # Forward data to destination
        try:
            self.dest_socket.sendall(data)
        except socket.error as e:
            self.logger.error(f"[{self.direction}] Failed to send data: {e}")
            raise

class UnixToTcpThread(ProxyThread):
    """Thread for Unix to TCP direction with newline appending"""

    def process_and_forward(self, data):
        """Append newline before forwarding to TCP"""
        # Check if data already ends with newline
        if not data.endswith(b'\n'):
            data = data + b'\n'

        super().process_and_forward(data)

class TcpToUnixThread(ProxyThread):
    """Thread for TCP to Unix direction with message buffering"""

    def __init__(self, source_socket, dest_socket, direction, buffer_size=BUFFER_SIZE):
        super().__init__(source_socket, dest_socket, direction, buffer_size)
        self.message_buffer = b''

    def run(self):
        """Main thread loop with message buffering"""
        self.logger.info(f"[{self.direction}] Thread started with message buffering")

        try:
            while self.running:
                # Use select to check if data is available with timeout
                readable, _, _ = select.select([self.source_socket], [], [], 0.1)

                if not readable:
                    continue

                # Receive data from source
                try:
                    data = self.source_socket.recv(self.buffer_size)
                except socket.timeout:
                    continue
                except socket.error as e:
                    if e.errno == 9:  # Bad file descriptor (socket closed)
                        break
                    raise

                if not data:
                    self.logger.info(f"[{self.direction}] Source socket closed")
                    # Send any remaining buffered data
                    if self.message_buffer:
                        self.logger.warning(f"[{self.direction}] Incomplete message in buffer: {len(self.message_buffer)} bytes")
                        self.process_and_forward(self.message_buffer)
                    break

                # Add data to buffer
                self.message_buffer += data

                # Process complete messages (newline-terminated)
                while b'\n' in self.message_buffer:
                    newline_pos = self.message_buffer.find(b'\n')
                    complete_message = self.message_buffer[:newline_pos + 1]
                    self.message_buffer = self.message_buffer[newline_pos + 1:]

                    # Forward the complete message
                    self.process_and_forward(complete_message)

        except Exception as e:
            self.logger.error(f"[{self.direction}] Error: {e}")
        finally:
            self.logger.info(f"[{self.direction}] Thread shutting down")
            self.logger.info(f"[{self.direction}] Transferred {self.bytes_transferred} bytes "
                           f"in {self.messages_transferred} messages")

class ConnectionHandler:
    """Handles a single client connection with bidirectional communication"""

    def __init__(self, client_socket, remote_host, remote_port, args):
        self.client_socket = client_socket
        self.remote_host = remote_host
        self.remote_port = remote_port
        self.args = args
        self.logger = logging.getLogger(__name__)
        self.threads = []

    def handle(self):
        """Handle the client connection"""
        remote_socket = None

        try:
            # Connect to remote TCP server
            remote_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)

            # Set socket options
            if self.args.keepalive:
                remote_socket.setsockopt(socket.SOL_SOCKET, socket.SO_KEEPALIVE, 1)

            if self.args.timeout:
                remote_socket.settimeout(self.args.timeout)

            remote_socket.connect((self.remote_host, self.remote_port))
            self.logger.info(f"Connected to remote server {self.remote_host}:{self.remote_port}")

            # Create bidirectional threads
            unix_to_tcp = UnixToTcpThread(
                self.client_socket, remote_socket,
                "Unix→TCP", self.args.buffer_size
            )
            tcp_to_unix = TcpToUnixThread(
                remote_socket, self.client_socket,
                "TCP→Unix", self.args.buffer_size
            )

            self.threads = [unix_to_tcp, tcp_to_unix]

            # Start both threads
            for thread in self.threads:
                thread.daemon = True
                thread.start()

            # Monitor threads
            while any(thread.is_alive() for thread in self.threads):
                time.sleep(0.1)

            # Signal all threads to stop
            for thread in self.threads:
                thread.stop()

        except Exception as e:
            self.logger.error(f"Connection error: {e}")
        finally:
            self.cleanup(remote_socket)

    def cleanup(self, remote_socket):
        """Clean up connections and threads"""
        self.logger.info("Cleaning up connections...")

        # Wait briefly for threads to finish
        for thread in self.threads:
            thread.join(timeout=1.0)

        # Close sockets
        for sock, name in [(remote_socket, "TCP"), (self.client_socket, "Unix")]:
            if sock:
                try:
                    sock.shutdown(socket.SHUT_RDWR)
                except:
                    pass
                try:
                    sock.close()
                except:
                    pass
                self.logger.debug(f"{name} socket closed")

        self.logger.info("Client disconnected")

def main():
    args = parse_args()
    logger = setup_logging(args.verbose)

    # Update socket path if provided
    socket_path = args.socket_path

    # Remove the socket file if it already exists
    if os.path.exists(socket_path):
        os.remove(socket_path)

    # Create Unix domain socket
    server_socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    server_socket.bind(socket_path)
    server_socket.listen(5)  # Allow up to 5 pending connections

    logger.info(f"Server listening on {socket_path}")
    logger.info(f"Will forward to {args.remote_host}:{args.remote_port}")
    logger.info(f"Buffer size: {args.buffer_size} bytes")
    if args.timeout:
        logger.info(f"Socket timeout: {args.timeout} seconds")
    if args.keepalive:
        logger.info("TCP keepalive enabled")

    try:
        while True:
            # Accept client connection
            client_socket, _ = server_socket.accept()
            logger.info("Client connected")

            # Handle each client in a separate thread
            handler = ConnectionHandler(
                client_socket, args.remote_host, args.remote_port, args
            )
            client_thread = threading.Thread(target=handler.handle)
            client_thread.daemon = True
            client_thread.start()

    except KeyboardInterrupt:
        logger.info("\nServer shutting down...")
    except Exception as e:
        logger.error(f"Server error: {e}")
    finally:
        server_socket.close()
        if os.path.exists(socket_path):
            os.remove(socket_path)
        logger.info("Server stopped")

if __name__ == "__main__":
    main()
