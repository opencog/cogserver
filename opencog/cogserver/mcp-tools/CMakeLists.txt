
ADD_LIBRARY (mcp-tools SHARED
	McpPlugAtomSpace.cc
	McpPlugEcho.cc
)

TARGET_LINK_LIBRARIES(mcp-tools
	PUBLIC
		json
		sexpr
		atomspace
		execution
		atombase
		${COGUTIL_LIBRARY}
	PRIVATE
		${JSONCPP_LIBRARIES}
)

INSTALL (TARGETS mcp-tools EXPORT CogServerTargets
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/opencog"
)

INSTALL (FILES
	McpPlugin.h
	McpPlugAtomSpace.h
	McpPlugEcho.h
	DESTINATION "include/opencog/cogserver/mcp-tools"
)

# -------------------------------
