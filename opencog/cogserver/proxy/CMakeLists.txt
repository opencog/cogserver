
ADD_LIBRARY (wthru-proxy SHARED
	WriteThruProxy.cc
)

TARGET_LINK_LIBRARIES(wthru-proxy
	network
	server
	${ATOMSPACE_LIBRARY}
	${COGUTIL_LIBRARY}
)

INSTALL (TARGETS wthru-proxy
	LIBRARY DESTINATION "lib${LIB_DIR_SUFFIX}/opencog/modules")

# --------------------------------------

ADD_LIBRARY (readthru-proxy SHARED
	ReadThruProxy.cc
)

TARGET_LINK_LIBRARIES(readthru-proxy
	network
	server
	${ATOMSPACE_LIBRARY}
	${COGUTIL_LIBRARY}
)

INSTALL (TARGETS readthru-proxy
	LIBRARY DESTINATION "lib${LIB_DIR_SUFFIX}/opencog/modules")

# --------------------------------------

INSTALL (FILES
	Proxy.h
	ReadThruProxy.h
	WriteThruProxy.h
	DESTINATION "include/opencog/cogserver/proxy/"
)
