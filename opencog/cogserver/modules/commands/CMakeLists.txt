
ADD_LIBRARY (builtinreqs MODULE
	BuiltinRequestsModule
	ModuleManagement
	LoadModuleRequest
	ShutdownRequest
	UnloadModuleRequest
)

TARGET_LINK_LIBRARIES(builtinreqs
	server
)

INSTALL (TARGETS builtinreqs
	DESTINATION "lib${LIB_DIR_SUFFIX}/opencog/modules")

# ------------------------------------------------------------
