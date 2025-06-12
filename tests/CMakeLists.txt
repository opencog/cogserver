ENABLE_TESTING()
INCLUDE(AddCxxtest)

# Perform tests in component-dependency order, as much as possible.
IF (CXXTEST_FOUND)

	IF (HAVE_CYTHON)
		ADD_SUBDIRECTORY (cython)
	ENDIF (HAVE_CYTHON)
	IF (HAVE_MCP)
		ADD_SUBDIRECTORY (mcp)
	ENDIF (HAVE_MCP)
	ADD_SUBDIRECTORY (proxy)
	ADD_SUBDIRECTORY (shell)

ENDIF (CXXTEST_FOUND)
