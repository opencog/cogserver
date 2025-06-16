# ------------------------------------------------------------

ADD_LIBRARY (server SHARED
	BaseServer.cc
	CogServer.cc
	MCPServer.cc
	ModuleManager.cc
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
# This defines the guile scheme loadable module

IF (HAVE_GUILE)
	INCLUDE_DIRECTORIES (${GUILE_INCLUDE_DIRS})

	ADD_LIBRARY (guile-cogserver SHARED
		CogServerSCM.cc
	)

	TARGET_LINK_LIBRARIES(guile-cogserver
		server
		# ${Boost_SYSTEM_LIBRARY}
		${GUILE_LIBRARIES}
	)

	INSTALL (TARGETS guile-cogserver DESTINATION "lib${LIB_DIR_SUFFIX}/opencog")

	ADD_GUILE_EXTENSION(SCM_CONFIG guile-cogserver "opencog-ext-path-cogserver")
ENDIF (HAVE_GUILE)

# ------------------------------------------------------------

ADD_EXECUTABLE(cogserver
	CogServerMain.cc
)

TARGET_LINK_LIBRARIES(cogserver
	server
	${Boost_FILESYSTEM_LIBRARY}
)

SET_TARGET_PROPERTIES(cogserver PROPERTIES LINK_DIRECTORIES "${BOOST_LIBRARY_DIRS}")
IF (WIN32)
	SET_TARGET_PROPERTIES(cogserver PROPERTIES LINK_FLAGS "/NODEFAULTLIB:libctm")
ENDIF (WIN32)

INSTALL (TARGETS cogserver RUNTIME DESTINATION "bin")

# XXX What does this do? What is the point of this?
INSTALL (TARGETS server
	EXPORT CogServerTargets
	DESTINATION "lib${LIB_DIR_SUFFIX}/opencog"
)

INSTALL (FILES
	BaseServer.h
	CogServer.h
	Factory.h
	MCPServer.h
	Module.h
	ModuleManager.h
	Request.h
	RequestClassInfo.h
	RequestManager.h
	WebServer.h
	DESTINATION "include/opencog/${PROJECT_NAME}/server"
)
