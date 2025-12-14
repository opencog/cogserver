# Generate the atom types
OPENCOG_GEN_CXX_ATOMTYPES(cogserver_types.script
	atom_types.h
	atom_types.definitions
	atom_types.inheritance)

OPENCOG_GEN_SCM_ATOMTYPES(cogserver_types.script cogserver_types.scm)
OPENCOG_GEN_PYTHON_ATOMTYPES(cogserver_types.script cogserver_types.pyx)

ADD_CUSTOM_TARGET(cogserver_atom_types
	DEPENDS atom_types.h cogserver_types.scm cogserver_types.pyx)

# The atom_types.h file is written to the build directory
INCLUDE_DIRECTORIES(${CMAKE_BINARY_DIR})

ADD_LIBRARY(cogserver-types SHARED
	cogserver_types_init.cc
)

# Without this, parallel make will race and crap up the generated files.
ADD_DEPENDENCIES(cogserver-types cogserver_atom_types)

TARGET_LINK_LIBRARIES(cogserver-types
	${ATOMSPACE_atomtypes_LIBRARY}
)

INSTALL (TARGETS cogserver-types
	EXPORT CogServerTargets
	LIBRARY DESTINATION "lib${LIB_DIR_SUFFIX}/opencog"
)

INSTALL (FILES
	${CMAKE_CURRENT_BINARY_DIR}/atom_types.h
	DESTINATION "include/opencog/cogserver/types"
)

# Install the auto-generated atom types as well
ADD_GUILE_MODULE (FILES
	${CMAKE_CURRENT_BINARY_DIR}/cogserver_types.scm
	MODULE_DESTINATION "${GUILE_SITE_DIR}/opencog/cogserver/types"
	DEPENDS cogserver_atom_types
)
