# ------------------------------------------------------------

ADD_LIBRARY (server SHARED
	CogServer.cc
	MCPServer.cc
	ModuleManager.cc
	PageServer.cc
	Request.cc
	RequestManager.cc
	ServerConsole.cc
	WebServer.cc
)

TARGET_LINK_LIBRARIES(server
	network
	${ATOMSPACE_LIBRARIES}
	${COGUTIL_LIBRARY}
	${OPENSSL_LIBRARIES}
	dl
)

IF(HAVE_MCP)
	TARGET_LINK_LIBRARIES(server mcp-eval)
ENDIF(HAVE_MCP)

# ------------------------------------------------------------

ADD_EXECUTABLE(cogserver
	CogServerMain.cc
)

TARGET_LINK_LIBRARIES(cogserver
	server
	servernode
	cogserver-types
)

IF (WIN32)
	SET_TARGET_PROPERTIES(cogserver PROPERTIES LINK_FLAGS "/NODEFAULTLIB:libctm")
ENDIF (WIN32)

INSTALL (TARGETS cogserver RUNTIME DESTINATION "bin")

# XXX What does this do? What is the point of this?
INSTALL (TARGETS server
	EXPORT CogServerTargets
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/opencog"
)

INSTALL (FILES
	CogServer.h
	Factory.h
	MCPServer.h
	Module.h
	ModuleManager.h
	PageServer.h
	Request.h
	RequestClassInfo.h
	RequestManager.h
	WebServer.h
	DESTINATION "include/opencog/${PROJECT_NAME}/server"
)
