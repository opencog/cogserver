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

SOCKET_PATH = "/tmp/echo_socket"

def parse_args():
    parser = argparse.ArgumentParser(description='Unix socket proxy server')
    parser.add_argument('--remote-host', default='localhost', help='Remote TCP server host')
    parser.add_argument('--remote-port', type=int, default=18888, help='Remote TCP server port')
    return parser.parse_args()

def main():
    args = parse_args()

    # Remove the socket file if it already exists
    if os.path.exists(SOCKET_PATH):
        os.remove(SOCKET_PATH)

    # Create Unix domain socket
    server_socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)
    server_socket.bind(SOCKET_PATH)
    server_socket.listen(1)

    print(f"Server listening on {SOCKET_PATH}")
    print(f"Will forward to {args.remote_host}:{args.remote_port}")

    try:
        while True:
            # Accept client connection
            client_socket, _ = server_socket.accept()
            print("Client connected")

            # Connect to remote TCP server
            remote_socket = None
            try:
                remote_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
                remote_socket.connect((args.remote_host, args.remote_port))
                print(f"Connected to remote server {args.remote_host}:{args.remote_port}")

                while True:
                    # Receive data from client
                    data = client_socket.recv(4096)
                    if not data:
                        break

                    # Print the data being forwarded
                    message = data.decode('utf-8', errors='ignore').strip()
                    print(f"[CLIENT -> REMOTE] {message}")

                    # Append newline to data before forwarding to remote server
                    data_with_newline = data + b'\n'

                    # Forward data to remote server
                    remote_socket.send(data_with_newline)

                    # Receive response from remote server
                    response_data = remote_socket.recv(4096)
                    if not response_data:
                        break

                    # Print the response being returned
                    response = response_data.decode('utf-8', errors='ignore').strip()
                    print(f"[REMOTE -> CLIENT] {response}")

                    # Send response back to client
                    client_socket.send(response_data)

            except Exception as e:
                print(f"Error handling client/remote connection: {e}")
            finally:
                if remote_socket:
                    remote_socket.close()
                client_socket.close()
                print("Client disconnected")

    except KeyboardInterrupt:
        print("\nServer shutting down...")
    finally:
        server_socket.close()
        if os.path.exists(SOCKET_PATH):
            os.remove(SOCKET_PATH)

if __name__ == "__main__":
    main()
