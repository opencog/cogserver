/**
 * @file mcp-checker.cc
 * @brief Demo MCP client connecting to server via TCP
 *
 * This implements a simple MCP test client that can be used to verify
 * that the MCP network server is available, and is responding to
 * commands.  It will list the tools and resources provided by the
 * MCP server. Aim it at a running instance of the CogServer.
 *
 * (This code was created by Claude.)
 */

#include <nlohmann/json.hpp>
#include <iostream>
#include <string>
#include <sstream>
#include <cstring>
#include <thread>
#include <chrono>
#include <atomic>

// Socket includes
#ifdef _WIN32
    #include <winsock2.h>
    #include <ws2tcpip.h>
    #pragma comment(lib, "ws2_32.lib")
    typedef int socklen_t;
#else
    #include <sys/socket.h>
    #include <arpa/inet.h>
    #include <netinet/in.h>
    #include <unistd.h>
    #include <netdb.h>
    #define INVALID_SOCKET -1
    #define SOCKET_ERROR -1
    #define closesocket close
    typedef int SOCKET;
#endif

using json = nlohmann::json;

class SimpleJsonRpcClient {
private:
    SOCKET sock;
    std::atomic<int> message_id{0};
    bool connected{false};

public:
    SimpleJsonRpcClient() : sock(INVALID_SOCKET) {
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
    }

    ~SimpleJsonRpcClient() {
        disconnect();
#ifdef _WIN32
        WSACleanup();
#endif
    }

    bool connect(const std::string& host, int port) {
        // Create socket
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock == INVALID_SOCKET) {
            std::cerr << "Failed to create socket" << std::endl;
            return false;
        }

        // Resolve hostname
        struct hostent* server = gethostbyname(host.c_str());
        if (server == nullptr) {
            std::cerr << "Failed to resolve hostname: " << host << std::endl;
            closesocket(sock);
            sock = INVALID_SOCKET;
            return false;
        }

        // Setup address structure
        struct sockaddr_in serv_addr;
        memset(&serv_addr, 0, sizeof(serv_addr));
        serv_addr.sin_family = AF_INET;
        memcpy(&serv_addr.sin_addr.s_addr, server->h_addr, server->h_length);
        serv_addr.sin_port = htons(port);

        // Connect to server
        if (::connect(sock, (struct sockaddr*)&serv_addr, sizeof(serv_addr)) < 0) {
            std::cerr << "Failed to connect to " << host << ":" << port << std::endl;
            closesocket(sock);
            sock = INVALID_SOCKET;
            return false;
        }

        connected = true;
        return true;
    }

    void disconnect() {
        if (sock != INVALID_SOCKET) {
            closesocket(sock);
            sock = INVALID_SOCKET;
        }
        connected = false;
    }

    bool sendMessage(const json& message) {
        if (!connected) return false;

        std::string msg_str = message.dump() + "\n";
        int total_sent = 0;
        int msg_len = msg_str.length();

        while (total_sent < msg_len) {
            int sent = send(sock, msg_str.c_str() + total_sent, msg_len - total_sent, 0);
            if (sent <= 0) {
                std::cerr << "Failed to send message" << std::endl;
                return false;
            }
            total_sent += sent;
        }

        return true;
    }

    json receiveMessage() {
        if (!connected) return json();

        std::string buffer;
        char recv_buf[4096];

        while (true) {
            int received = recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);
            if (received <= 0) {
                std::cerr << "Failed to receive message" << std::endl;
                return json();
            }

            recv_buf[received] = '\0';
            buffer += recv_buf;

            // Check if we have a complete line
            size_t newline_pos = buffer.find('\n');
            if (newline_pos != std::string::npos) {
                std::string line = buffer.substr(0, newline_pos);
                buffer = buffer.substr(newline_pos + 1);

                try {
                    return json::parse(line);
                } catch (const json::parse_error& e) {
                    std::cerr << "Failed to parse JSON: " << e.what() << std::endl;
                    return json();
                }
            }
        }
    }

    json sendRequest(const std::string& method, const json& params = json()) {
        int id = message_id++;

        json request = {
            {"jsonrpc", "2.0"},
            {"method", method},
            {"id", id}
        };

        if (!params.is_null()) {
            request["params"] = params;
        }

        if (!sendMessage(request)) {
            throw std::runtime_error("Failed to send request");
        }

        json response = receiveMessage();
        if (response.empty()) {
            throw std::runtime_error("Failed to receive response");
        }

        if (response.contains("error")) {
            throw std::runtime_error("RPC error: " + response["error"].dump());
        }

        if (response.contains("result")) {
            return response["result"];
        }

        return json();
    }

    bool initialize(const std::string& name, const std::string& version) {
        json params = {
            {"protocolVersion", "2024-11-05"},
            {"capabilities", json::object()},
            {"clientInfo", {
                {"name", name},
                {"version", version}
            }}
        };

        try {
            json result = sendRequest("initialize", params);
            std::cout << "Initialize response: " << result.dump(2) << std::endl;

            // Send initialized notification
            json notification = {
                {"jsonrpc", "2.0"},
                {"method", "notifications/initialized"}
            };
            sendMessage(notification);

            return true;
        } catch (const std::exception& e) {
            std::cerr << "Initialize failed: " << e.what() << std::endl;
            return false;
        }
    }

    bool ping() {
        try {
            sendRequest("ping");
            return true;
        } catch (...) {
            return false;
        }
    }

    json getServerCapabilities() {
        return sendRequest("mcp/describe");
    }

    json listTools() {
        return sendRequest("tools/list");
    }

    json callTool(const std::string& name, const json& arguments = json()) {
        json params = {
            {"name", name}
        };

        if (!arguments.is_null()) {
            params["arguments"] = arguments;
        }

        return sendRequest("tools/call", params);
    }

    json listResources() {
        return sendRequest("resources/list");
    }

    json readResource(const std::string& uri) {
        json params = {
            {"uri", uri}
        };
        return sendRequest("resources/read", params);
    }
};

