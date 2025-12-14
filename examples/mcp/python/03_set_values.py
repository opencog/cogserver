import os
import sys

sys.path.append(os.path.dirname(os.path.dirname(__file__)))

from mcp_client import MCPClient


client = MCPClient()

init_info = client.initialize()
print("Initialized:", init_info)

# Create the anchor and key atoms we will attach values to.
anchor_atom = '(AnchorNode "sample-anchor")'
note_key = '(PredicateNode "note")'
vector_key = '(PredicateNode "vector")'

client.call_tool("makeAtom", {"atomese": anchor_atom})
client.call_tool("makeAtom", {"atomese": note_key})
client.call_tool("makeAtom", {"atomese": vector_key})

# Attach a string value and a numeric vector value.
client.call_tool(
    "setValue",
    {
        "atomese": anchor_atom,
        "key": {"atomese": note_key},
        "value": {"atomese": '(StringValue "example note for anchor")'},
    },
)

client.call_tool(
    "setValue",
    {
        "atomese": anchor_atom,
        "key": {"atomese": vector_key},
        "value": {"atomese": "(FloatValue 1 2 3 4.5)"},
    },
)

# Read back what we stored.
keys = client.call_tool("getKeys", {"atomese": anchor_atom})
all_values = client.call_tool("getValues", {"atomese": anchor_atom})

print("Keys on anchor:", keys)
print("All values on anchor:", all_values)
