
ADD_LIBRARY (wthru-proxy SHARED
	WriteThruProxy
)

TARGET_LINK_LIBRARIES(wthru-proxy
	network
	server
	${ATOMSPACE_LIBRARY}
	${COGUTIL_LIBRARY}
)

INSTALL (TARGETS wthru-proxy
	LIBRARY DESTINATION "lib${LIB_DIR_SUFFIX}/opencog/modules")

INSTALL (FILES
	Proxy.h
	WriteThruProxy.h
	DESTINATION "include/opencog/cogserver/proxy/"
)
