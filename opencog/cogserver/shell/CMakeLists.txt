INCLUDE_DIRECTORIES(
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

IF (HAVE_CYTHON)
	INCLUDE_DIRECTORIES(
		${PYTHON_INCLUDE_PATH}
	)

	ADD_LIBRARY (py-shell SHARED
		PythonShell
		PythonShellModule
	)

	TARGET_LINK_LIBRARIES(py-shell
		network
		server # Python shell uses the cogserver atomspace
		PythonEval
		${ATOMSPACE_PythonEval_LIBRARY}
		${ATOMSPACE_smob_LIBRARY}
		${ATOMSPACE_LIBRARY}
		${COGUTIL_LIBRARY}
		# ${Boost_SYSTEM_LIBRARY}
		${PYTHON_LIBRARIES}
	)
ENDIF (HAVE_CYTHON)

ADD_LIBRARY (scheme-shell SHARED
	SchemeShell
	SchemeShellModule
)

TARGET_LINK_LIBRARIES(scheme-shell
	network
	server # Guile shell uses the cogserver atomspace.
	${ATOMSPACE_smob_LIBRARY}
	${ATOMSPACE_LIBRARY}
	${COGUTIL_LIBRARY}
)

ADD_LIBRARY (sexpr-shell SHARED
	SexprShell
	SexprShellModule
)

TARGET_LINK_LIBRARIES(sexpr-shell
	network
	server
	${ATOMSPACE_LIBRARY}
	${COGUTIL_LIBRARY}
)

ADD_LIBRARY (json-shell SHARED
	JsonShell
	JsonShellModule
)

TARGET_LINK_LIBRARIES(json-shell
	network
	server
	${ATOMSPACE_LIBRARY}
	${COGUTIL_LIBRARY}
)

IF (HAVE_CYTHON)
	INSTALL (TARGETS py-shell
		LIBRARY DESTINATION "lib${LIB_DIR_SUFFIX}/opencog/modules")
ENDIF (HAVE_CYTHON)

INSTALL (TARGETS scheme-shell
	LIBRARY DESTINATION "lib${LIB_DIR_SUFFIX}/opencog/modules")
INSTALL (TARGETS sexpr-shell
	LIBRARY DESTINATION "lib${LIB_DIR_SUFFIX}/opencog/modules")
INSTALL (TARGETS json-shell
	LIBRARY DESTINATION "lib${LIB_DIR_SUFFIX}/opencog/modules")
