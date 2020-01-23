
ADD_SUBDIRECTORY(agents)
ADD_SUBDIRECTORY(commands)

IF (HAVE_CYTHON)
	ADD_SUBDIRECTORY(python)
ENDIF (HAVE_CYTHON)

# Stub out. This has two issues:
# 1) It has a circular dependency on the attention subsystem
# 2) It doesn't publish value-change events.
# Basically, its bit-rotted code.
#
# IF (HAVE_EVENT_PUBLISHING_DEPENDENCIES)
#	ADD_SUBDIRECTORY(events)
# ENDIF (HAVE_EVENT_PUBLISHING_DEPENDENCIES)
