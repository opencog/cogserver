// AtomSpace Graph Visualization using vis-network
// This file handles the graph rendering and interaction logic

// Global variables
let network = null;
let nodes = null;
let edges = null;
let rootAtoms = [];
let serverUrl = null;
let atomNodeMap = new Map();
let nodeIdCounter = 1;

// Operation control variables
let operationCancelled = false;
let pendingOperation = null;
let operationStartTime = null;
let stopButtonTimer = null;
const LARGE_ATOM_THRESHOLD = 300; // Warn if more than 300 atoms
const STOP_BUTTON_DELAY = 2000;   // Show stop button after 2 seconds

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
            // Add to atom cache
            atomSpaceCache.clear();
            atomSpaceCache.addAtom(atom);
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
            // Add to atom cache
            atomSpaceCache.clear();
            rootAtoms.forEach(atom => atomSpaceCache.addAtom(atom));
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

                // Check current atom count and warn if getting large
                const currentAtomCount = atomSpaceCache.getStats().totalAtoms;
                if (currentAtomCount > LARGE_ATOM_THRESHOLD) {
                    showWarningDialog(currentAtomCount, function() {
                        atomSpaceCache.fetchIncomingSet(node.atom);
                    });
                } else {
                    // Start operation tracking for stop button
                    startOperation();
                    // Fetch incoming set via cache
                    atomSpaceCache.fetchIncomingSet(node.atom);
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

    // Connect via atom cache
    atomSpaceCache.connect(jsonUrl);
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

    // Process outgoing links if this is a link (no depth limit)
    if (atom.outgoing && atom.outgoing.length > 0) {
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
    // Get the atom associated with this node
    const node = nodes.get(nodeId);
    if (!node || !node.atom) {
        return;
    }

    // Remove the atom and its parents from the cache
    const removedCount = atomSpaceCache.removeAtomAndParents(node.atom);

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
    updateStatus(`Removed ${nodesToRemove.size} node(s) from display and ${removedCount} atom(s) from cache`, 'connected');
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
    // Set up cache event listeners
    atomSpaceCache.addEventListener('connection', function(event) {
        const status = event.detail.status;
        const message = event.detail.message;
        if (status === 'connected') {
            updateStatus(message, 'connected');
        } else if (status === 'disconnected' || status === 'error') {
            updateStatus(message, 'error');
        }
    });

    atomSpaceCache.addEventListener('update', function(event) {
        const updateType = event.detail.type;

        // Handle cancellation event
        if (updateType === 'operations-cancelled') {
            endOperation();
            return;
        }

        // Handle atoms-removed event - no need to rebuild as removal is already done
        if (updateType === 'atoms-removed') {
            // The visual removal is already handled by removeNodeAndParents
            // This event just confirms cache is in sync
            return;
        }

        // Check if operation was cancelled
        if (operationCancelled) {
            endOperation();
            return;
        }

        if (updateType === 'incoming-set') {
            const parent = event.detail.parent;
            const atoms = event.detail.atoms;

            // Check current layout mode
            const layoutSelect = document.getElementById('layoutSelect');
            const layoutType = layoutSelect ? layoutSelect.value : 'hierarchical';

            if (layoutType === 'graph') {
                // For graph view, delegate to graph-view.js handler
                if (typeof handleGraphViewCacheUpdate === 'function') {
                    handleGraphViewCacheUpdate(parent, atoms);
                }
            } else {
                // For hierarchical/network view, rebuild the entire graph with new atoms
                // This ensures proper bottom-up level calculation
                if (!operationCancelled) {
                    rebuildFromAtomCache();

                    // Refit the network to show the new nodes
                    network.fit();
                    updateStatus(`Added ${atoms.length} incoming links`, 'connected');
                }
            }
        } else if (updateType === 'listlinks-complete') {
            endOperation();
            updateStatus('Ready', 'connected');
        }
    });

    atomSpaceCache.addEventListener('error', function(event) {
        updateStatus(event.detail.message, 'error');
    });

    // Reset view button
    document.getElementById('resetBtn').addEventListener('click', function() {
        network.fit();
    });


    // Layout select
    document.getElementById('layoutSelect').addEventListener('change', function(e) {
        const layoutType = e.target.value;
        const previousLayout = this.getAttribute('data-previous-layout');

        // Destroy and recreate network to ensure clean state
        if (network) {
            network.destroy();
            network = null;
        }

        // Clear all data structures
        nodes = new vis.DataSet();
        edges = new vis.DataSet();
        atomNodeMap.clear();
        nodeIdCounter = 1;

        // Recreate network with fresh options
        const container = document.getElementById('mynetwork');
        let options = {};

        if (layoutType === 'graph') {
            // Use graph view mode with special edge handling
            initializeGraphViewWithAtomCache();
            options = getGraphViewOptions();
        } else {
            // Rebuild from cache for hierarchical/network view
            rebuildFromAtomCache();

            if (layoutType === 'hierarchical') {
                options = {
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
                        smooth: {
                            enabled: false  // Straight lines
                        },
                        arrows: {
                            to: {
                                enabled: true,
                                scaleFactor: 0.5
                            }
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
                            enabled: true,
                            iterations: 1000,
                            updateInterval: 100
                        }
                    },
                    layout: {
                        hierarchical: {
                            enabled: true,
                            direction: 'UD',  // Up-Down: root at top, nodes at bottom
                            sortMethod: 'hubsize',
                            levelSeparation: 150,
                            nodeSpacing: 100,
                            treeSpacing: 200,
                            blockShifting: true,
                            edgeMinimization: true,
                            parentCentralization: true
                        }
                    }
                };
            } else {
                // Network view
                options = {
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
                        smooth: {
                            enabled: true,  // Allow curves in network mode
                            type: 'dynamic'
                        },
                        arrows: {
                            to: {
                                enabled: true,
                                scaleFactor: 0.5
                            }
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

        // Create new network with the appropriate options
        const data = { nodes: nodes, edges: edges };
        network = new vis.Network(container, data, options);

        // Re-attach event handlers
        network.on('click', function(params) {
            if (params.nodes.length > 0) {
                const nodeId = params.nodes[0];
                const node = nodes.get(nodeId);
                if (node && node.atom) {
                    // Check current atom count and warn if getting large
                    const currentAtomCount = atomSpaceCache.getStats().totalAtoms;
                    if (currentAtomCount > LARGE_ATOM_THRESHOLD) {
                        showWarningDialog(currentAtomCount, function() {
                            atomSpaceCache.fetchIncomingSet(node.atom);
                        });
                    } else {
                        // Start operation tracking for stop button
                        startOperation();
                        // Fetch via cache
                        atomSpaceCache.fetchIncomingSet(node.atom);
                    }
                }
            } else if (params.edges.length > 0) {
                const edgeId = params.edges[0];
                // Edge click - remove parent nodes
                const edge = edges.get(edgeId);
                if (edge) {
                    removeNodeAndParents(edge.from);
                }
            }
        });

        this.setAttribute('data-previous-layout', layoutType);

        // Stabilize and fit
        network.stabilize();
        network.fit();
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
        // Use graph view with atom cache
        initializeGraphViewWithAtomCache();
    } else {
        // Rebuild from cache for hierarchical/network view
        rebuildFromAtomCache();
        network.fit();
    }

    updateStatus('Graph refreshed', 'connected');
}

function updateStatus(message, className) {
    const statusElement = document.getElementById('status');
    statusElement.textContent = message;
    statusElement.className = className || '';
}

// Warning and cancellation functions
function showWarningDialog(atomCount, callback) {
    const dialog = document.getElementById('warningDialog');
    const overlay = document.getElementById('overlay');
    const message = document.getElementById('warningMessage');

    message.textContent = `This operation will process approximately ${atomCount} atoms. This may take some time and could affect performance. Do you want to continue?`;

    dialog.style.display = 'block';
    overlay.style.display = 'block';

    pendingOperation = callback;
}

function cancelLargeOperation() {
    const dialog = document.getElementById('warningDialog');
    const overlay = document.getElementById('overlay');

    dialog.style.display = 'none';
    overlay.style.display = 'none';

    pendingOperation = null;
    updateStatus('Operation cancelled', 'connected');
}

function proceedWithLargeOperation() {
    const dialog = document.getElementById('warningDialog');
    const overlay = document.getElementById('overlay');

    dialog.style.display = 'none';
    overlay.style.display = 'none';

    if (pendingOperation) {
        startOperation();  // This will reset cancellation flag in cache
        pendingOperation();
        pendingOperation = null;
    }
}

function startOperation() {
    operationCancelled = false;
    operationStartTime = Date.now();

    // Reset cancellation flag in cache
    atomSpaceCache.resetCancellation();

    // Show stop button after a delay
    stopButtonTimer = setTimeout(() => {
        if (!operationCancelled) {
            document.getElementById('stopButton').style.display = 'block';
        }
    }, STOP_BUTTON_DELAY);
}

function stopCurrentOperation() {
    operationCancelled = true;

    // Cancel all pending operations in the cache
    atomSpaceCache.cancelAllOperations();

    // Hide stop button
    document.getElementById('stopButton').style.display = 'none';

    // Clear timer if still pending
    if (stopButtonTimer) {
        clearTimeout(stopButtonTimer);
        stopButtonTimer = null;
    }

    // Cancel any pending graph updates in graph-view
    if (typeof pendingGraphUpdate !== 'undefined' && pendingGraphUpdate) {
        clearTimeout(pendingGraphUpdate);
        pendingGraphUpdate = null;
    }
    if (typeof pendingListLinkFetches !== 'undefined') {
        pendingListLinkFetches = 0;
    }

    updateStatus('Processing stopped', 'connected');

    // Ensure the graph remains functional
    if (network) {
        network.stabilize();
    }
}

function endOperation() {
    // Hide stop button
    document.getElementById('stopButton').style.display = 'none';

    // Clear timer if still pending
    if (stopButtonTimer) {
        clearTimeout(stopButtonTimer);
        stopButtonTimer = null;
    }

    operationCancelled = false;
    operationStartTime = null;
}

// Removed handleServerResponse - now handled via cache events

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

// Removed pendingIncomingRequest - now handled by cache

// Calculate the maximum depth of an atom (distance to its deepest leaf)
function calculateMaxDepth(atom, depthCache = new Map()) {
    const atomKey = atomSpaceCache.atomToKey(atom);

    // Check cache
    if (depthCache.has(atomKey)) {
        return depthCache.get(atomKey);
    }

    let maxDepth = 0;

    // For Links, check their outgoing atoms
    if (atom.outgoing && atom.outgoing.length > 0) {
        // Has children, so depth is 1 + max depth of children
        atom.outgoing.forEach(child => {
            if (typeof child === 'object' && child !== null) {
                const childDepth = calculateMaxDepth(child, depthCache);
                maxDepth = Math.max(maxDepth, childDepth + 1);
            }
        });
    }
    // For Nodes or empty Links, depth is 0 (they are leaves)

    depthCache.set(atomKey, maxDepth);
    return maxDepth;
}

// Rebuild visualization from atom cache for hierarchical/network view
function rebuildFromAtomCache() {
    // Check if operation was cancelled
    if (operationCancelled) {
        return;
    }

    // Clear existing
    nodes.clear();
    edges.clear();
    atomNodeMap.clear();
    nodeIdCounter = 1;

    // Calculate depths for all atoms
    const depthCache = new Map();
    const allAtoms = atomSpaceCache.getAllAtoms();
    let maxDepthInGraph = 0;

    // First pass: calculate max depth for each atom
    allAtoms.forEach(atom => {
        const depth = calculateMaxDepth(atom, depthCache);
        maxDepthInGraph = Math.max(maxDepthInGraph, depth);
    });

    // Second pass: add all atoms with inverted levels (bottom-up)
    allAtoms.forEach(atom => {
        const atomKey = atomSpaceCache.atomToKey(atom);
        if (atomNodeMap.has(atomKey)) {
            return; // Already added
        }

        const maxDepth = depthCache.get(atomKey);
        // Invert the level: leaves at bottom (high level number), roots at top (level 0)
        const level = maxDepthInGraph - maxDepth;

        // Create node
        const nodeId = nodeIdCounter++;
        const nodeLabel = createCompactLabel(atom);
        const nodeColor = getNodeColor(atom.type);

        nodes.add({
            id: nodeId,
            label: nodeLabel,
            color: nodeColor,
            atom: atom,
            level: level,
            title: atomToSExpression(atom)
        });

        atomNodeMap.set(atomKey, nodeId);
    });

    // Third pass: add edges for parent-child relationships
    allAtoms.forEach(atom => {
        const atomKey = atomSpaceCache.atomToKey(atom);
        const parentNodeId = atomNodeMap.get(atomKey);

        if (parentNodeId && atom.outgoing && atom.outgoing.length > 0) {
            atom.outgoing.forEach(child => {
                if (typeof child === 'object' && child !== null) {
                    const childKey = atomSpaceCache.atomToKey(child);
                    const childNodeId = atomNodeMap.get(childKey);

                    if (childNodeId) {
                        edges.add({
                            from: parentNodeId,
                            to: childNodeId,
                            arrows: {
                                to: {
                                    enabled: true,
                                    scaleFactor: 0.5
                                }
                            }
                        });
                    }
                }
            });
        }
    });
}

// Removed fetchIncomingSet - now handled by atomspace-cache
// Removed initializeGraphViewWithAtomCache - moved to graph-view.js
