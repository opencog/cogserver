
ADD_LIBRARY (w-thru-proxy SHARED
	ThruProxy.cc
	WriteThruProxy.cc
)

TARGET_LINK_LIBRARIES(w-thru-proxy
	network
	server
	${ATOMSPACE_LIBRARY}
	${COGUTIL_LIBRARY}
)

INSTALL (TARGETS w-thru-proxy
	LIBRARY DESTINATION "lib${LIB_DIR_SUFFIX}/opencog/modules")

# --------------------------------------

ADD_LIBRARY (r-thru-proxy SHARED
	ReadThruProxy.cc
	ThruProxy.cc
)

TARGET_LINK_LIBRARIES(r-thru-proxy
	network
	server
	${ATOMSPACE_LIBRARY}
	${COGUTIL_LIBRARY}
)

INSTALL (TARGETS r-thru-proxy
	LIBRARY DESTINATION "lib${LIB_DIR_SUFFIX}/opencog/modules")

# --------------------------------------

INSTALL (FILES
	Proxy.h
	ReadThruProxy.h
	ThruProxy.h
	WriteThruProxy.h
	DESTINATION "include/opencog/cogserver/proxy/"
)
