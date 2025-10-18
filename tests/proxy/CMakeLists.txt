
LINK_DIRECTORIES(
	${PROJECT_BINARY_DIR}/opencog/atomspace
	${PROJECT_BINARY_DIR}/opencog/cogserver/server
)

LINK_LIBRARIES(
	server
	${ATOMSPACE_LIBRARIES}
)

# Heh. No unit tests. The olld one didn't work. Removed.
# The problem is that these unit tests cannot depend on rocks
# or cog-storage, because those do not yet exist when these
# tests are run in circle-ci
