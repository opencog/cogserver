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
                // Check if this is a null value response for getValueAtKey
                if (atomData.pendingValueRequest && result === 'null') {
                    console.log('Received null value at key');
                    const { display } = atomData.pendingValueRequest;
                    displayKeyValue(null, display);
                    atomData.pendingValueRequest = null;
                } else {
                    // Version response or other string results
                    console.log('Received string result:', result);
                    // If it looks like a version number, just log it
                    if (result.match(/^\d+\.\d+\.\d+/)) {
                        console.log('CogServer JSON API version:', result);
                    }
                }
            } else if (Array.isArray(result)) {
                // Check if this is a keys response
                if (atomData.pendingKeysRequest) {
                    console.log('Received keys response:', result);
                    const { display, atom } = atomData.pendingKeysRequest;
                    displayAtomKeys(result, display, atom);
                    atomData.pendingKeysRequest = null;
                    return; // Don't process further
                } else if (result.length === 0) {
                    // Empty array - could be empty atom list
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
                // Check if this is a value response for getValueAtKey
                if (atomData.pendingValueRequest) {
                    console.log('Received value at key:', result);
                    const { display } = atomData.pendingValueRequest;
                    displayKeyValue(result, display);
                    atomData.pendingValueRequest = null;
                } else {
                    // Check if it's the reportCounts response (object with type names as keys)
                    const keys = Object.keys(result);
                    if (keys.length > 0 && keys.every(key => typeof result[key] === 'number')) {
                        console.log('Received atom counts:', result);
                        processAtomCounts(result);
                    } else {
                        // Could be a single atom or other object
                        console.log('Received object result:', result);
                    }
                }
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

    // Check if this is for displaying atoms of a specific type
    if (atomData.pendingTypeDisplay) {
        const type = atomData.pendingTypeDisplay;
        atomData.pendingTypeDisplay = null;

        // Update the listing panel with the atoms
        if (atoms.length === 0) {
            atomListingContent.innerHTML = '<div class="no-atoms">No atoms of this type found</div>';
        } else {
            // Create a container for the clickable atoms
            const container = document.createElement('div');
            container.className = 'atom-sexpr-list';

            // Store these atoms temporarily for s-expression conversion
            const tempAtoms = atomData.atoms;
            atomData.atoms = atoms;

            // Create clickable atom elements
            atoms.forEach(atom => {
                const atomElement = createClickableAtom(atom);
                container.appendChild(atomElement);
            });

            // Restore the original atoms
            atomData.atoms = tempAtoms;

            atomListingContent.innerHTML = '';
            atomListingContent.appendChild(container);
        }

        console.log(`Displayed ${atoms.length} atoms of type ${type}`);
        return;
    }

    // This is the old full atom list processing - keeping for backward compatibility
    // but it shouldn't be called anymore since we're using reportCounts()
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

function processAtomCounts(counts) {
    console.log('Processing atom counts from reportCounts()');

    // Calculate totals
    let totalAtoms = 0;
    let nodes = 0;
    let links = 0;
    const typeCountMap = new Map();

    for (const [typeName, count] of Object.entries(counts)) {
        totalAtoms += count;
        typeCountMap.set(typeName, count);

        // Determine if it's a Node or Link based on the type name
        if (typeName.endsWith('Node')) {
            nodes += count;
        } else if (typeName.endsWith('Link')) {
            links += count;
        } else {
            // For types that don't follow the naming convention,
            // we'll need to check the type hierarchy
            // For now, assume it's a node if not explicitly a link
            nodes += count;
        }
    }

    console.log(`Stats from reportCounts - Total: ${totalAtoms}, Nodes: ${nodes}, Links: ${links}, Types: ${typeCountMap.size}`);

    // Update UI
    updateStats({
        total: totalAtoms,
        nodes: nodes,
        links: links,
        types: typeCountMap.size
    });

    // Update atom types breakdown
    updateAtomTypesBreakdown(typeCountMap);

    // Store the counts for later use
    atomData.counts = counts;
    atomData.totalCount = totalAtoms;
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

    // Use the new reportCounts() command for efficient stats gathering
    const command = 'AtomSpace.reportCounts()';
    console.log('Sending command:', command);
    sendMessage(command);

    // Also fetch types after a short delay for additional type information
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

function createClickableAtom(atom) {
    // Create a container for the atom and its keys
    const atomContainer = document.createElement('div');
    atomContainer.className = 'atom-container';

    // Create the clickable atom element
    const atomElement = document.createElement('div');
    atomElement.className = 'atom-clickable';

    // Generate unique ID for this atom display
    const atomId = `atom-${atom.type}-${atom.name || 'link'}-${Math.random().toString(36).substr(2, 9)}`;
    atomElement.setAttribute('data-atom-id', atomId);

    // Set the s-expression as the content
    const sexpr = atomToSExpression(atom);
    atomElement.textContent = sexpr;

    // Create keys display area (initially hidden)
    const keysDisplay = document.createElement('div');
    keysDisplay.className = 'atom-keys-display hidden';
    keysDisplay.id = `keys-${atomId}`;

    // Add click handler
    atomElement.addEventListener('click', () => {
        handleAtomClick(atom, keysDisplay, atomElement);
    });

    // Add both elements to container
    atomContainer.appendChild(atomElement);
    atomContainer.appendChild(keysDisplay);

    return atomContainer;
}

function handleAtomClick(atom, keysDisplay, atomElement) {
    // Toggle keys display
    if (!keysDisplay.classList.contains('hidden')) {
        keysDisplay.classList.add('hidden');
        atomElement.classList.remove('expanded');
        return;
    }

    // Show loading state
    keysDisplay.innerHTML = '<span class="loading-keys">Loading keys...</span>';
    keysDisplay.classList.remove('hidden');
    atomElement.classList.add('expanded');

    // Construct the command to get keys
    // For nodes with names, create the atom specification
    // For links, we need to include the full outgoing set
    let atomSpec;
    if (atom.name !== undefined) {
        // It's a node - escape the name properly for JSON
        const escapedName = JSON.stringify(atom.name);
        atomSpec = `{"type": "${atom.type}", "name": ${escapedName}}`;
    } else {
        // It's a link or complex atom - use full JSON serialization
        atomSpec = JSON.stringify(atom);
    }

    const command = `AtomSpace.getKeys(${atomSpec})`;
    console.log('Getting keys for atom:', command);

    // Store callback for when we receive the response
    atomData.pendingKeysRequest = {
        atom: atom,
        display: keysDisplay,
        element: atomElement
    };

    // Send the command
    if (socket && socket.readyState === WebSocket.OPEN) {
        socket.send(command);
    } else {
        keysDisplay.innerHTML = '<span class="error">Not connected to server</span>';
    }
}

function displayAtomKeys(keys, keysDisplay, parentAtom) {
    if (!keys || keys.length === 0) {
        keysDisplay.innerHTML = '<span class="no-keys">No keys</span>';
    } else {
        keysDisplay.innerHTML = '<div class="keys-header">Keys:</div>';
        const keysList = document.createElement('ul');
        keysList.className = 'keys-list';

        keys.forEach(key => {
            const keyItem = document.createElement('li');
            keyItem.className = 'key-item';

            // Create a clickable key element
            const keyElement = document.createElement('span');
            keyElement.className = 'key-clickable';
            keyElement.textContent = atomToSExpression(key);

            // Create value display area (initially hidden)
            const valueDisplay = document.createElement('div');
            valueDisplay.className = 'key-value-display hidden';

            // Add click handler to fetch value at this key
            keyElement.addEventListener('click', () => {
                handleKeyClick(parentAtom, key, valueDisplay, keyElement);
            });

            keyItem.appendChild(keyElement);
            keyItem.appendChild(valueDisplay);
            keysList.appendChild(keyItem);
        });

        keysDisplay.appendChild(keysList);
    }
}

function handleKeyClick(atom, key, valueDisplay, keyElement) {
    // Toggle value display
    if (!valueDisplay.classList.contains('hidden')) {
        valueDisplay.classList.add('hidden');
        keyElement.classList.remove('expanded');
        return;
    }

    // Show loading state
    valueDisplay.innerHTML = '<span class="loading-value">Loading value...</span>';
    valueDisplay.classList.remove('hidden');
    keyElement.classList.add('expanded');

    // Construct the getValueAtKey command
    const atomSpec = atom.name !== undefined ?
        `{"type": "${atom.type}", "name": ${JSON.stringify(atom.name)}` :
        JSON.stringify(atom).slice(0, -1);  // Remove closing brace to add key

    const keySpec = key.name !== undefined ?
        `{"type": "${key.type}", "name": ${JSON.stringify(key.name)}}` :
        JSON.stringify(key);

    const command = `AtomSpace.getValueAtKey(${atomSpec}, "key": ${keySpec}})`;
    console.log('Getting value at key:', command);

    // Store callback for when we receive the response
    atomData.pendingValueRequest = {
        atom: atom,
        key: key,
        display: valueDisplay,
        element: keyElement
    };

    // Send the command
    if (socket && socket.readyState === WebSocket.OPEN) {
        socket.send(command);
    } else {
        valueDisplay.innerHTML = '<span class="error">Not connected to server</span>';
    }
}

function displayKeyValue(value, valueDisplay) {
    if (!value || value === null) {
        valueDisplay.innerHTML = '<span class="no-value">No value</span>';
    } else {
        // Display the value in s-expression format inline
        const valueContent = document.createElement('span');
        valueContent.className = 'value-content-inline';
        valueContent.textContent = '→ ' + valueToSExpression(value);
        valueDisplay.innerHTML = '';
        valueDisplay.appendChild(valueContent);
    }
}

function valueToSExpression(value) {
    // Convert a value object to s-expression format
    if (!value || !value.type) {
        return String(value);
    }

    // Get the value type
    const valueType = value.type;

    // Get the actual values
    let values = value.value;

    // Handle different value types
    if (valueType === 'StringValue' && Array.isArray(values)) {
        // StringValue: quote each string
        const quotedValues = values.map(v => JSON.stringify(v));
        return `(${valueType} ${quotedValues.join(' ')})`;
    } else if (Array.isArray(values)) {
        // FloatValue, LinkValue, etc.: just space-separated values
        return `(${valueType} ${values.join(' ')})`;
    } else if (valueType === 'TruthValue' && values && Array.isArray(values.value)) {
        // TruthValue has nested structure
        return `(${valueType} ${values.value.join(' ')})`;
    } else {
        // Fallback: try to stringify the value part
        const valueStr = typeof values === 'object' ? JSON.stringify(values) : String(values);
        return `(${valueType} ${valueStr})`;
    }
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

    // Update title with count from reportCounts if available
    const count = atomData.counts?.[type] || 0;
    atomListingTitle.textContent = `${type} Atoms (${count})`;

    // Clear content
    atomListingContent.innerHTML = '';

    // Show loading message
    atomListingContent.innerHTML = '<div class="loading">Loading atoms...</div>';

    // Show the panel
    atomListingPanel.classList.remove('hidden');

    // Fetch atoms of this specific type
    const command = `AtomSpace.getAtoms("${type}", false)`;
    console.log('Fetching atoms of type:', type);

    // Store the type we're fetching for later processing
    atomData.pendingTypeDisplay = type;

    // Send the command
    if (socket && socket.readyState === WebSocket.OPEN) {
        socket.send(command);
    } else {
        atomListingContent.innerHTML = '<div class="error">Not connected to server</div>';
    }
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
