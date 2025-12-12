
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
