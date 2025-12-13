LINK_LIBRARIES(
	server
	servernode
	cogserver-types
	${ATOMSPACE_LIBRARIES}
	${COGUTIL_LIBRARIES}
)

ADD_CXXTEST(HttpUTest)
ADD_CXXTEST(WebSocketUTest)
