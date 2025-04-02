
# ----------------------------------------
# Python module to be loaded by CogServer

INCLUDE_DIRECTORIES(
   ${Python3_INCLUDE_DIRS}
   ${CMAKE_CURRENT_SOURCE_DIR}
   ${CMAKE_CURRENT_BINARY_DIR}
)

ADD_LIBRARY (PythonModule MODULE
	PythonModule.cc 
	PyRequest.cc
)

TARGET_LINK_LIBRARIES(PythonModule
	server
	PythonEval
	${ATOMSPACE_LIBRARIES}
	${Python3_LIBRARIES}
)

INSTALL (TARGETS PythonModule DESTINATION
	"lib${LIB_DIR_SUFFIX}/opencog/modules")
