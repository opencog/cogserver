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
    
    // Set up event listeners
    connectBtn.addEventListener('click', toggleConnection);
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
    
    // Fetch initial stats
    fetchAtomSpaceStats();
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
    
    // Hide stats panel
    atomspaceStats.classList.add('hidden');
    
    socket = null;
}

function onMessage(event) {
    console.log('Received message:', event.data);
    
    try {
        // Parse JSON response (we're always using JSON endpoint)
        const data = JSON.parse(event.data);
        handleJSONResponse(data);
    } catch (err) {
        // If not JSON, just display the raw message
        console.error('Failed to parse JSON:', err);
        debugResponse.textContent = event.data;
    }
}

function onError(event) {
    console.error('WebSocket error:', event);
    showError('Connection error: Unable to connect to ' + serverURL);
}

function handleJSONResponse(data) {
    console.log('Handling JSON response:', data);
    
    // Display in debug console
    debugResponse.textContent = JSON.stringify(data, null, 2);
    
    // Handle different response types
    if (data.result && Array.isArray(data.result)) {
        // Response from getAtoms
        processAtomList(data.result);
    } else if (data.result && data.result.atoms) {
        // Alternative format with atoms array
        processAtomList(data.result.atoms);
    } else if (data.types && Array.isArray(data.types)) {
        // Response from getSubTypes
        processTypeList(data.types);
    }
}

function processAtomList(atoms) {
    console.log('Processing atom list:', atoms.length, 'atoms');
    
    atomData.atoms = atoms;
    atomData.totalCount = atoms.length;
    
    // Count nodes and links
    let nodes = 0;
    let links = 0;
    const types = new Set();
    
    atoms.forEach(atom => {
        types.add(atom.type);
        
        // Check if it's a node or link based on whether it has outgoing connections
        if (atom.outgoing && atom.outgoing.length > 0) {
            links++;
        } else {
            nodes++;
        }
    });
    
    // Update UI
    updateStats({
        total: atoms.length,
        nodes: nodes,
        links: links,
        types: types.size
    });
}

function processTypeList(types) {
    console.log('Processing type list:', types.length, 'types');
    
    atomData.types = types;
    typeCount.textContent = types.length;
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

function fetchAtomSpaceStats() {
    if (!isConnected || !socket) {
        showError('Not connected to CogServer');
        return;
    }
    
    console.log('Fetching AtomSpace stats...');
    
    // Always use JSON commands
    const command = 'AtomSpace.getAtoms("Atom", true)';
    sendMessage(command);
    
    // Also fetch types
    setTimeout(() => {
        sendMessage('AtomSpace.getSubTypes("TopType", true)');
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