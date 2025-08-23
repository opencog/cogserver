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
            margin: 5,
            widthConstraint: {
                maximum: 150,
                minimum: 40
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
                enabled: false  // Straight lines
            }
        },
        physics: {
            enabled: true,
            solver: 'hierarchicalRepulsion',
            hierarchicalRepulsion: {
                nodeDistance: 150,
                centralGravity: 0.0,
                springLength: 100,
                springConstant: 0.01,
                damping: 0.09
            },
            stabilization: {
                iterations: 150
            }
        },
        layout: {
            hierarchical: {
                enabled: true,
                direction: 'UD',  // Up-Down: root at top, nodes at bottom
                sortMethod: 'hubsize',  // This preserves insertion order better
                levelSeparation: 150,
                nodeSpacing: 100,
                treeSpacing: 200,
                blockShifting: false,  // Prevent reordering of siblings
                edgeMinimization: false  // Don't minimize edge crossings to preserve order
            }
        },
        interaction: {
            hover: true,
            tooltipDelay: 200
        }
    };

    network = new vis.Network(container, data, options);

    // Add network event handlers
    network.on('click', function(params) {
        // Handle node clicks
        if (params.nodes.length > 0) {
            const nodeId = params.nodes[0];
            const node = nodes.get(nodeId);
            if (node && node.atom) {
                // Check if we're in graph view mode
                const layoutSelect = document.getElementById('layoutSelect');
                const layoutType = layoutSelect ? layoutSelect.value : 'hierarchical';
                if (layoutType === 'graph' && typeof fetchIncomingSetForGraph === 'function') {
                    // Use graph view's special fetch function
                    fetchIncomingSetForGraph(node.atom, nodeId);
                } else {
                    // Use regular fetch function
                    fetchIncomingSet(node.atom, nodeId);
                }
            }
        }
        // Handle edge clicks
        else if (params.edges.length > 0) {
            const edgeId = params.edges[0];
            const edge = edges.get(edgeId);
            if (edge) {
                removeNodeAndParents(edge.from);
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

    // Ensure the URL ends with /json for the JSON endpoint
    let jsonUrl = serverUrl;
    if (!jsonUrl.endsWith('/json')) {
        if (!jsonUrl.endsWith('/')) {
            jsonUrl += '/';
        }
        jsonUrl += 'json';
    }

    console.log('Attempting to connect to:', jsonUrl);
    updateStatus('Connecting to server...', 'loading');

    try {
        socket = new WebSocket(jsonUrl);
    } catch (e) {
        console.error('Failed to create WebSocket:', e);
        updateStatus('Failed to create connection', 'error');
        return;
    }

    socket.onopen = function() {
        console.log('WebSocket connected');
        updateStatus('Connected to server', 'connected');
    };

    socket.onmessage = function(event) {
        try {
            const response = JSON.parse(event.data);
            console.log('Raw server response:', response);

            // Handle wrapped response format {success: true/false, result: ...}
            if (response.hasOwnProperty('success')) {
                handleServerResponse(response);
            } else {
                // Handle unwrapped response for backward compatibility
                handleServerResponse({success: true, result: response});
            }
        } catch (e) {
            console.error('Failed to parse server response:', e);
            console.error('Raw data:', event.data);
        }
    };

    socket.onerror = function(error) {
        console.error('WebSocket error:', error);
        updateStatus('Connection error', 'error');
    };

    socket.onclose = function(event) {
        console.log('WebSocket disconnected:', event.code, event.reason);
        updateStatus('Disconnected from server', 'error');
    };

    // Add a timeout to detect if connection fails
    setTimeout(function() {
        if (socket.readyState === WebSocket.CONNECTING) {
            console.warn('Connection timeout - still connecting after 5 seconds');
            updateStatus('Connection timeout', 'error');
            socket.close();
        }
    }, 5000);
}

function addAtomToGraph(atom, parentId, depth, order = 0) {
    // Create a unique identifier for this atom
    const atomKey = atomToKey(atom);

    // Check if we've already processed this atom
    if (atomNodeMap.has(atomKey)) {
        const existingNodeId = atomNodeMap.get(atomKey);

        // If we have a parent, we need to ensure proper hierarchy
        if (parentId !== null) {
            const parentNode = nodes.get(parentId);
            const existingNode = nodes.get(existingNodeId);

            if (parentNode && existingNode) {
                // If this node's current level is not deeper than parent's level,
                // we need to move it down to maintain hierarchy
                if (existingNode.level <= parentNode.level) {
                    // Place it one level below the parent
                    const newLevel = parentNode.level + 1;
                    nodes.update({
                        id: existingNodeId,
                        level: newLevel
                    });

                    // Recursively update all children of this node to maintain hierarchy
                    updateChildrenLevels(existingNodeId, newLevel);
                }
                // Always add the edge from parent to child
                addEdgeIfNotExists(parentId, existingNodeId);
            }
        }
        return existingNodeId;
    }

    // Create a new node
    const nodeId = nodeIdCounter++;
    const nodeLabel = createCompactLabel(atom);
    const nodeColor = getNodeColor(atom.type);

    nodes.add({
        id: nodeId,
        label: nodeLabel,
        color: nodeColor,
        atom: atom,
        level: depth,
        x: order * 150,  // Use order to hint at horizontal position
        title: atomToSExpression(atom) // Full format for tooltip
    });

    atomNodeMap.set(atomKey, nodeId);

    // Add edge from parent if exists
    if (parentId !== null) {
        edges.add({
            from: parentId,
            to: nodeId,
            arrows: {
                to: {
                    enabled: true
                }
            }
        });
    }

    // Process outgoing links if this is a link and we haven't reached max depth
    if (atom.outgoing && atom.outgoing.length > 0 && depth < currentDepth) {
        atom.outgoing.forEach((outgoing, index) => {
            if (typeof outgoing === 'object' && outgoing !== null) {
                addAtomToGraph(outgoing, nodeId, depth + 1, index);
            }
        });
    }

    return nodeId;
}

function updateChildrenLevels(nodeId, parentLevel) {
    // Find all edges where this node is the parent
    const childEdges = edges.get({
        filter: function(edge) {
            return edge.from === nodeId;
        }
    });

    // Update each child's level if needed
    childEdges.forEach(edge => {
        const childNode = nodes.get(edge.to);
        if (childNode && childNode.level <= parentLevel) {
            const newChildLevel = parentLevel + 1;
            nodes.update({
                id: edge.to,
                level: newChildLevel
            });
            // Recursively update this child's children
            updateChildrenLevels(edge.to, newChildLevel);
        }
    });
}

function removeNodeAndParents(nodeId) {
    // Collect all nodes to remove (this node and all its parents)
    const nodesToRemove = new Set();
    const edgesToRemove = new Set();

    // Recursive function to find all parent nodes
    function collectParents(currentNodeId) {
        if (nodesToRemove.has(currentNodeId)) {
            return; // Already processed
        }

        nodesToRemove.add(currentNodeId);

        // Find all edges where this node is the parent (from)
        const childEdges = edges.get({
            filter: function(edge) {
                return edge.from === currentNodeId;
            }
        });
        childEdges.forEach(edge => {
            edgesToRemove.add(edge.id);
        });

        // Find all edges where this node is the child (to)
        const parentEdges = edges.get({
            filter: function(edge) {
                return edge.to === currentNodeId;
            }
        });

        // For each parent edge, recursively collect the parent node
        parentEdges.forEach(edge => {
            edgesToRemove.add(edge.id);
            collectParents(edge.from);
        });
    }

    // Start collection from the clicked node
    collectParents(nodeId);

    // Remove all collected edges
    edges.remove(Array.from(edgesToRemove));

    // Remove all collected nodes
    const nodeIdsToRemove = Array.from(nodesToRemove);
    nodes.remove(nodeIdsToRemove);

    // Clean up atomNodeMap
    atomNodeMap.forEach((value, key) => {
        if (nodesToRemove.has(value)) {
            atomNodeMap.delete(key);
        }
    });

    // Update status
    updateStatus(`Removed ${nodesToRemove.size} node(s)`, 'connected');
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
            to: to,
            arrows: {
                to: {
                    enabled: true
                }
            }
        });
    }
}

