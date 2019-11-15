
ADD_LIBRARY (builtinreqs SHARED
	BuiltinRequestsModule
	Request
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

INSTALL (FILES
	Request.h
	RequestClassInfo.h
	DESTINATION "include/${PROJECT_NAME}/cogserver/modules/commands"
)
