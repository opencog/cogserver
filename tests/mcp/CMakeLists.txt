
LINK_DIRECTORIES(
	${PROJECT_BINARY_DIR}/opencog/atomspace
	${PROJECT_BINARY_DIR}/opencog/cogserver/server
)

LINK_LIBRARIES(
	server
	${ATOMSPACE_LIBRARIES}
	${Boost_SYSTEM_LIBRARY}
)

ADD_CXXTEST(MCPUTest)
