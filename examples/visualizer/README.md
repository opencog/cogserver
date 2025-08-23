# AtomSpace Visualizer

A web-based visualization tool for the AtomSpace in-RAM database, providing real-time statistics and monitoring through WebSocket connections to the CogServer using the JSON protocol.

[***Try it here***](https://html-preview.github.io/?url=https://github.com/opencog/cogserver/blob/master/examples/visutalizer/index.html).
You must have a CogServer running somewhere; you will need to type the
URL into the connection box.

## Features

- **WebSocket Connection Management**: Connect to any running CogServer instance via WebSocket
- **JSON Protocol**: Uses the JSON endpoint for reliable data exchange and visualization
- **Real-time Statistics**: Display total atom count, nodes, links, and atom types
- **Interactive Debug Console**: Send JSON commands directly to the CogServer
- **Responsive Design**: Modern, clean interface that works on desktop and mobile devices
- **Connection Status Monitoring**: Visual feedback for connection state

## Getting Started

### Prerequisites

1. A running CogServer instance with WebSocket support enabled
2. A web browser with WebSocket support (all modern browsers)

### Starting the CogServer

Start the CogServer using one of these methods:

#### Command Line:
```bash
/usr/local/bin/cogserver
```

#### From Guile Scheme:
```scheme
(use-modules (opencog) (opencog cogserver))
(start-cogserver)
```

#### From Python:
```python
import opencog, opencog.cogserver
opencog.cogserver.startCogserver()
```

### Using the Visualizer

1. Open `index.html` in a web browser.
   [***Try it here***](https://html-preview.github.io/?url=https://github.com/opencog/cogserver/blob/master/examples/visutalizer/index.html).
2. Enter the WebSocket URL of your CogServer (default: `ws://localhost:18080/`)
3. Click "Connect" (the JSON endpoint will be automatically appended)
4. Once connected, the visualizer will automatically fetch and display AtomSpace statistics

## Interface Components

### Connection Panel
- **CogServer URL**: Enter the WebSocket URL of your CogServer (JSON endpoint is automatically used)
- **Connect/Disconnect Button**: Manage the WebSocket connection

### Status Panel
- Displays current connection status
- Shows connected server URL with JSON endpoint

### Statistics Panel
- **Total Atoms**: Complete count of all atoms in the AtomSpace
- **Nodes**: Count of atomic nodes
- **Links**: Count of link atoms
- **Atom Types**: Number of distinct atom types
- **Refresh Button**: Manually update statistics
- **Last Update**: Timestamp of most recent data fetch

### Atoms by Type
- Displays a breakdown of all atoms grouped by their type
- Shows the count for each atom type
- Sorted by count (highest first) and then alphabetically
- Only displays types with at least one atom (zero counts are excluded)
- Updates automatically when fetching atom statistics

### Debug Console
- Send JSON commands to the CogServer
- View raw responses from the server
- Useful for testing and exploration

## Supported JSON Commands

- `AtomSpace.version()` - Get API version
- `AtomSpace.getAtoms("Atom", true)` - Fetch all atoms
- `AtomSpace.getSubTypes("TopType", true)` - Get all atom types
- `AtomSpace.makeAtom({"type": "Concept", "name": "example"})` - Create an atom
- See the [JSON README](https://github.com/opencog/atomspace-storage/tree/master/opencog/persist/json) for full documentation

## Architecture

The visualizer consists of three main components:

1. **index.html**: Main HTML structure and layout
2. **styles.css**: Comprehensive styling with modern CSS features
3. **visualizer.js**: WebSocket management and JSON data processing logic

## Troubleshooting

### Connection Issues
- Verify the CogServer is running and accessible
- Check the WebSocket URL format (should be `ws://host:port/`)
- The visualizer will automatically append `/json` to connect to the JSON endpoint
- Ensure no firewall is blocking the WebSocket port
- Check browser console for detailed error messages
- Try the
  [../websockets/json-test.html](https://html-preview.github.io/?url=https://github.com/opencog/cogserver/blob/master/examples/websockets/json-test.html)
  page and check for errors.

### No Data Displayed
- Confirm the CogServer has JSON endpoint enabled
- Use the debug console to test JSON commands
- Check that the AtomSpace contains atoms to display

## Future Enhancements

Potential improvements for future versions:
- Graph visualization of atom relationships
- Real-time atom creation/deletion monitoring
- Atom type distribution charts
- Search and filter capabilities
- Export functionality for statistics
- Multi-server connection support

## License

Part of the OpenCog project. See the main project license for details.
