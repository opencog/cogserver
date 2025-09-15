INCLUDE_DIRECTORIES(
	${Python3_INCLUDE_DIRS}
)

ADD_CXXTEST(PyEvalUTest)

TARGET_LINK_LIBRARIES(PyEvalUTest
	py-shell
	${COGSERVER_LIBRARIES}
	${ATOMSPACE_LIBRARIES}
	${COGUTIL_LIBRARY}
)

# --------------------------

ADD_TEST(NAME CogServerPythonTest
	COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/test_cogserver.py
	WORKING_DIRECTORY ${CMAKE_BINARY_DIR}
)

SET_PROPERTY(TEST CogServerPythonTest
	PROPERTY ENVIRONMENT
	"PYTHONPATH=${CMAKE_BINARY_DIR}/opencog/cython:${PYTHON_ROOT}:$ENV{PYTHONPATH}"
	"LD_LIBRARY_PATH=${CMAKE_BINARY_DIR}/opencog/cogserver/server:${CMAKE_BINARY_DIR}/opencog/cython/opencog:$ENV{LD_LIBRARY_PATH}"
)

# Mark test to run after the cogserver_cython target is built
SET_PROPERTY(TEST CogServerPythonTest
	PROPERTY DEPENDS cogserver_cython
)

# --------------------------
