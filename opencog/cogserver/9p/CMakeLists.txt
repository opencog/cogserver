# ------------------------------------------------------------
# The 9P2000 wire-format codec. This is a pure encoder/decoder
# with no dependencies beyond the C++ standard library; the
# AtomSpace-backed filesystem that uses it lives in higher layers.

ADD_LIBRARY (nine9p SHARED
	Nine9P.cc
)

# The AtomSpace-backed synthetic file tree, and the per-connection
# 9P session dispatcher that serves it.
ADD_LIBRARY (ninefs SHARED
	AtomFs.cc
	NineNode.cc
	NineSession.cc
)

TARGET_LINK_LIBRARIES(ninefs
	nine9p
	${ATOMSPACE_LIBRARIES}
	${COGUTIL_LIBRARY}
)

# The EXPORT is needed to autogenerate CMake boilerplate files in the
# lib directory that lets other packages FIND_PACKAGE(CogServer)
INSTALL (TARGETS nine9p ninefs
	EXPORT CogServerTargets
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/opencog"
)

INSTALL (FILES
	AtomFs.h
	Nine9P.h
	NineNode.h
	NineSession.h
	DESTINATION "include/opencog/cogserver/9p"
)
