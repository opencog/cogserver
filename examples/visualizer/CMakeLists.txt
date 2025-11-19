
# Install visualizer files to /usr/local/share/cogserver/visualizer/
INSTALL(FILES
	index.html
	tree-view.html
	type-view.html
	README.md
	DESTINATION share/cogserver/visualizer
	COMPONENT visualizer
)

INSTALL(FILES
	atomspace-cache.js
	graph-view.js
	landing.js
	theme.js
	tree-view.js
	type-view.js
	styles.css
	DESTINATION share/cogserver/visualizer
	COMPONENT visualizer
)

# Install vis-network subdirectory
INSTALL(DIRECTORY vis-network
	DESTINATION share/cogserver/visualizer
	COMPONENT visualizer
	FILES_MATCHING
	PATTERN "*"
)
