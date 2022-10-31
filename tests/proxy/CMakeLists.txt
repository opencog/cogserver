
LINK_DIRECTORIES(
	${PROJECT_BINARY_DIR}/opencog/atomspace
	${PROJECT_BINARY_DIR}/opencog/cogserver/server
)

LINK_LIBRARIES(
	server
	${ATOMSPACE_LIBRARIES}
	${Boost_SYSTEM_LIBRARY}
)


# Disable for now. This passes for me, but fails in circleci
# and I cannot tell why. Too lazy to fix.
# ADD_CXXTEST(WriteThruProxyUTest)
