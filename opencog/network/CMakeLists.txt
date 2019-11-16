# ------------------------------------------------------------

ADD_LIBRARY (network SHARED
	ConsoleSocket
	GenericShell
	NetworkServer
	ServerSocket
)

TARGET_LINK_LIBRARIES(network
	${COGUTIL_LIBRARY}
)

# XXX What does this do? What is the point of this?
INSTALL (TARGETS network
	EXPORT CogServerTargets
	DESTINATION "lib${LIB_DIR_SUFFIX}/opencog"
)

# ------------------------------------------------------------

INSTALL (FILES
	ConsoleSocket.h
	GenericShell.h
	NetworkServer.h
	ServerSocket.h
	DESTINATION "include/${PROJECT_NAME}/network"
)
