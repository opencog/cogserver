
ADD_LIBRARY(servernode SHARED
	CogServerNode.cc
)

TARGET_LINK_LIBRARIES(servernode
	server
	cogserver-types
	${ATOMSPACE_LIBRARIES}
)

INSTALL (TARGETS servernode
	EXPORT CogServerTargets
	LIBRARY DESTINATION "lib${LIB_DIR_SUFFIX}/opencog"
)

INSTALL (FILES
	CogServerNode.h
	DESTINATION "include/opencog/${PROJECT_NAME}/atoms"
)
