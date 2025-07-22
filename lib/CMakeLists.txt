
include(CMakePackageConfigHelpers)

set(ConfigPackageLocation lib/cmake/CogServer)

install(EXPORT CogServerTargets
	FILE CogServerTargets.cmake
	DESTINATION ${ConfigPackageLocation}
)

configure_package_config_file(CogServerConfig.cmake.in
	${CMAKE_CURRENT_BINARY_DIR}/CogServerConfig.cmake
	INSTALL_DESTINATION ${ConfigPackageLocation}
	PATH_VARS CMAKE_INSTALL_PREFIX
)

SET(SEMANTIC_VERSION 1.0.0)

write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/CogServerConfigVersion.cmake"
	VERSION ${SEMANTIC_VERSION}
	COMPATIBILITY SameMajorVersion
)

INSTALL(FILES
	${CMAKE_CURRENT_BINARY_DIR}/CogServerConfigVersion.cmake
	${CMAKE_CURRENT_BINARY_DIR}/CogServerConfig.cmake
	DESTINATION ${ConfigPackageLocation}
)

# Create a test configuration file by copying the default configuration file
CONFIGURE_FILE(${CMAKE_SOURCE_DIR}/lib/opencog-test.conf
	${PROJECT_BINARY_DIR}/lib/opencog-test.conf)

# Place a default config file in a default location
# CONFDIR normally expands to /usr/local/etc/
INSTALL (FILES
	cogserver.conf
	DESTINATION "${CONFDIR}"
)
