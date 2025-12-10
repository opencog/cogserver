import os
import sys

sys.path.append(os.path.dirname(os.path.dirname(__file__)))

from mcp_client import MCPClient


def ensure_atom(client: MCPClient, atomese: str):
    """Create an atom idempotently (server ignores duplicates)."""
    client.call_tool("makeAtom", {"atomese": atomese})


client = MCPClient()

init_info = client.initialize()
print("Initialized:", init_info)

# Check tool availability.
tools = {t.get("name") for t in client.list_tools().get("tools", [])}
if "getIncoming" not in tools:
    raise SystemExit("getIncoming tool not available; check server configuration.")

# Build a small graph: Alice likes Bob; Bob likes Milk.
ensure_atom(client, '(Concept "Alice")')
ensure_atom(client, '(Concept "Bob")')
ensure_atom(client, '(Concept "Milk")')

ensure_atom(client, '(Edge (Predicate "likes") (List (Concept "Alice") (Concept "Bob")) )')
ensure_atom(client, '(Edge (Predicate "likes") (List (Concept "Bob") (Concept "Milk")) )')

# Query incoming links for Bob and Milk.
incoming_bob = client.call_tool("getIncoming", {"atomese": '(Concept "Bob")'})
incoming_milk = client.call_tool("getIncoming", {"atomese": '(Concept "Milk")'})

print("Incoming links for Bob:", incoming_bob)
print("Incoming links for Milk:", incoming_milk)
