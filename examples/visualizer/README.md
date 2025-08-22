# AtomSpace Visualizer

A web-based visualization tool for the AtomSpace in-RAM database, providing real-time statistics and monitoring through WebSocket connections to the CogServer.

## Features

- **WebSocket Connection Management**: Connect to any running CogServer instance via WebSocket
- **Multiple Endpoint Support**: Compatible with JSON, Scheme, Python, and S-Expression endpoints
- **Real-time Statistics**: Display total atom count, nodes, links, and atom types
- **Interactive Debug Console**: Send commands directly to the CogServer
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

1. Open `index.html` in a web browser
2. Enter the WebSocket URL of your CogServer (default: `ws://localhost:18080/`)
3. Select the endpoint type (JSON recommended for visualization)
4. Click "Connect"
5. Once connected, the visualizer will automatically fetch and display AtomSpace statistics

## Interface Components

### Connection Panel
- **CogServer URL**: Enter the WebSocket URL of your CogServer
- **Endpoint**: Choose between JSON, Scheme, Python, or S-Expression endpoints
- **Connect/Disconnect Button**: Manage the WebSocket connection

### Status Panel
- Displays current connection status
- Shows connected server URL
- Indicates active endpoint type

### Statistics Panel
- **Total Atoms**: Complete count of all atoms in the AtomSpace
- **Nodes**: Count of atomic nodes
- **Links**: Count of link atoms
- **Atom Types**: Number of distinct atom types
- **Refresh Button**: Manually update statistics
- **Last Update**: Timestamp of most recent data fetch

### Debug Console
- Send custom commands to the CogServer
- View raw responses from the server
- Useful for testing and exploration

## Supported Commands

### JSON Endpoint Commands
- `AtomSpace.version()` - Get API version
- `AtomSpace.getAtoms("Atom", true)` - Fetch all atoms
- `AtomSpace.getSubTypes("TopType", true)` - Get all atom types
- `AtomSpace.makeAtom({"type": "Concept", "name": "example"})` - Create an atom

### Scheme Endpoint Commands
- `(cog-get-atoms 'Atom #t)` - Fetch all atoms
- `(cog-report-counts)` - Get summary report
- `(Concept "example")` - Create a concept node

## Architecture

The visualizer consists of three main components:

1. **index.html**: Main HTML structure and layout
2. **styles.css**: Comprehensive styling with modern CSS features
3. **visualizer.js**: WebSocket management and data processing logic

## Browser Compatibility

- Chrome/Edge 79+
- Firefox 75+
- Safari 13+
- Opera 66+

## Troubleshooting

### Connection Issues
- Verify the CogServer is running and accessible
- Check the WebSocket URL format (should be `ws://host:port/`)
- Ensure no firewall is blocking the WebSocket port
- Check browser console for detailed error messages

### No Data Displayed
- Confirm the selected endpoint matches your CogServer configuration
- Try the JSON endpoint for best compatibility
- Use the debug console to test raw commands

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