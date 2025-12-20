
LINK_DIRECTORIES(
	${PROJECT_BINARY_DIR}/opencog/atomspace
	${PROJECT_BINARY_DIR}/opencog/cogserver/server
)

LINK_LIBRARIES(
	server
	servernode
	cogserver-types
	${ATOMSPACE_LIBRARIES}
)

ADD_CXXTEST(MCPUTest)

# Set COGSERVER_MODULE_PATH so modules can be found in the build directory
SET_PROPERTY(TEST MCPUTest APPEND PROPERTY ENVIRONMENT
	"COGSERVER_MODULE_PATH=${PROJECT_BINARY_DIR}/opencog/cogserver/modules:${PROJECT_BINARY_DIR}/opencog/cogserver/shell")
