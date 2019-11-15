# ------------------------------------------------------------

ADD_LIBRARY (server SHARED
	BaseServer
	CogServer
	NetworkServer
	Request
	ServerSocket
	ConsoleSocket
)

TARGET_LINK_LIBRARIES(server
	${ATOMSPACE_LIBRARIES}
	${COGUTIL_LIBRARY}
	${Boost_FILESYSTEM_LIBRARY}
	dl
)

# ------------------------------------------------------------
# This defines the guile scheme loadable module

IF (HAVE_GUILE)
	INCLUDE_DIRECTORIES (${GUILE_INCLUDE_DIRS})

	ADD_LIBRARY (guile-cogserver SHARED
		CogServerSCM
	)

	TARGET_LINK_LIBRARIES(guile-cogserver
		server
		${Boost_SYSTEM_LIBRARY}
		${GUILE_LIBRARIES}
	)

	INSTALL (TARGETS guile-cogserver DESTINATION "lib${LIB_DIR_SUFFIX}/opencog")

	ADD_GUILE_EXTENSION(SCM_CONFIG guile-cogserver "opencog-ext-path-cogserver")
ENDIF (HAVE_GUILE)

# ------------------------------------------------------------

ADD_EXECUTABLE(cogserver
	CogServerMain
)

TARGET_LINK_LIBRARIES(cogserver
	server
	${Boost_SYSTEM_LIBRARY}
)

SET_TARGET_PROPERTIES(cogserver PROPERTIES LINK_DIRECTORIES "${BOOST_LIBRARY_DIRS}")
IF (WIN32)
	SET_TARGET_PROPERTIES(cogserver PROPERTIES LINK_FLAGS "/NODEFAULTLIB:libctm")
ENDIF (WIN32)

INSTALL (TARGETS cogserver RUNTIME DESTINATION "bin")

INSTALL (TARGETS server
	EXPORT CogServerTargets
	DESTINATION "lib${LIB_DIR_SUFFIX}/opencog"
)

INSTALL (FILES
	BaseServer.h
	CogServer.h
	ConsoleSocket.h
	Factory.h
	Module.h
	NetworkServer.h
	Registry.h
	Request.h
	RequestClassInfo.h
	DESTINATION "include/${PROJECT_NAME}/cogserver/server"
)
