
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

write_basic_package_version_file(
	"${CMAKE_CURRENT_BINARY_DIR}/CogServerConfigVersion.cmake"
	VERSION ${PROJECT_VERSION}
	COMPATIBILITY SameMajorVersion
)

INSTALL(FILES
	${CMAKE_CURRENT_BINARY_DIR}/CogServerConfigVersion.cmake
	${CMAKE_CURRENT_BINARY_DIR}/CogServerConfig.cmake
	DESTINATION ${ConfigPackageLocation}
)
