
# Need to use -fno-strict-aliasing when compiling cython code, in order
# to avoid nasty compiler warnings about aliasing.  Cython explicitly
# performs aliasing, in order to emulate python object inheritance.
# See, for example,
# https://groups.google.com/forum/#!topic/cython-users/JV1-KvIUeIg
#
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fno-strict-aliasing")

INCLUDE_DIRECTORIES(
	${ATOMSPACE_INCLUDE_DIR}
	${PYTHON_INCLUDE_DIRS}
	${CMAKE_CURRENT_SOURCE_DIR}
	${CMAKE_CURRENT_BINARY_DIR}
)

SET(CYTHON_FLAGS "-3" "-f" "-I" "${ATOMSPACE_INCLUDE_DIR}/opencog/cython")

# Use this as a guide:
# https://github.com/OpenKinect/libfreenect/blob/master/wrappers/python/CMakeLists.txt

IF (HAVE_ATOMSPACE)
	############################## type constructors #####################
	CYTHON_ADD_MODULE_PYX(cogserver_type_constructors)

	list(APPEND ADDITIONAL_MAKE_CLEAN_FILES "cogserver_type_constructors.cpp")

	ADD_LIBRARY(cogserver_type_constructors SHARED
		cogserver_type_constructors.cpp
	)

	TARGET_LINK_LIBRARIES(cogserver_type_constructors
		${ATOMSPACE_LIBRARIES}
		${PYTHON_LIBRARIES}
	)

	SET_TARGET_PROPERTIES(cogserver_type_constructors PROPERTIES
		PREFIX ""
		OUTPUT_NAME cogserver_type_constructors)

	INSTALL (TARGETS cogserver_type_constructors
		DESTINATION "${PYTHON_DEST}")

ENDIF (HAVE_ATOMSPACE)


############################ cogserver #########################
CYTHON_ADD_MODULE_PYX(cogserver
	"${ATOMSPACE_INCLUDE_DIR}/opencog/cython/opencog/atomspace.pxd"
	"request.pyx" "agent.pyx" "agent_finder_types.h"
	"../../cogserver/modules/agents/Agent.h" "../../cogserver/server/Request.h"
)

list(APPEND ADDITIONAL_MAKE_CLEAN_FILES "cogserver.cpp")

# opencog.cogserver Python bindings
ADD_LIBRARY(cogserver_cython SHARED
	cogserver.cpp
)

TARGET_LINK_LIBRARIES(cogserver_cython
	server
	${ATOMSPACE_LIBRARIES}
	${COGUTIL_LIBRARY}
	${PYTHON_LIBRARIES}
)

SET_TARGET_PROPERTIES(cogserver_cython PROPERTIES
	PREFIX ""
	OUTPUT_NAME cogserver)

INSTALL (TARGETS cogserver_cython
	DESTINATION "${PYTHON_DEST}")

####################### agent finder ########################
CYTHON_ADD_MODULE_PYX(agent_finder
	"${ATOMSPACE_INCLUDE_DIR}/opencog/cython/opencog/atomspace.pxd"
	"cogserver.pxd"
	"../../cogserver/modules/agents/Agent.h" "../../cogserver/server/Request.h"
)

list(APPEND ADDITIONAL_MAKE_CLEAN_FILES
	"agent_finder.cpp" "agent_finder_api.h")

# utility library for PythonModule
ADD_LIBRARY(agent_finder SHARED
	agent_finder.cpp
)

TARGET_LINK_LIBRARIES(agent_finder
	${ATOMSPACE_LIBRARIES}
	${PYTHON_LIBRARIES}
	${Boost_THREAD_LIBRARY}
	${Boost_SYSTEM_LIBRARY}
)

SET_TARGET_PROPERTIES(agent_finder PROPERTIES
	PREFIX ""
	OUTPUT_NAME agent_finder)

INSTALL (TARGETS agent_finder
	DESTINATION "${PYTHON_DEST}")
