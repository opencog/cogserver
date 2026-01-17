#
# Conditionally install index.html to /usr/local/share/cogserver/visualizer/
# only if the directory does not exist or index.html is not already installed.
# This allows atomspace-viz to be installed separately and take precedence.
#
INSTALL(CODE "
	set(DEST_DIR \"\$ENV{DESTDIR}\${CMAKE_INSTALL_PREFIX}/share/cogserver/visualizer\")
	set(DEST_FILE \"\${DEST_DIR}/index.html\")
	if(NOT EXISTS \"\${DEST_DIR}\" OR NOT EXISTS \"\${DEST_FILE}\")
		message(STATUS \"Installing: \${DEST_FILE}\")
		file(INSTALL \"${CMAKE_CURRENT_SOURCE_DIR}/index.html\"
			DESTINATION \"\${DEST_DIR}\")
	else()
		message(STATUS \"Skipping: \${DEST_FILE} (already installed by atomspace-viz)\")
	endif()
"
	COMPONENT visualizer
)
