# ------------------------------------------------------------

ADD_LIBRARY (network SHARED
	NetworkServer
	ServerSocket
	ConsoleSocket
)

TARGET_LINK_LIBRARIES(network
	${COGUTIL_LIBRARY}
)

# ------------------------------------------------------------

INSTALL (FILES
	ConsoleSocket.h
	NetworkServer.h
	DESTINATION "include/${PROJECT_NAME}/network"
)
