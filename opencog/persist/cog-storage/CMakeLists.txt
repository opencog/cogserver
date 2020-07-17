#
# Build the CogServer driver for the AtomSpace
#

ADD_LIBRARY (persist-cog SHARED
	CogStorage
	CogIO
	CogPersistSCM
)

TARGET_LINK_LIBRARIES(persist-cog
	${ATOMSPACE_LIBRARIES}
)

ADD_GUILE_EXTENSION(SCM_CONFIG persist-cog "opencog-ext-path-persist-cog")

INSTALL (TARGETS persist-cog EXPORT AtomSpaceTargets
	DESTINATION "lib${LIB_DIR_SUFFIX}/opencog"
)
