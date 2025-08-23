// AtomSpace Visualizer JavaScript

let socket = null;
let serverURL = '';
let isConnected = false;
let atomData = {};

// DOM elements
let serverInput, connectBtn;
let connectionStatus, serverDisplay;
let atomspaceStats, errorPanel, errorMessage;
let atomCount, nodeCount, linkCount, typeCount;
let refreshBtn, lastUpdate;
let debugCommand, sendCommand, debugResponse;
let atomTypesBreakdown, typesList;
let atomListingPanel, atomListingTitle, atomListingContent, closeAtomListing;

// Initialize when DOM is loaded
document.addEventListener('DOMContentLoaded', init);

function init() {
    // Get DOM elements
    serverInput = document.getElementById('server-url');
    connectBtn = document.getElementById('connect-btn');

    connectionStatus = document.getElementById('connection-status');
    serverDisplay = document.getElementById('server-display');

    atomspaceStats = document.getElementById('atomspace-stats');
    errorPanel = document.getElementById('error-panel');
    errorMessage = document.getElementById('error-message');

    atomCount = document.getElementById('atom-count');
    nodeCount = document.getElementById('node-count');
    linkCount = document.getElementById('link-count');
    typeCount = document.getElementById('type-count');

    refreshBtn = document.getElementById('refresh-stats');
    lastUpdate = document.getElementById('last-update');

    debugCommand = document.getElementById('debug-command');
    sendCommand = document.getElementById('send-command');
    debugResponse = document.getElementById('debug-response');

    atomTypesBreakdown = document.getElementById('atom-types-breakdown');
    typesList = document.getElementById('types-list');

    atomListingPanel = document.getElementById('atom-listing-panel');
    atomListingTitle = document.getElementById('atom-listing-title');
    atomListingContent = document.getElementById('atom-listing-content');
    closeAtomListing = document.getElementById('close-atom-listing');

    // Set up event listeners
    connectBtn.addEventListener('click', toggleConnection);
    closeAtomListing.addEventListener('click', hideAtomListing);
    refreshBtn.addEventListener('click', fetchAtomSpaceStats);
    sendCommand.addEventListener('click', sendDebugCommand);
    debugCommand.addEventListener('keypress', (e) => {
        if (e.key === 'Enter') sendDebugCommand();
    });

    // Set default values
    serverInput.value = serverInput.value || 'ws://localhost:18080/';
}

function toggleConnection() {
    if (isConnected) {
        disconnect();
    } else {
        connect();
    }
}

function connect() {
    const baseURL = serverInput.value.trim();
    const endpoint = 'json'; // Always use JSON endpoint

    if (!baseURL) {
        showError('Please enter a CogServer URL');
        return;
    }

    // Ensure URL ends with /
    const normalizedURL = baseURL.endsWith('/') ? baseURL : baseURL + '/';
    serverURL = normalizedURL + endpoint;

    console.log('Connecting to:', serverURL);

    try {
        socket = new WebSocket(serverURL);

        socket.addEventListener('open', onConnect);
        socket.addEventListener('close', onDisconnect);
        socket.addEventListener('message', onMessage);
        socket.addEventListener('error', onError);

        // Update UI to connecting state
        connectBtn.disabled = true;
        connectionStatus.textContent = 'Connecting...';
        connectionStatus.className = 'status-value';
        hideError();
    } catch (err) {
        showError('Failed to create WebSocket connection: ' + err.message);
    }
}

function disconnect() {
    if (socket) {
        console.log('Disconnecting from:', serverURL);
        socket.close();
    }
}

function onConnect() {
    console.log('Connected to:', serverURL);
    isConnected = true;

    // Update UI
    connectBtn.disabled = false;
    connectBtn.innerHTML = '<span class="btn-icon">🔌</span><span class="btn-text">Disconnect</span>';
    connectBtn.classList.add('connected');

    connectionStatus.textContent = 'Connected';
    connectionStatus.className = 'status-value connected';

    serverDisplay.textContent = serverInput.value + 'json';

    // Enable controls
    serverInput.disabled = true;
    refreshBtn.disabled = false;
    debugCommand.disabled = false;
    sendCommand.disabled = false;

    // Show stats panel
    atomspaceStats.classList.remove('hidden');

    // First test with a simple version command
    console.log('Testing connection with version command...');
    sendMessage('AtomSpace.version()');

    // Then fetch initial stats after a short delay
    setTimeout(() => {
        fetchAtomSpaceStats();
    }, 1000);
}