void print_usage(const char* program_name) {
    std::cout << "Usage: " << program_name << " [options]\n"
              << "Options:\n"
              << "  -h, --host <host>     Server host (default: localhost)\n"
              << "  -p, --port <port>     Server port (default: 18888)\n"
              << "  --help               Show this help message\n"
              << "\n"
              << "Example:\n"
              << "  " << program_name << " --host localhost --port 18888\n"
              << "\n"
              << "This client connects to an existing MCP server running on the specified\n"
              << "host and port, and lists the available tools and resources.\n";
}

int main(int argc, char* argv[]) {
    std::string host = "localhost";
    int port = 18888;

    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        std::string arg = argv[i];

        if (arg == "--help") {
            print_usage(argv[0]);
            return 0;
        } else if ((arg == "-h" || arg == "--host") && i + 1 < argc) {
            host = argv[++i];
        } else if ((arg == "-p" || arg == "--port") && i + 1 < argc) {
            port = std::stoi(argv[++i]);
        } else {
            std::cerr << "Unknown argument: " << arg << std::endl;
            print_usage(argv[0]);
            return 1;
        }
    }

    std::cout << "MCP Network Client Example\n";
    std::cout << "Connecting to: " << host << ":" << port << std::endl;

    try {
        SimpleJsonRpcClient client;

        // Connect to server
        if (!client.connect(host, port)) {
            std::cerr << "Failed to connect to server" << std::endl;
            return 1;
        }

        std::cout << "✓ Connected to server" << std::endl;

        // Initialize the client
        std::cout << "\nInitializing client..." << std::endl;
        if (!client.initialize("NetworkClientJsonOnly", "1.0.0")) {
            std::cerr << "Failed to initialize client" << std::endl;
            return 1;
        }

        std::cout << "✓ Client initialized successfully" << std::endl;

        // Test ping
        std::cout << "\nTesting ping..." << std::endl;
        if (client.ping()) {
            std::cout << "✓ Ping successful" << std::endl;
        } else {
            std::cout << "✗ Ping failed" << std::endl;
        }

        // Get server capabilities
        std::cout << "\nGetting server capabilities..." << std::endl;
        try {
            json capabilities = client.getServerCapabilities();
            std::cout << "✓ Server capabilities: " << capabilities.dump(2) << std::endl;
        } catch (const std::exception& e) {
            std::cout << "✗ Failed to get server capabilities: " << e.what() << std::endl;
        }

        // List available tools
        std::cout << "\nListing available tools..." << std::endl;
        try {
            json tools_response = client.listTools();

            if (tools_response.contains("tools") && tools_response["tools"].is_array()) {
                auto tools = tools_response["tools"];
                std::cout << "✓ Found " << tools.size() << " tools:" << std::endl;

                for (const auto& tool : tools) {
                    std::cout << "  - " << tool["name"].get<std::string>()
                              << ": " << tool["description"].get<std::string>() << std::endl;
                }

                // Call a tool if available
                if (!tools.empty()) {
                    const auto& first_tool = tools[0];
                    std::string tool_name = first_tool["name"].get<std::string>();
                    std::cout << "\nCalling tool '" << tool_name << "'..." << std::endl;

                    json tool_args;
                    // Add some example arguments based on common tool types
                    if (tool_name == "echo") {
                        tool_args["text"] = "Hello from network client!";
                    } else if (tool_name == "greeting") {
                        tool_args["name"] = "NetworkClient";
                    } else if (tool_name == "time") {
                        // No arguments needed for time tool
                    }

                    try {
                        json result = client.callTool(tool_name, tool_args);
                        std::cout << "✓ Tool result: " << result.dump(2) << std::endl;
                    } catch (const std::exception& e) {
                        std::cout << "✗ Tool call failed: " << e.what() << std::endl;
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cout << "✗ Failed to list tools: " << e.what() << std::endl;
        }

        // List available resources
        std::cout << "\nListing available resources..." << std::endl;
        try {
            json resources = client.listResources();
            std::cout << "✓ Resources: " << resources.dump(2) << std::endl;

            // Try to read a resource if available
            if (resources.contains("resources") && resources["resources"].is_array() &&
                !resources["resources"].empty()) {

                const auto& first_resource = resources["resources"][0];
                if (first_resource.contains("uri")) {
                    std::string uri = first_resource["uri"].get<std::string>();
                    std::cout << "\nReading resource: " << uri << std::endl;

                    try {
                        json content = client.readResource(uri);
                        std::cout << "✓ Resource content: " << content.dump(2) << std::endl;
                    } catch (const std::exception& e) {
                        std::cout << "✗ Failed to read resource: " << e.what() << std::endl;
                    }
                }
            }
        } catch (const std::exception& e) {
            std::cout << "✗ Failed to list resources: " << e.what() << std::endl;
        }

        std::cout << "\n✓ All operations completed successfully!" << std::endl;
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return 1;
    }

    return 0;
}
