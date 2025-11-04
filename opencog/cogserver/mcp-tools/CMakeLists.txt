
ADD_LIBRARY (mcp-tools SHARED
	McpPlugAtomSpace.cc
)

TARGET_LINK_LIBRARIES(mcp-tools
	json
	sexpr
	atomspace
	execution
	atombase
	${COGUTIL_LIBRARY}
)

INSTALL (TARGETS mcp-tools EXPORT CogServerTargets
	DESTINATION "lib${LIB_DIR_SUFFIX}/opencog"
)

INSTALL (FILES
	McpPlugin.h
	McpPlugAtomSpace.h
	DESTINATION "include/opencog/cogserver/mcp-tools"
)

# -------------------------------
