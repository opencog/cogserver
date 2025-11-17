INCLUDE_DIRECTORIES (
	${PROJECT_SOURCE_DIR}/opencog/util
	${PROJECT_SOURCE_DIR}/opencog/atomspace
	${PROJECT_SOURCE_DIR}/opencog/cogserver/server
)

LINK_DIRECTORIES(
	${PROJECT_BINARY_DIR}/opencog/atomspace
	${PROJECT_BINARY_DIR}/opencog/cogserver/server
)

LINK_LIBRARIES(
	server
	${ATOMSPACE_LIBRARIES}
)

ADD_CXXTEST(JsonShellUTest)
TARGET_LINK_LIBRARIES(JsonShellUTest json-shell)

IF (HAVE_MCP)
	ADD_CXXTEST(McpShellUTest)
	TARGET_LINK_LIBRARIES(McpShellUTest mcp-shell mcp-eval)
ENDIF (HAVE_MCP)

ADD_CXXTEST(IPv6UTest)
ADD_CXXTEST(ShellUTest)
