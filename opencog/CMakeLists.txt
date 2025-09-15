
# The atom_types.h file is written to the build directory
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})

# Enable testing without installing first.
DECLARE_GUILE_CONFIG_TARGET(SCM_CONFIG "opencog cs-config" "OPENCOG_TEST")

ADD_SUBDIRECTORY (network)
ADD_SUBDIRECTORY (cogserver)
IF(HAVE_CYTHON)
	ADD_SUBDIRECTORY (cython)
ENDIF(HAVE_CYTHON)

WRITE_GUILE_CONFIG(${GUILE_BIN_DIR}/opencog/cs-config.scm SCM_CONFIG TRUE)

WRITE_GUILE_CONFIG(${GUILE_BIN_DIR}/opencog/cs-config-installable.scm SCM_CONFIG FALSE)
INSTALL(FILES ${GUILE_BIN_DIR}/opencog/cs-config-installable.scm DESTINATION ${GUILE_SITE_DIR}/opencog RENAME cs-config.scm)
