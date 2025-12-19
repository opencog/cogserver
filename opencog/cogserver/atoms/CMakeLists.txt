
ADD_LIBRARY(servernode SHARED
	CogServerNode.cc
)

TARGET_LINK_LIBRARIES(servernode
	server
	cogserver-types
	${ATOMSPACE_LIBRARIES}
)

ADD_GUILE_EXTENSION(SCM_CONFIG servernode "opencog-ext-path-servernode")

INSTALL (TARGETS servernode
	EXPORT CogServerTargets
	LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}/opencog"
)

INSTALL (FILES
	CogServerNode.h
	DESTINATION "include/opencog/${PROJECT_NAME}/atoms"
)
