# ------------------------------------------------------------

ADD_LIBRARY (network SHARED
	ConsoleSocket.cc
	GenericShell.cc
	NetworkServer.cc
	ServerSocket.cc
	SocketManager.cc
	WebSocket.cc
)

TARGET_LINK_LIBRARIES(network
	${COGUTIL_LIBRARY}
)

# WebSocket.cc uses the OpenSSL SHA1 hash for the websocket handshake.
IF (HAVE_OPENSSL)
	TARGET_LINK_LIBRARIES(network ${OPENSSL_LIBRARIES})
ENDIF (HAVE_OPENSSL)

# The EXPORT is needed to autogenerate CMake boilerplate files in the
# lib directory that lets other packages FIND_PACKAGE(CogServer)
INSTALL (TARGETS network
	EXPORT CogServerTargets
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/opencog"
)

# ------------------------------------------------------------

INSTALL (FILES
	ConsoleSocket.h
	GenericShell.h
	NetworkServer.h
	ServerSocket.h
	SocketManager.h
	DESTINATION "include/opencog/network"
)
