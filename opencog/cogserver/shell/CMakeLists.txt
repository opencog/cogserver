INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

IF (HAVE_CYTHON)
	INCLUDE_DIRECTORIES(
		${Python3_INCLUDE_DIRS}
	)

	ADD_LIBRARY (py-shell SHARED
		PythonShell.cc
		PythonShellModule.cc
	)

	TARGET_LINK_LIBRARIES(py-shell
		network
		server # Python shell uses the cogserver atomspace
		PythonEval
		${ATOMSPACE_PythonEval_LIBRARY}
		${ATOMSPACE_smob_LIBRARY}
		${ATOMSPACE_LIBRARY}
		${COGUTIL_LIBRARY}
		${Python3_LIBRARIES}
	)
ENDIF (HAVE_CYTHON)

ADD_LIBRARY (scheme-shell SHARED
	SchemeShell.cc
	SchemeShellModule.cc
)

TARGET_LINK_LIBRARIES(scheme-shell
	network
	server # Guile shell uses the cogserver atomspace.
	${ATOMSPACE_smob_LIBRARY}
	${ATOMSPACE_LIBRARY}
	${COGUTIL_LIBRARY}
)

ADD_LIBRARY (sexpr-shell SHARED
	SexprShell.cc
	SexprShellModule.cc
)

TARGET_LINK_LIBRARIES(sexpr-shell
	network
	server
	${ATOMSPACE_STORAGE_LIBRARIES}
	${ATOMSPACE_LIBRARY}
	${COGUTIL_LIBRARY}
)

ADD_LIBRARY (json-shell SHARED
	JsonShell.cc
	JsonShellModule.cc
)

TARGET_LINK_LIBRARIES(json-shell
	network
	server
	${ATOMSPACE_STORAGE_LIBRARIES}
	${ATOMSPACE_LIBRARY}
	${COGUTIL_LIBRARY}
)

ADD_LIBRARY (top-shell SHARED
	TopEval.cc
	TopShell.cc
	TopShellModule.cc
)

TARGET_LINK_LIBRARIES(top-shell
	network
	server
	${ATOMSPACE_LIBRARY}
	${COGUTIL_LIBRARY}
)

IF (HAVE_MCP)
	ADD_LIBRARY (mcp-eval SHARED
		McpEval.cc
		McpPlugEcho.cc
	)
	TARGET_LINK_LIBRARIES(mcp-eval
		${ATOMSPACE_STORAGE_LIBRARIES}
		# nlohmann_json::nlohmann_json
		${nlohmann_json_LIBRARIES}
	)

	INSTALL (TARGETS mcp-eval
		EXPORT CogServerTargets
		DESTINATION "lib${LIB_DIR_SUFFIX}/opencog"
	)

	ADD_LIBRARY (mcp-shell SHARED
		McpShell.cc
		McpShellModule.cc
	)

	TARGET_LINK_LIBRARIES(mcp-shell
		mcp-eval
		network
		server
		${ATOMSPACE_LIBRARY}
		${COGUTIL_LIBRARY}
	)
ENDIF (HAVE_MCP)

# ---------------------- install targets

INSTALL (TARGETS
	json-shell
	scheme-shell
	sexpr-shell
	top-shell
	LIBRARY DESTINATION "lib${LIB_DIR_SUFFIX}/opencog/modules")

IF (HAVE_CYTHON)
	INSTALL (TARGETS py-shell
		LIBRARY DESTINATION "lib${LIB_DIR_SUFFIX}/opencog/modules")
ENDIF (HAVE_CYTHON)

IF (HAVE_MCP)
	INSTALL (TARGETS mcp-shell
		LIBRARY DESTINATION "lib${LIB_DIR_SUFFIX}/opencog/modules")
ENDIF (HAVE_MCP)