function onDisconnect() {
    console.log('Disconnected from:', serverURL);
    isConnected = false;

    // Update UI
    connectBtn.disabled = false;
    connectBtn.innerHTML = '<span class="btn-icon">⚡</span><span class="btn-text">Connect</span>';
    connectBtn.classList.remove('connected');

    connectionStatus.textContent = 'Disconnected';
    connectionStatus.className = 'status-value disconnected';

    serverDisplay.textContent = 'Not connected';

    // Disable controls
    serverInput.disabled = false;
    refreshBtn.disabled = true;
    debugCommand.disabled = true;
    sendCommand.disabled = true;

    // Hide stats panel, types breakdown, and atom listing
    atomspaceStats.classList.add('hidden');
    atomTypesBreakdown.classList.add('hidden');
    atomListingPanel.classList.add('hidden');

    socket = null;
}

function onMessage(event) {
    console.log('Received message:', event.data);

    try {
        // Parse JSON response
        const data = JSON.parse(event.data);
        console.log('Parsed JSON:', data);

        // Display in debug console
        debugResponse.textContent = JSON.stringify(data, null, 2);

        // Check if this is a success response
        if (data.success === true && data.result !== undefined) {
            const result = data.result;

            // Handle different result types
            if (typeof result === 'string') {
                // Version response or other string results
                console.log('Received string result:', result);
                // If it looks like a version number, just log it
                if (result.match(/^\d+\.\d+\.\d+/)) {
                    console.log('CogServer JSON API version:', result);
                }
            } else if (Array.isArray(result)) {
                // Could be atoms or types
                if (result.length === 0) {
                    // Empty array - treat as empty atom list
                    console.log('Received empty array');
                    processAtomList(result);
                } else if (typeof result[0] === 'string') {
                    // Array of strings = types list
                    console.log('Received types list with', result.length, 'types');
                    processTypeList(result);
                } else if (typeof result[0] === 'object') {
                    // Array of objects = atoms list
                    console.log('Received atoms list with', result.length, 'atoms');
                    processAtomList(result);
                }
            } else if (typeof result === 'boolean') {
                // Response from makeAtom or other boolean operations
                console.log('Received boolean result:', result);
            } else if (typeof result === 'object' && result !== null) {
                // Could be a single atom
                console.log('Received object result:', result);
            }
        } else if (data.success === false) {
            // Error response
            const errorMsg = data.error?.message || data.error || 'Unknown error';
            console.error('Server returned error:', errorMsg);
            showError('Server error: ' + errorMsg);
        } else {
            // Unknown response format
            console.warn('Unknown response format:', data);
        }
    } catch (err) {
        // Failed to parse JSON
        console.error('Failed to parse JSON:', err);
        console.error('Raw message was:', event.data);
        debugResponse.textContent = 'Error parsing JSON: ' + err.message + '\n\nRaw response:\n' + event.data;
        showError('Invalid response from server');
    }
}

function onError(event) {
    console.error('WebSocket error:', event);
    showError('Connection error: Unable to connect to ' + serverURL);
}

