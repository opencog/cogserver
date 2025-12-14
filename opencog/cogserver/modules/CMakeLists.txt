
ADD_LIBRARY (builtinreqs MODULE
	BuiltinRequestsModule.cc
	ModuleManagement.cc
	ShutdownRequest.cc
)

TARGET_LINK_LIBRARIES(builtinreqs
	servernode
	server
)

INSTALL (TARGETS builtinreqs
	DESTINATION "lib${LIB_DIR_SUFFIX}/opencog/modules")

# ------------------------------------------------------------
