#!/usr/bin/env python3
#
# stdio_to_unix_proxy.py
#
# Proxy to connect stdio to a unix socket. This is used to evade network
# connectivity issues that can arise due to technical issues (bugs,
# actually; see https://github.com/anthropics/claude-code/issues/1536)
#
# Reads from stdin, copies to the Unix domain socket /tmp/echo_socket
# then turns around and reads from /tmp/echo_socket and passes that to
# stdout.  This is tailored for MCP/JSON, in that it is careful to
# strip off the whitespace that drives JSON crazy.  It also assumes
# reads and writes are interleaved, one-to-one, synchronous. It is
# currently limited to messages that are 4KBytes in length.
#
# Example usage:
# claude mcp list
# claude mcp add cogserv /home/opencog/stdio_to_unix_proxy.py
#
# Be sure to run unix_to_tcp_proxy.py in another shell, so as to forward
# the MCP requests and responses to the CogServer.  Be sure to run the
# CogServer as well.
#
import socket
import sys

SOCKET_PATH = "/tmp/echo_socket"

def main():
    try:
        # Create Unix domain socket
        client_socket = socket.socket(socket.AF_UNIX, socket.SOCK_STREAM)

        # Connect to server
        try:
            client_socket.connect(SOCKET_PATH)
        except socket.error as e:
            print(f"Error connecting to server: {e}", file=sys.stderr)
            print("Make sure the server is running first.", file=sys.stderr)
            sys.exit(1)

        print("Connected to server. Type messages (Ctrl+D to exit):", file=sys.stderr)

        try:
            # Read from stdin and send to server
            for line in sys.stdin:
                message = line.strip()
                if message:
                    # Send message to server
                    client_socket.send(message.encode('utf-8'))

                    # Receive response from server
                    response = client_socket.recv(4096)
                    response = response.strip()
                    if response:
                        # Print response to stdout
                        print(response.decode('utf-8'))
                        sys.stdout.flush()
                    else:
                        print("Server closed connection", file=sys.stderr)
                        break

        except KeyboardInterrupt:
            print("\nClient shutting down...", file=sys.stderr)

    except Exception as e:
        print(f"Client error: {e}", file=sys.stderr)
    finally:
        client_socket.close()

if __name__ == "__main__":
    main()
