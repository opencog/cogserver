// AtomSpace Graph Visualization using vis-network
// This file handles the graph rendering and interaction logic

// Global variables
let network = null;
let nodes = null;
let edges = null;
let socket = null;
let rootAtoms = [];
let serverUrl = null;
let currentDepth = 2;
let processedAtoms = new Set();
let atomNodeMap = new Map();
let nodeIdCounter = 1;

// Initialize on page load
document.addEventListener('DOMContentLoaded', function() {
    // Parse URL parameters
    const urlParams = new URLSearchParams(window.location.search);
    const atomParam = urlParams.get('atom');
    const atomsParam = urlParams.get('atoms');
    const serverParam = urlParams.get('server');

    // Handle single atom parameter
    if (atomParam) {
        try {
            const atom = JSON.parse(decodeURIComponent(atomParam));
            rootAtoms = [atom];
            console.log('Root atom:', atom);
        } catch (e) {
            console.error('Failed to parse atom data:', e);
            updateStatus('Failed to parse atom data', 'error');
            return;
        }
    }

    // Handle multiple atoms parameter
    if (atomsParam) {
        try {
            rootAtoms = JSON.parse(decodeURIComponent(atomsParam));
            console.log('Root atoms:', rootAtoms);
        } catch (e) {
            console.error('Failed to parse atoms data:', e);
            updateStatus('Failed to parse atoms data', 'error');
            return;
        }
    }

    if (serverParam) {
        serverUrl = decodeURIComponent(serverParam);
        console.log('Server URL:', serverUrl);
    }

    // Initialize the graph
    initializeGraph();

    // Connect to server if URL provided
    if (serverUrl) {
        connectToServer();
    }

    // Set up event handlers
    setupEventHandlers();
});

function initializeGraph() {
    // Check if vis is loaded
    if (typeof vis === 'undefined') {
        console.error('vis-network library not loaded');
        updateStatus('Error: vis-network library not loaded', 'error');
        return;
    }

    // Create initial nodes and edges arrays
    nodes = new vis.DataSet();
    edges = new vis.DataSet();

    // Get the container
    const container = document.getElementById('mynetwork');

    if (!container) {
        console.error('Container element #mynetwork not found');
        updateStatus('Error: Container element not found', 'error');
        return;
    }

    // Create the network
    const data = {
        nodes: nodes,
        edges: edges
    };

    const options = {
        nodes: {
            shape: 'box',
            font: {
                size: 14,
                face: 'monospace'
            },
            margin: 10,
            widthConstraint: {
                maximum: 200
            }
        },
        edges: {
            arrows: {
                to: {
                    enabled: true,
                    scaleFactor: 0.5
                }
            },
            smooth: {
                type: 'cubicBezier',
                forceDirection: 'vertical',
                roundness: 0.4
            }
        },
        physics: {
            enabled: true,
            solver: 'forceAtlas2Based',
            forceAtlas2Based: {
                gravitationalConstant: -50,
                centralGravity: 0.01,
                springLength: 100,
                springConstant: 0.08
            },
            stabilization: {
                iterations: 150
            }
        },
        layout: {
            improvedLayout: true
        },
        interaction: {
            hover: true,
            tooltipDelay: 200
        }
    };

    network = new vis.Network(container, data, options);

    // Add network event handlers
    network.on('doubleClick', function(params) {
        if (params.nodes.length > 0) {
            const nodeId = params.nodes[0];
            const node = nodes.get(nodeId);
            if (node && node.atom) {
                expandNode(node.atom);
            }
        }
    });

    // Add the root atoms to the graph
    if (rootAtoms && rootAtoms.length > 0) {
        console.log('Adding root atoms to graph:', rootAtoms);
        rootAtoms.forEach(atom => {
            addAtomToGraph(atom, null, 0);
        });
        network.fit();
        updateStatus(`Graph initialized with ${rootAtoms.length} atom(s)`, 'connected');
    } else {
        updateStatus('No atoms to display', 'error');
        console.warn('No root atoms found');
    }
}

function connectToServer() {
    if (!serverUrl) {
        updateStatus('No server URL provided', 'error');
        return;
    }

    updateStatus('Connecting to server...', 'loading');

    socket = new WebSocket(serverUrl);

    socket.onopen = function() {
        console.log('WebSocket connected');
        updateStatus('Connected to server', 'connected');
    };

    socket.onmessage = function(event) {
        try {
            const response = JSON.parse(event.data);
            handleServerResponse(response);
        } catch (e) {
            console.error('Failed to parse server response:', e);
        }
    };

    socket.onerror = function(error) {
        console.error('WebSocket error:', error);
        updateStatus('Connection error', 'error');
    };

    socket.onclose = function() {
        console.log('WebSocket disconnected');
        updateStatus('Disconnected from server', 'error');
    };
}

function addAtomToGraph(atom, parentId, depth) {
    // Create a unique identifier for this atom
    const atomKey = atomToKey(atom);

    // Check if we've already processed this atom
    if (atomNodeMap.has(atomKey)) {
        // If we have a parent, just add an edge
        if (parentId !== null) {
            const existingNodeId = atomNodeMap.get(atomKey);
            addEdgeIfNotExists(parentId, existingNodeId);
        }
        return atomNodeMap.get(atomKey);
    }

    // Create a new node
    const nodeId = nodeIdCounter++;
    const nodeLabel = atomToSExpression(atom, true); // true for compact format
    const nodeColor = getNodeColor(atom.type);

    nodes.add({
        id: nodeId,
        label: nodeLabel,
        color: nodeColor,
        atom: atom,
        level: depth,
        title: atomToSExpression(atom, false) // Full format for tooltip
    });

    atomNodeMap.set(atomKey, nodeId);

    // Add edge from parent if exists
    if (parentId !== null) {
        edges.add({
            from: parentId,
            to: nodeId
        });
    }

    // Process outgoing links if this is a link and we haven't reached max depth
    if (atom.outgoing && atom.outgoing.length > 0 && depth < currentDepth) {
        atom.outgoing.forEach((outgoing, index) => {
            if (typeof outgoing === 'object' && outgoing !== null) {
                addAtomToGraph(outgoing, nodeId, depth + 1);
            }
        });
    }

    return nodeId;
}

