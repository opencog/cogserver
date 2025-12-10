import os
import sys

sys.path.append(os.path.dirname(os.path.dirname(__file__)))

from mcp_client import MCPClient


client = MCPClient()

init_info = client.initialize()
print("Initialized:", init_info)

# Discover tools and confirm makeAtom is available.
tools_info = client.list_tools()
tool_names = [tool.get("name") for tool in tools_info.get("tools", [])]
print("Available tools:", tool_names)
if "makeAtom" not in tool_names:
    raise SystemExit("makeAtom tool not available; check server configuration.")

# Create atoms
client.call_tool("makeAtom", {"atomese": '(Concept "Cat")'})
client.call_tool("makeAtom", {"atomese": '(Concept "Milk")'})
client.call_tool(
    "makeAtom",
    {"atomese": '(Edge (Predicate "likes") (List (Concept "Cat") (Concept "Milk")))'},
)

# Show all Concept atoms
concepts = client.call_tool("getAtoms", {"type": "Concept"})
print("Concept atoms:", concepts)