function processAtomList(atoms) {
    // Handle empty or invalid atom list
    if (!atoms || !Array.isArray(atoms)) {
        console.log('Invalid or empty atom list received');
        atoms = [];
    }

    console.log('Processing atom list:', atoms.length, 'atoms');

    atomData.atoms = atoms;
    atomData.totalCount = atoms.length;

    // Count nodes and links, and track types
    let nodes = 0;
    let links = 0;
    const types = new Set();
    const typeCountMap = new Map();

    atoms.forEach(atom => {
        if (atom.type) {
            types.add(atom.type);
            // Count atoms by type
            const currentCount = typeCountMap.get(atom.type) || 0;
            typeCountMap.set(atom.type, currentCount + 1);
        }

        // Check if it's a node or link
        // Links have an 'outgoing' array with references to other atoms
        // Nodes typically don't have outgoing connections or have an empty array
        if (atom.outgoing && Array.isArray(atom.outgoing) && atom.outgoing.length > 0) {
            links++;
        } else {
            nodes++;
        }
    });

    console.log(`Stats - Total: ${atoms.length}, Nodes: ${nodes}, Links: ${links}, Types: ${types.size}`);

    // Update UI even if empty
    updateStats({
        total: atoms.length,
        nodes: nodes,
        links: links,
        types: types.size
    });

    // Update atom types breakdown
    updateAtomTypesBreakdown(typeCountMap);
}

function processTypeList(types) {
    if (!types || !Array.isArray(types)) {
        console.log('Invalid or empty types list received');
        types = [];
    }

    console.log('Processing type list:', types.length, 'types');

    atomData.types = types;
    typeCount.textContent = types.length.toLocaleString();
}

function updateStats(stats) {
    atomCount.textContent = stats.total.toLocaleString();
    nodeCount.textContent = stats.nodes.toLocaleString();
    linkCount.textContent = stats.links.toLocaleString();
    typeCount.textContent = stats.types.toLocaleString();

    // Update timestamp
    const now = new Date();
    lastUpdate.textContent = `Last updated: ${now.toLocaleTimeString()}`;

    // Add pulse animation to updated values
    [atomCount, nodeCount, linkCount, typeCount].forEach(elem => {
        elem.parentElement.classList.add('pulse');
        setTimeout(() => elem.parentElement.classList.remove('pulse'), 2000);
    });
}

function updateAtomTypesBreakdown(typeCountMap) {
    // Clear existing content
    typesList.innerHTML = '';

    if (typeCountMap.size === 0) {
        // Hide the breakdown panel if no types
        atomTypesBreakdown.classList.add('hidden');
        return;
    }

    // Show the breakdown panel
    atomTypesBreakdown.classList.remove('hidden');

    // Sort types by count (descending) and then by name
    const sortedTypes = Array.from(typeCountMap.entries())
        .filter(([type, count]) => count > 0) // Only include types with count > 0
        .sort((a, b) => {
            if (b[1] !== a[1]) {
                return b[1] - a[1]; // Sort by count descending
            }
            return a[0].localeCompare(b[0]); // Then by name ascending
        });

    // Create type items as buttons
    sortedTypes.forEach(([type, count]) => {
        const typeButton = document.createElement('button');
        typeButton.className = 'type-item-button';
        typeButton.setAttribute('data-type', type);
        typeButton.addEventListener('click', () => showAtomsOfType(type));

        const typeName = document.createElement('span');
        typeName.className = 'type-name';
        typeName.textContent = type;

        const typeCount = document.createElement('span');
        typeCount.className = 'type-count';
        typeCount.textContent = count.toLocaleString();

        typeButton.appendChild(typeName);
        typeButton.appendChild(typeCount);
        typesList.appendChild(typeButton);
    });

    console.log(`Displayed ${sortedTypes.length} atom types`);
}

function fetchAtomSpaceStats() {
    if (!isConnected || !socket) {
        showError('Not connected to CogServer');
        return;
    }

    console.log('Fetching AtomSpace stats...');

    // Send the command as a plain string - this is what the demo shows
    const command = 'AtomSpace.getAtoms("Atom", true)';
    console.log('Sending command:', command);
    sendMessage(command);

    // Also fetch types after a short delay
    setTimeout(() => {
        const typesCommand = 'AtomSpace.getSubTypes("TopType", true)';
        console.log('Sending types command:', typesCommand);
        sendMessage(typesCommand);
    }, 500);
}

function sendDebugCommand() {
    const command = debugCommand.value.trim();
    if (!command) return;

    sendMessage(command);
    debugCommand.value = '';
}

function sendMessage(message) {
    if (!isConnected || !socket || socket.readyState !== WebSocket.OPEN) {
        showError('Not connected to CogServer');
        return;
    }

    console.log('Sending message:', message);
    socket.send(message);
}

