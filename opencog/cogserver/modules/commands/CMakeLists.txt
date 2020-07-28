
ADD_LIBRARY (builtinreqs MODULE
	BuiltinRequestsModule
	LoadModuleRequest
	ListRequest
	ShutdownRequest
	UnloadModuleRequest
	ListModulesRequest
)

TARGET_LINK_LIBRARIES(builtinreqs
	server
)

INSTALL (TARGETS builtinreqs
	DESTINATION "lib${LIB_DIR_SUFFIX}/opencog/modules")

# ------------------------------------------------------------
