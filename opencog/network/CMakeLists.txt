# ------------------------------------------------------------

ADD_LIBRARY (network SHARED
	ConsoleSocket.cc
	GenericShell.cc
	NetworkServer.cc
	ServerSocket.cc
	WebSocket.cc
)

TARGET_LINK_LIBRARIES(network
	${COGUTIL_LIBRARY}
	# ${Boost_SYSTEM_LIBRARY}
)

# The EXPORT is needed to autogenerate CMake boilerplate files in the
# lib directory that lets other packages FIND_PACKAGE(CogServer)
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
	DESTINATION "include/opencog/network"
)
