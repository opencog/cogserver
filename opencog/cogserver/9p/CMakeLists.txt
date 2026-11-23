# ------------------------------------------------------------
# The 9P2000 wire-format codec. This is a pure encoder/decoder
# with no dependencies beyond the C++ standard library; the
# AtomSpace-backed filesystem that uses it lives in higher layers.

ADD_LIBRARY (nine9p SHARED
	Nine9P.cc
)

# The EXPORT is needed to autogenerate CMake boilerplate files in the
# lib directory that lets other packages FIND_PACKAGE(CogServer)
INSTALL (TARGETS nine9p
	EXPORT CogServerTargets
	DESTINATION "${CMAKE_INSTALL_LIBDIR}/opencog"
)

INSTALL (FILES
	Nine9P.h
	DESTINATION "include/opencog/cogserver/9p"
)