function createCompactLabel(atom) {
    // Create a compact label for display in the graph
    const typeBase = atom.type.replace(/Node$/, '').replace(/Link$/, '');

    if (!atom.outgoing || atom.outgoing.length === 0) {
        // It's a Node
        if (atom.name !== undefined) {
            // Show only first 14 characters of the name, without quotes
            const name = String(atom.name);
            return name.length > 14 ? name.substring(0, 14) : name;
        }
        // If no name, show truncated type
        return typeBase.length > 14 ? typeBase.substring(0, 14) : typeBase;
    } else {
        // It's a Link - show only first 4 letters without parenthesis
        return typeBase.length > 4 ? typeBase.substring(0, 4) : typeBase;
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

function atomToSExpression(atom, indent = 0) {
    const typeBase = atom.type.replace(/Node$/, '').replace(/Link$/, '');

    if (!atom.outgoing || atom.outgoing.length === 0) {
        if (atom.name !== undefined) {
            const quotedName = JSON.stringify(atom.name);
            return `(${typeBase} ${quotedName})`;
        }
        return `(${typeBase})`;
    } else {
        // Links with proper indentation using non-breaking spaces
        const nextIndent = indent + 1;
        // Use non-breaking spaces (\u00A0) for indentation to preserve it in tooltips
        const nextIndentStr = '\u00A0\u00A0\u00A0\u00A0'.repeat(nextIndent);
        const outgoingStrs = atom.outgoing.map(item => {
            if (typeof item === 'object' && item !== null) {
                return nextIndentStr + atomToSExpression(item, nextIndent);
            } else if (typeof item === 'string') {
                // It's a reference to another atom by ID or handle
                return nextIndentStr + `(Atom "${item}")`;
            } else {
                return nextIndentStr + String(item);
            }
        });
        return `(${typeBase}\n${outgoingStrs.join('\n')})`;
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


function setupEventHandlers() {
    // Expand button
    document.getElementById('expandBtn').addEventListener('click', function() {
        const selectedNodes = network.getSelectedNodes();
        if (selectedNodes.length > 0) {
            const node = nodes.get(selectedNodes[0]);
            if (node && node.atom) {
                fetchIncomingSet(node.atom, selectedNodes[0]);
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

        if (layoutType === 'graph') {
            // Switch to graph view mode - need to redraw everything
            nodes.clear();
            edges.clear();
            atomNodeMap.clear();
            nodeIdCounter = 1;

            // Use graph view mode with special edge handling
            initializeGraphView();
            options = getGraphViewOptions();
        } else {
            // If switching from graph mode to another mode, need to redraw
            const previousLayout = this.getAttribute('data-previous-layout');
            if (previousLayout === 'graph') {
                nodes.clear();
                edges.clear();
                atomNodeMap.clear();
                nodeIdCounter = 1;

                // Re-add the root atoms using normal tree view
                if (rootAtoms && rootAtoms.length > 0) {
                    rootAtoms.forEach(atom => {
                        addAtomToGraph(atom, null, 0);
                    });
                }
            }

            if (layoutType === 'hierarchical') {
                options = {
                    edges: {
                        smooth: {
                            enabled: false  // Straight lines
                        }
                    },
                    physics: {
                        enabled: true,
                        solver: 'hierarchicalRepulsion',
                        hierarchicalRepulsion: {
                            nodeDistance: 150,
                            centralGravity: 0.0,
                            springLength: 100,
                            springConstant: 0.01,
                            damping: 0.09
                        }
                    },
                    layout: {
                        hierarchical: {
                            enabled: true,
                            direction: 'UD',  // Up-Down: root at top, nodes at bottom
                            sortMethod: 'hubsize',  // This preserves insertion order better
                            levelSeparation: 150,
                            nodeSpacing: 100,
                            treeSpacing: 200,
                            blockShifting: false,  // Prevent reordering of siblings
                            edgeMinimization: false  // Don't minimize edge crossings to preserve order
                        }
                    }
                };
            } else {
                options = {
                    edges: {
                        smooth: {
                            enabled: true,  // Allow curves in network mode
                            type: 'dynamic'
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
                        }
                    },
                    layout: {
                        hierarchical: {
                            enabled: false
                        }
                    }
                };
            }
        }

        network.setOptions(options);
        this.setAttribute('data-previous-layout', layoutType);
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

    // Check current layout mode
    const layoutSelect = document.getElementById('layoutSelect');
    const layoutType = layoutSelect ? layoutSelect.value : 'hierarchical';

    if (layoutType === 'graph') {
        // Use graph view refresh
        initializeGraphView();
    } else {
        // Re-add the root atoms using normal tree view
        if (rootAtoms && rootAtoms.length > 0) {
            rootAtoms.forEach(atom => {
                addAtomToGraph(atom, null, 0);
            });
            network.fit();
        }
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
    console.log('Server response:', response);

    // Check if this is a response to getIncoming
    if (pendingIncomingRequest) {
        if (response.success && response.result) {
            const incomingAtoms = response.result;
            const targetNodeId = pendingIncomingRequest.nodeId;

            // Check if this is a graph view request
            if (pendingIncomingRequest.isGraphView && typeof processIncomingSetForGraph === 'function') {
                // Use graph view's special processing
                processIncomingSetForGraph(incomingAtoms, targetNodeId);
            } else {
                // Regular tree view processing
                incomingAtoms.forEach(atom => {
                    // Add the incoming atom to the graph
                    const incomingNodeId = addAtomToGraph(atom, null, 0);

                    // Find which outgoing atom matches our target and connect to it
                    if (atom.outgoing) {
                        atom.outgoing.forEach((outgoing, index) => {
                            // Check if this outgoing matches our target atom
                            if (isMatchingAtom(outgoing, pendingIncomingRequest.atom)) {
                                // Connect the incoming atom to the existing node
                                addEdgeIfNotExists(incomingNodeId, targetNodeId);
                            }
                        });
                    }
                });

                // Refit the network to show the new nodes
                network.fit();
                updateStatus(`Added ${incomingAtoms.length} incoming links`, 'connected');
            }
        } else {
            updateStatus('No incoming links found', 'connected');
        }

        pendingIncomingRequest = null;
    }
}

// Helper function to check if two atoms match
function isMatchingAtom(atom1, atom2) {
    if (typeof atom1 === 'string' || typeof atom2 === 'string') {
        return false; // Can't match string references accurately
    }
    if (atom1.type !== atom2.type) {
        return false;
    }
    if (atom1.name !== undefined && atom2.name !== undefined) {
        return atom1.name === atom2.name;
    }
    // For links, would need to compare outgoing, but that gets complex
    return atomToKey(atom1) === atomToKey(atom2);
}

// Global variable to track pending incoming set request
let pendingIncomingRequest = null;

function fetchIncomingSet(atom, nodeId) {
    if (!socket || socket.readyState !== WebSocket.OPEN) {
        updateStatus('Not connected to server', 'error');
        return;
    }

    updateStatus('Fetching incoming links...', 'loading');

    // Construct the command to get incoming set
    let atomSpec;
    if (atom.name !== undefined) {
        // It's a node - escape the name properly for JSON
        const escapedName = JSON.stringify(atom.name);
        atomSpec = `{"type": "${atom.type}", "name": ${escapedName}}`;
    } else {
        // It's a link or complex atom - use full JSON serialization
        atomSpec = JSON.stringify(atom);
    }

    const command = `AtomSpace.getIncoming(${atomSpec})`;
    console.log('Getting incoming set for atom:', command);

    // Store request info for when we receive the response
    pendingIncomingRequest = {
        atom: atom,
        nodeId: nodeId
    };

    // Send the command
    socket.send(command);
}