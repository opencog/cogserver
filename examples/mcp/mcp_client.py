import requests


class MCPClient:
    """Minimal MCP HTTP client for CogServer."""

    def __init__(self, url="http://localhost:18080/mcp"):
        self.url = url
        self.req_id = 1
        self.initialized = False

    def _post(self, payload, expect_response=True):
        response = requests.post(self.url, json=payload, timeout=30)
        response.raise_for_status()

        if not expect_response:
            return None

        data = response.json()
        if "error" in data:
            raise RuntimeError(data["error"])
        return data.get("result")

    def _rpc(self, method, params=None):
        payload = {"jsonrpc": "2.0", "id": self.req_id, "method": method}
        if params:
            payload["params"] = params
        self.req_id += 1
        return self._post(payload)

    def _notify(self, method, params=None):
        payload = {"jsonrpc": "2.0", "method": method}
        if params:
            payload["params"] = params
        # HTTP transport may still return a JSON body; ignore it.
        self._post(payload, expect_response=True)

    def initialize(self, client_name="PythonExample", client_version="0.1.0"):
        result = self._rpc(
            "initialize",
            {
                "protocolVersion": "2025-06-18",
                "capabilities": {},
                "clientInfo": {"name": client_name, "version": client_version},
            },
        )
        try:
            self._notify("notifications/initialized")
        finally:
            self.initialized = True
        return result

    def list_tools(self):
        return self._rpc("tools/list")

    def call_tool(self, name, arguments=None):
        params = {"name": name}
        if arguments:
            params["arguments"] = arguments
        return self._rpc("tools/call", params)

    def list_resources(self):
        return self._rpc("resources/list")

    def read_resource(self, uri):
        return self._rpc("resources/read", {"uri": uri})