function addEdgeIfNotExists(from, to) {
    const existingEdges = edges.get({
        filter: function(edge) {
            return edge.from === from && edge.to === to;
        }
    });

    if (existingEdges.length === 0) {
        edges.add({
            from: from,
            to: to
        });
    }
}

function atomToKey(atom) {
    // Create a unique key for an atom
    if (atom.name !== undefined) {
        return `${atom.type}:${atom.name}`;
    } else if (atom.outgoing) {
        return `${atom.type}:[${atom.outgoing.map(o =>
            typeof o === 'object' ? atomToKey(o) : o
        ).join(',')}]`;
    } else {
        return `${atom.type}:${JSON.stringify(atom)}`;
    }
}

function atomToSExpression(atom, compact = false) {
    const typeBase = atom.type.replace(/Node$/, '').replace(/Link$/, '');

    if (!atom.outgoing || atom.outgoing.length === 0) {
        if (atom.name !== undefined) {
            const quotedName = JSON.stringify(atom.name);
            return `(${typeBase} ${quotedName})`;
        }
        return `(${typeBase})`;
    } else {
        if (compact) {
            // Compact format for node labels
            return `(${typeBase} ...)`;
        } else {
            // Full format for tooltips
            const outgoingStrs = atom.outgoing.map(item => {
                if (typeof item === 'object' && item !== null) {
                    return atomToSExpression(item, false);
                } else if (typeof item === 'string') {
                    return `(Atom "${item}")`;
                } else {
                    return String(item);
                }
            });
            return `(${typeBase} ${outgoingStrs.join(' ')})`;
        }
    }
}

function getNodeColor(type) {
    // Color scheme for different atom types
    const colors = {
        'ConceptNode': '#4CAF50',
        'PredicateNode': '#2196F3',
        'NumberNode': '#FF9800',
        'TypeNode': '#9C27B0',
        'ListLink': '#00BCD4',
        'EvaluationLink': '#FFC107',
        'InheritanceLink': '#795548',
        'MemberLink': '#607D8B',
        'DefineLink': '#E91E63',
        'ImplicationLink': '#3F51B5'
    };

    return {
        background: colors[type] || '#9E9E9E',
        border: '#000000',
        highlight: {
            background: '#FFD700',
            border: '#000000'
        }
    };
}

function expandNode(atom) {
    if (!socket || socket.readyState !== WebSocket.OPEN) {
        updateStatus('Not connected to server', 'error');
        return;
    }

    // Get incoming and outgoing links for this atom
    updateStatus('Expanding node...', 'loading');

    // For now, just expand the existing atom's connections
    // In a full implementation, we'd query the server for more connections
    const nodeKey = atomToKey(atom);
    const nodeId = atomNodeMap.get(nodeKey);

    if (nodeId && atom.outgoing) {
        atom.outgoing.forEach((outgoing, index) => {
            if (typeof outgoing === 'object' && outgoing !== null) {
                addAtomToGraph(outgoing, nodeId, 1);
            }
        });
        network.fit();
        updateStatus('Node expanded', 'connected');
    }
}

function setupEventHandlers() {
    // Expand button
    document.getElementById('expandBtn').addEventListener('click', function() {
        const selectedNodes = network.getSelectedNodes();
        if (selectedNodes.length > 0) {
            const node = nodes.get(selectedNodes[0]);
            if (node && node.atom) {
                expandNode(node.atom);
            }
        } else {
            updateStatus('Select a node to expand', 'error');
        }
    });

    // Reset view button
    document.getElementById('resetBtn').addEventListener('click', function() {
        network.fit();
    });

    // Depth input
    document.getElementById('depthInput').addEventListener('change', function(e) {
        currentDepth = parseInt(e.target.value);
        refreshGraph();
    });

    // Layout select
    document.getElementById('layoutSelect').addEventListener('change', function(e) {
        const layoutType = e.target.value;
        let options = {};

        if (layoutType === 'hierarchical') {
            options = {
                layout: {
                    hierarchical: {
                        direction: 'UD',
                        sortMethod: 'directed',
                        shakeTowards: 'roots'
                    }
                }
            };
        } else {
            options = {
                layout: {
                    hierarchical: false
                }
            };
        }

        network.setOptions(options);
    });

    // Refresh button
    document.getElementById('refreshBtn').addEventListener('click', function() {
        refreshGraph();
    });
}

function refreshGraph() {
    // Clear the graph
    nodes.clear();
    edges.clear();
    atomNodeMap.clear();
    nodeIdCounter = 1;

    // Re-add the root atoms
    if (rootAtoms && rootAtoms.length > 0) {
        rootAtoms.forEach(atom => {
            addAtomToGraph(atom, null, 0);
        });
        network.fit();
    }

    updateStatus('Graph refreshed', 'connected');
}

function updateStatus(message, className) {
    const statusElement = document.getElementById('status');
    statusElement.textContent = message;
    statusElement.className = className || '';
}

function handleServerResponse(response) {
    // Handle responses from the server
    // This would be extended to handle various query responses
    console.log('Server response:', response);
}