function showError(message) {
    errorMessage.textContent = message;
    errorPanel.classList.remove('hidden');

    // Auto-hide after 5 seconds
    setTimeout(hideError, 5000);
}

function hideError() {
    errorPanel.classList.add('hidden');
}

function atomToSExpression(atom) {
    // Convert an atom to s-expression format
    // Remove "Node" and "Link" suffixes from type for s-expression format
    const typeBase = atom.type.replace(/Node$/, '').replace(/Link$/, '');

    if (!atom.outgoing || atom.outgoing.length === 0) {
        // It's a node
        if (atom.name !== undefined) {
            // Quote the name if it contains spaces or special characters
            const quotedName = JSON.stringify(atom.name);
            return `(${typeBase} ${quotedName})`;
        } else {
            return `(${typeBase})`;
        }
    } else {
        // It's a link - recursively show its outgoing connections
        const outgoingStr = atom.outgoing.map(outgoingItem => {
            // The outgoing item could be:
            // 1. An atom object directly (nested atom)
            // 2. A reference/handle to another atom
            // 3. A UUID/ID string

            if (typeof outgoingItem === 'object' && outgoingItem !== null) {
                // It's already an atom object, convert it directly
                if (outgoingItem.type) {
                    return atomToSExpression(outgoingItem);
                }
            }

            // Try to find the referenced atom in our data
            const referencedAtom = atomData.atoms?.find(a => {
                // Check various possible handle/id formats
                if (typeof outgoingItem === 'string' || typeof outgoingItem === 'number') {
                    return (a.handle === outgoingItem) ||
                           (a.uuid === outgoingItem) ||
                           (a.id === outgoingItem) ||
                           (a.name === outgoingItem); // Sometimes might reference by name
                }
                return false;
            });

            if (referencedAtom) {
                return atomToSExpression(referencedAtom);
            } else {
                // Fallback for unresolved references
                return `<unresolved:${JSON.stringify(outgoingItem)}>`;
            }
        }).join(' ');

        // Return the link with its nested s-expressions
        if (outgoingStr) {
            return `(${typeBase} ${outgoingStr})`;
        } else {
            return `(${typeBase})`;
        }
    }
}

function showAtomsOfType(type) {
    console.log(`Showing atoms of type: ${type}`);

    // Filter atoms by type
    const atomsOfType = atomData.atoms?.filter(atom => atom.type === type) || [];

    // Update title
    atomListingTitle.textContent = `${type} Atoms (${atomsOfType.length})`;

    // Clear content
    atomListingContent.innerHTML = '';

    if (atomsOfType.length === 0) {
        atomListingContent.innerHTML = '<div class="no-atoms">No atoms of this type found</div>';
    } else {
        // Create a pre element for the s-expression listing
        const preElement = document.createElement('pre');
        preElement.className = 'atom-sexpr-list';

        // Convert each atom to s-expression and add to the listing
        const sExpressions = atomsOfType.map(atom => atomToSExpression(atom));
        preElement.textContent = sExpressions.join('\n');

        atomListingContent.appendChild(preElement);
    }

    // Show the panel
    atomListingPanel.classList.remove('hidden');
}

function hideAtomListing() {
    atomListingPanel.classList.add('hidden');
    atomListingContent.innerHTML = '';
}

function openStatsPage() {
    // Get the current server URL from the input field
    const serverUrl = serverInput.value.trim();

    if (!serverUrl) {
        showError('Please enter a CogServer URL first');
        return;
    }

    // Convert ws:// to http:// and remove trailing slash if present
    let statsUrl = serverUrl.replace(/^ws:\/\//, 'http://').replace(/^wss:\/\//, 'https://');

    // Remove trailing slash
    if (statsUrl.endsWith('/')) {
        statsUrl = statsUrl.slice(0, -1);
    }

    // Add /stats path
    statsUrl = statsUrl + '/stats';

    console.log('Opening stats page:', statsUrl);

    // Open in new tab
    window.open(statsUrl, '_blank');
}
