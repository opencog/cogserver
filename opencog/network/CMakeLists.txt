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

# ------------------------------------------------------------

INSTALL (FILES
	ConsoleSocket.h
	GenericShell.h
	NetworkServer.h
	ServerSocket.h
	DESTINATION "include/${PROJECT_NAME}/network"
)
