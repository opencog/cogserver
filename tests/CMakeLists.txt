ENABLE_TESTING()
INCLUDE(AddCxxtest)

# Perform tests in component-dependency order, as much as possible.
IF (CXXTEST_FOUND)

	IF (HAVE_CYTHON)
		ADD_SUBDIRECTORY (cython)
	ENDIF (HAVE_CYTHON)
	ADD_SUBDIRECTORY (shell)
	ADD_SUBDIRECTORY (proxy)

ENDIF (CXXTEST_FOUND)
