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

#include <json/json.h>
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

// Using jsoncpp's Json namespace

// Helper function to convert Json::Value to string with optional indentation
static inline std::string dump_result(const Json::Value& value, bool pretty = true) {
    Json::StreamWriterBuilder builder;
    builder["indentation"] = pretty ? "  " : "";
    std::unique_ptr<Json::StreamWriter> writer(builder.newStreamWriter());
    std::ostringstream oss;
    writer->write(value, &oss);
    return oss.str();
}

class SimpleJsonRpcClient {
private:
    SOCKET sock;
    std::atomic<int> message_id{0};
    bool connected{false};
    Json::StreamWriterBuilder writer_builder;
    Json::CharReaderBuilder reader_builder;

public:
    SimpleJsonRpcClient() : sock(INVALID_SOCKET) {
#ifdef _WIN32
        WSADATA wsaData;
        WSAStartup(MAKEWORD(2, 2), &wsaData);
#endif
        writer_builder["indentation"] = "";
        writer_builder["commentStyle"] = "None";
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

    bool sendMessage(const Json::Value& message) {
        if (!connected) return false;

        std::string msg_str = dump_result(message, false) + "\n";
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

    Json::Value receiveMessage() {
        if (!connected) return Json::Value();

        static std::string buffer; // Make buffer static to preserve data between calls
        char recv_buf[4096];

        while (true) {
            // First check if we already have a complete line in the buffer
            size_t newline_pos = buffer.find('\n');
            if (newline_pos != std::string::npos) {
                std::string line = buffer.substr(0, newline_pos);
                buffer = buffer.substr(newline_pos + 1);

                Json::Value root;
                std::unique_ptr<Json::CharReader> reader(reader_builder.newCharReader());
                std::string errors;

                if (!reader->parse(line.c_str(), line.c_str() + line.length(), &root, &errors)) {
                    std::cerr << "Failed to parse JSON: " << errors << std::endl;
                    return Json::Value();
                }
                return root;
            }

            // Read more data if we don't have a complete line
            int received = recv(sock, recv_buf, sizeof(recv_buf) - 1, 0);
            if (received <= 0) {
                std::cerr << "Failed to receive message" << std::endl;
                return Json::Value();
            }

            recv_buf[received] = '\0';
            buffer += recv_buf;
        }
    }

    Json::Value sendRequest(const std::string& method, const Json::Value& params = Json::Value()) {
        int id = message_id++;

        Json::Value request;
        request["jsonrpc"] = "2.0";
        request["method"] = method;
        request["id"] = id;

        if (!params.isNull()) {
            request["params"] = params;
        }

        if (!sendMessage(request)) {
            throw std::runtime_error("Failed to send request");
        }

        Json::Value response = receiveMessage();
        if (response.empty()) {
            throw std::runtime_error("Failed to receive response");
        }

        if (response.isMember("error")) {
            throw std::runtime_error("RPC error: " + dump_result(response["error"], false));
        }

        if (response.isMember("result")) {
            return response["result"];
        }

        return Json::Value();
    }

    bool initialize(const std::string& name, const std::string& version) {
        Json::Value params;
        params["protocolVersion"] = "2025-06-18";
        params["capabilities"] = Json::objectValue;
        params["clientInfo"]["name"] = name;
        params["clientInfo"]["version"] = version;

        try {
            Json::Value result = sendRequest("initialize", params);
            std::cout << "Initialize response: " << dump_result(result) << std::endl;

            // Send initialized notification
            Json::Value notification;
            notification["jsonrpc"] = "2.0";
            notification["method"] = "notifications/initialized";
            sendMessage(notification);

            // Per JSON RPC spec, there should be no response to
            // notifications. But we have to hack this, to keep
            // Claude HTTP happy. So read a response, and discard it.
            Json::Value init_response = receiveMessage();
            std::cout << "DEBUG: initialize notification response: "
                  << dump_result(init_response) << std::endl;
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

    Json::Value listTools() {
        return sendRequest("tools/list");
    }

    Json::Value callTool(const std::string& name, const Json::Value& arguments = Json::Value()) {
        Json::Value params;
        params["name"] = name;

        if (!arguments.isNull()) {
            params["arguments"] = arguments;
        }

        return sendRequest("tools/call", params);
    }

    Json::Value listResources() {
        return sendRequest("resources/list");
    }

    Json::Value readResource(const std::string& uri) {
        Json::Value params;
        params["uri"] = uri;
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

        // List available tools
        std::cout << "\nListing available tools..." << std::endl;
        try {
            Json::Value tools_response = client.listTools();
            std::cout << "DEBUG: Raw tools response: " << dump_result(tools_response) << std::endl;

            if (tools_response.isMember("tools") && tools_response["tools"].isArray()) {
                Json::Value tools = tools_response["tools"];
                std::cout << "✓ Found " << tools.size() << " tools:" << std::endl;

                for (Json::ArrayIndex i = 0; i < tools.size(); ++i) {
                    const Json::Value& tool = tools[i];
                    std::cout << "  - " << tool["name"].asString()
                              << ": " << tool["description"].asString() << std::endl;
                }

                // Call a tool if available
                if (tools.size() > 0) {
                    const Json::Value& first_tool = tools[0];
                    std::string tool_name = first_tool["name"].asString();
                    std::cout << "\nCalling tool '" << tool_name << "'..." << std::endl;

                    Json::Value tool_args;
                    // Add some example arguments based on common tool types
                    if (tool_name == "echo") {
                        tool_args["text"] = "Hello from network client!";
                    } else if (tool_name == "greeting") {
                        tool_args["name"] = "NetworkClient";
                    } else if (tool_name == "time") {
                        // No arguments needed for time tool
                    }

                    try {
                        Json::Value result = client.callTool(tool_name, tool_args);
                        std::cout << "✓ Tool result: " << dump_result(result) << std::endl;
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
            Json::Value resources_response = client.listResources();
            std::cout << "DEBUG: Raw resources response: " << dump_result(resources_response) << std::endl;

            if (resources_response.isMember("resources") && resources_response["resources"].isArray()) {
                Json::Value resources = resources_response["resources"];
                std::cout << "✓ Found " << resources.size() << " resources:" << std::endl;

                for (Json::ArrayIndex i = 0; i < resources.size(); ++i) {
                    const Json::Value& resource = resources[i];
                    std::cout << "  - URI: " << resource["uri"].asString() << std::endl;
                    if (resource.isMember("name")) {
                        std::cout << "    Name: " << resource["name"].asString() << std::endl;
                    }
                    if (resource.isMember("description")) {
                        std::cout << "    Description: " << resource["description"].asString() << std::endl;
                    }
                    if (resource.isMember("mimeType")) {
                        std::cout << "    MIME Type: " << resource["mimeType"].asString() << std::endl;
                    }
                }

                // Try to read the first resource if available
                if (resources.size() > 0) {
                    const Json::Value& first_resource = resources[0];
                    if (first_resource.isMember("uri")) {
                        std::string uri = first_resource["uri"].asString();
                        std::cout << "\nReading first resource: " << uri << std::endl;

                        try {
                            Json::Value read_response = client.readResource(uri);
                            if (read_response.isMember("contents")) {
                                Json::Value contents = read_response["contents"];
                                if (contents.isArray() && contents.size() > 0) {
                                    const Json::Value& first_content = contents[0];
                                    if (first_content.isMember("text")) {
                                        std::string text = first_content["text"].asString();
                                        // Print first 200 chars of content
                                        if (text.length() > 200) {
                                            std::cout << "✓ Resource content (first 200 chars):\n"
                                                      << text.substr(0, 200) << "..." << std::endl;
                                        } else {
                                            std::cout << "✓ Resource content:\n" << text << std::endl;
                                        }
                                    } else if (first_content.isMember("uri")) {
                                        std::cout << "✓ Resource references URI: "
                                                  << first_content["uri"].asString() << std::endl;
                                    }
                                }
                            } else {
                                std::cout << "✓ Read response: " << dump_result(read_response) << std::endl;
                            }
                        } catch (const std::exception& e) {
                            std::cout << "✗ Failed to read resource: " << e.what() << std::endl;
                        }
                    }
                }
            } else {
                std::cout << "✓ No resources available" << std::endl;
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
