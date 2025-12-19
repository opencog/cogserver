INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

LINK_LIBRARIES(
	network
	server
	servernode
	${ATOMSPACE_LIBRARY}
	${COGUTIL_LIBRARY})

IF (HAVE_CYTHON)
	INCLUDE_DIRECTORIES(
		${Python3_INCLUDE_DIRS}
	)

	ADD_LIBRARY (py-shell SHARED
		PythonShell.cc
		PythonShellModule.cc
	)

	TARGET_LINK_LIBRARIES(py-shell
		PythonEval
		${ATOMSPACE_PythonEval_LIBRARY}
		${ATOMSPACE_smob_LIBRARY}
		${Python3_LIBRARIES}
	)
ENDIF (HAVE_CYTHON)

ADD_LIBRARY (scheme-shell SHARED
	SchemeShell.cc
	SchemeShellModule.cc
)

TARGET_LINK_LIBRARIES(scheme-shell
	${ATOMSPACE_smob_LIBRARY}
)

ADD_LIBRARY (sexpr-shell SHARED
	SexprShell.cc
	SexprShellModule.cc
)

TARGET_LINK_LIBRARIES(sexpr-shell
	${ATOMSPACE_STORAGE_LIBRARIES}
)

ADD_LIBRARY (json-shell SHARED
	JsonShell.cc
	JsonShellModule.cc
)

TARGET_LINK_LIBRARIES(json-shell
	${ATOMSPACE_STORAGE_LIBRARIES}
)

ADD_LIBRARY (top-shell SHARED
	TopEval.cc
	TopShell.cc
	TopShellModule.cc
)

IF (HAVE_MCP)
	ADD_LIBRARY (mcp-shell SHARED
		McpShell.cc
		McpShellModule.cc
	)

	TARGET_LINK_LIBRARIES(mcp-shell
		mcp-eval
	)
ENDIF (HAVE_MCP)

# ---------------------- install targets

INSTALL (TARGETS
	json-shell
	scheme-shell
	sexpr-shell
	top-shell
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/opencog/modules")

IF (HAVE_CYTHON)
	INSTALL (TARGETS py-shell
		LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/opencog/modules")
ENDIF (HAVE_CYTHON)

IF (HAVE_MCP)
	INSTALL (TARGETS mcp-shell
		LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/opencog/modules")
ENDIF (HAVE_MCP)
