
IF (HAVE_GUILE)
	ADD_GUILE_TEST(CogServerTest cog-server-test.scm)

	# Set COGSERVER_MODULE_PATH so modules can be found in the build directory
	SET_PROPERTY(TEST CogServerTest APPEND PROPERTY ENVIRONMENT
		"COGSERVER_MODULE_PATH=${PROJECT_BINARY_DIR}/opencog/cogserver/modules:${PROJECT_BINARY_DIR}/opencog/cogserver/shell")
ENDIF (HAVE_GUILE)
