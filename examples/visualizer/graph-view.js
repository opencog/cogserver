// Graph View - Special handling for Edge and Evaluation links as labeled edges

// Check if an atom is an Edge or Evaluation link with the special pattern
function isGraphEdgePattern(atom) {
    if (!atom || !atom.outgoing || atom.outgoing.length !== 2) {
        return false;
    }

    // Must be EdgeLink or EvaluationLink
    if (atom.type !== 'EdgeLink' && atom.type !== 'EvaluationLink') {
        return false;
    }

    // First outgoing must be a PredicateNode or BondNode
    const predicate = atom.outgoing[0];
    if (typeof predicate !== 'object' || (predicate.type !== 'PredicateNode' && predicate.type !== 'BondNode')) {
        return false;
    }

    // Second outgoing must be a List with exactly 2 nodes
    const list = atom.outgoing[1];
    if (typeof list !== 'object' || list.type !== 'ListLink') {
        return false;
    }

    if (!list.outgoing || list.outgoing.length !== 2) {
        return false;
    }

    // Both items in the list should be nodes (any type ending in "Node")
    const from = list.outgoing[0];
    const to = list.outgoing[1];

    if (typeof from !== 'object' || !from.type.endsWith('Node')) {
        return false;
    }
    if (typeof to !== 'object' || !to.type.endsWith('Node')) {
        return false;
    }

    return true;
}

// Extract edge information from the special pattern
function extractGraphEdgeInfo(atom) {
    const predicate = atom.outgoing[0];
    const list = atom.outgoing[1];
    const fromNode = list.outgoing[0];
    const toNode = list.outgoing[1];

    return {
        edgeLabel: predicate.name || 'edge',
        fromNode: fromNode,
        toNode: toNode,
        edgeType: atom.type // EdgeLink or EvaluationLink
    };
}

// Add atoms to graph with special handling for edge patterns
function addAtomToGraphView(atom, parentId, depth, order = 0) {
    // Check if this is a special edge pattern
    if (isGraphEdgePattern(atom)) {
        const edgeInfo = extractGraphEdgeInfo(atom);

        // Add the from and to nodes if they don't exist
        const fromNodeId = addNodeToGraph(edgeInfo.fromNode);
        const toNodeId = addNodeToGraph(edgeInfo.toNode);

        // Add a labeled edge between them
        addLabeledEdge(fromNodeId, toNodeId, edgeInfo.edgeLabel, edgeInfo.edgeType);

        // Don't add the Edge/EvaluationLink itself as a node
        return null;
    }

    // For non-edge patterns, use regular graph addition
    return addAtomToGraph(atom, parentId, depth, order);
}

// Add a single node to the graph (reusing existing if already present)
function addNodeToGraph(atom) {
    const atomKey = atomToKey(atom);

    // Check if node already exists
    if (atomNodeMap.has(atomKey)) {
        return atomNodeMap.get(atomKey);
    }

    // Create new node
    const nodeId = nodeIdCounter++;
    const nodeLabel = createCompactLabel(atom);
    const nodeColor = getNodeColor(atom.type);

    nodes.add({
        id: nodeId,
        label: nodeLabel,
        color: nodeColor,
        atom: atom,
        title: atomToSExpression(atom),
        shape: 'ellipse' // Use ellipse for graph mode nodes
    });

    atomNodeMap.set(atomKey, nodeId);
    return nodeId;
}

// Add a labeled edge between two nodes
function addLabeledEdge(fromId, toId, label, edgeType) {
    // Every Atom is unique in AtomSpace - always add the edge
    // Different style for graph edges
    const edgeColor = edgeType === 'EdgeLink' ? '#FF6B6B' : '#4ECDC4';

    // Count existing edges between these nodes to offset curves
    const existingEdges = edges.get({
        filter: function(item) {
            return (item.from === fromId && item.to === toId) ||
                   (item.from === toId && item.to === fromId);
        }
    });
    const curveOffset = existingEdges.length;

    edges.add({
        from: fromId,
        to: toId,
        label: label,
        arrows: {
            to: {
                enabled: true,
                scaleFactor: 0.7
            }
        },
        color: {
            color: edgeColor,
            highlight: '#FFD93D',
            hover: '#FFD93D'
        },
        font: {
            color: '#333333',
            size: 12,
            background: 'rgba(255, 255, 255, 0.8)',
            strokeWidth: 0,
            align: 'middle'
        },
        smooth: {
            enabled: true,
            type: curveOffset === 0 ? 'dynamic' : 'curvedCW',
            roundness: 0.2 + (curveOffset * 0.15) // Increase curve for each additional edge
        },
        width: 2,
        dashes: edgeType === 'EvaluationLink' ? [5, 5] : false
    });
}

// Initialize graph view using atom cache
function initializeGraphViewWithAtomCache() {
    // Clear existing nodes and edges
    nodes.clear();
    edges.clear();
    atomNodeMap.clear();
    nodeIdCounter = 1;

    // Get atoms from cache for graph view
    const graphData = atomSpaceCache.getAtomsForGraphView();

    // Add all nodes first
    graphData.nodes.forEach(atom => {
        const atomKey = atomSpaceCache.atomToKey(atom);
        if (!atomNodeMap.has(atomKey)) {
            const nodeId = nodeIdCounter++;
            const nodeLabel = createCompactLabel(atom);
            const nodeColor = getNodeColor(atom.type);

            nodes.add({
                id: nodeId,
                label: nodeLabel,
                color: nodeColor,
                atom: atom,
                title: atomToSExpression(atom)
            });

            atomNodeMap.set(atomKey, nodeId);
        }
    });

    // Add all edges
    graphData.edges.forEach(edge => {
        const fromKey = atomSpaceCache.atomToKey(edge.from);
        const toKey = atomSpaceCache.atomToKey(edge.to);
        const fromId = atomNodeMap.get(fromKey);
        const toId = atomNodeMap.get(toKey);

        if (fromId && toId) {
            edges.add({
                from: fromId,
                to: toId,
                label: edge.label,
                arrows: {
                    to: {
                        enabled: true,
                        scaleFactor: 0.5
                    }
                },
                font: {
                    size: 10,
                    align: 'middle'
                }
            });
        }
    });
}

// Initialize graph view mode (legacy - for compatibility)
function initializeGraphView() {
    // Clear any existing data
    if (nodes) nodes.clear();
    if (edges) edges.clear();
    atomNodeMap.clear();
    nodeIdCounter = 1;

    // Process root atoms with graph view logic
    if (rootAtoms && rootAtoms.length > 0) {
        rootAtoms.forEach(atom => {
            processAtomForGraphView(atom);
        });

        if (network) {
            network.fit();
        }
        updateStatus(`Graph view initialized with ${rootAtoms.length} atom(s)`, 'connected');
    }
}

// Initialize graph view with all stored atoms - redirects to cache-based version
function initializeGraphViewWithAllAtoms() {
    initializeGraphViewWithAtomCache();
}

// Handle cache updates for graph view - performs double fetch for ListLinks
function handleGraphViewCacheUpdate(parent, atoms) {
    // Check if we need to fetch incoming sets for ListLinks
    const listLinksToFetch = [];

    atoms.forEach(atom => {
        // Check if it's a ListLink with exactly 2 nodes
        if (atom.type === 'ListLink' && atom.outgoing && atom.outgoing.length === 2) {
            // Check if both outgoing atoms are nodes
            const firstIsNode = atom.outgoing[0] && typeof atom.outgoing[0] === 'object' &&
                              atom.outgoing[0].type && atom.outgoing[0].type.endsWith('Node');
            const secondIsNode = atom.outgoing[1] && typeof atom.outgoing[1] === 'object' &&
                               atom.outgoing[1].type && atom.outgoing[1].type.endsWith('Node');

            if (firstIsNode && secondIsNode) {
                listLinksToFetch.push(atom);
            }
        }
    });

    // Fetch incoming sets for qualifying ListLinks (second-level fetch)
    if (listLinksToFetch.length > 0) {
        listLinksToFetch.forEach(listLink => {
            atomSpaceCache.fetchIncomingSet(listLink);
        });
    }

    // Rebuild the graph to show current state
    initializeGraphViewWithAtomCache();
}

// Check if this is a ListLink that belongs to an Edge/EvaluationLink pattern
function isListLinkInEdgePattern(atom, parent) {
    if (!atom || atom.type !== 'ListLink') {
        return false;
    }
    // Check if parent is an Edge/EvaluationLink with this ListLink as second argument
    if (parent && (parent.type === 'EdgeLink' || parent.type === 'EvaluationLink')) {
        if (parent.outgoing && parent.outgoing.length === 2 && parent.outgoing[1] === atom) {
            // Check if first argument is a PredicateNode or BondNode
            const predicate = parent.outgoing[0];
            if (predicate && typeof predicate === 'object' &&
                (predicate.type === 'PredicateNode' || predicate.type === 'BondNode')) {
                return true;
            }
        }
    }

    return false;
}

// Check if this is a PredicateNode/BondNode that belongs to an Edge/EvaluationLink pattern
function isPredicateInEdgePattern(atom, parent) {
    if (!atom || (atom.type !== 'PredicateNode' && atom.type !== 'BondNode')) {
        return false;
    }
    // Check if parent is an Edge/EvaluationLink with this PredicateNode/BondNode as first argument
    if (parent && (parent.type === 'EdgeLink' || parent.type === 'EvaluationLink')) {
        if (parent.outgoing && parent.outgoing.length === 2 && parent.outgoing[0] === atom) {
            // Check if second argument is a ListLink with 2 nodes
            const list = parent.outgoing[1];
            if (list && typeof list === 'object' && list.type === 'ListLink') {
                if (list.outgoing && list.outgoing.length === 2) {
                    const first = list.outgoing[0];
                    const second = list.outgoing[1];
                    if (first && typeof first === 'object' && first.type.endsWith('Node') &&
                        second && typeof second === 'object' && second.type.endsWith('Node')) {
                        return true;
                    }
                }
            }
        }
    }

    return false;
}

// Check if an atom should be skipped in graph view
function shouldSkipInGraphView(atom) {
    if (!atom || typeof atom !== 'object') {
        return true;
    }

    // Skip ListLinks that could be part of Edge/EvaluationLink patterns
    if (atom.type === 'ListLink' && atom.outgoing && atom.outgoing.length === 2) {
        const first = atom.outgoing[0];
        const second = atom.outgoing[1];
        // If it has exactly 2 nodes, it might be part of a pattern - skip it
        if (first && typeof first === 'object' && first.type.endsWith('Node') &&
            second && typeof second === 'object' && second.type.endsWith('Node')) {
            return true;
        }
    }

    // Skip PredicateNode and BondNode as they're used as labels in EdgeLinks
    if (atom.type === 'PredicateNode' || atom.type === 'BondNode') {
        return true;
    }

    return false;
}

// Process an atom and its children for graph view
function processAtomForGraphView(atom, visited = new Set()) {
    if (!atom || typeof atom !== 'object') {
        return;
    }

    // Check if this is an Edge/EvaluationLink with the special pattern FIRST
    // Don't use visited set for EdgeLinks - process them all
    if (isGraphEdgePattern(atom)) {
        const edgeInfo = extractGraphEdgeInfo(atom);

        // Add only the two endpoint nodes (or reuse if they exist)
        const fromNodeId = addNodeToGraph(edgeInfo.fromNode);
        const toNodeId = addNodeToGraph(edgeInfo.toNode);

        // Add labeled edge between them
        addLabeledEdge(fromNodeId, toNodeId, edgeInfo.edgeLabel, edgeInfo.edgeType);

        // Don't process this atom's children as regular nodes
        return;
    }

    // For non-EdgeLink atoms, use the visited set normally
    const atomKey = atomToKey(atom);

    if (visited.has(atomKey)) {
        return;
    }

    // Skip atoms that shouldn't be shown in graph view
    if (shouldSkipInGraphView(atom)) {
        // Don't mark certain atoms as visited as they can be shared by multiple EdgeLinks
        if ((atom.type === 'ListLink' && atom.outgoing && atom.outgoing.length === 2) ||
            atom.type === 'PredicateNode' ||
            atom.type === 'BondNode') {
            // These can be shared by multiple EdgeLinks - don't mark as visited
        } else {
            // Mark other skipped atoms as visited
            visited.add(atomKey);
        }
        // Still process children to find patterns
        if (atom.outgoing && atom.outgoing.length > 0) {
            atom.outgoing.forEach(outgoing => {
                if (typeof outgoing === 'object' && outgoing !== null) {
                    processAtomForGraphView(outgoing, visited);
                }
            });
        }
        return;
    }

    // Mark as visited for regular atoms
    visited.add(atomKey);

    // Regular atom - add as node and process children
    const nodeId = addNodeToGraph(atom);

    // Process outgoing links
    if (atom.outgoing && atom.outgoing.length > 0) {
        atom.outgoing.forEach(outgoing => {
            if (typeof outgoing === 'object' && outgoing !== null) {
                // Process the child
                processAtomForGraphView(outgoing, visited);

                // Add edge only if the child was actually added as a node
                if (!isGraphEdgePattern(outgoing) && !shouldSkipInGraphView(outgoing)) {
                    const outgoingKey = atomToKey(outgoing);
                    if (atomNodeMap.has(outgoingKey)) {
                        const childId = atomNodeMap.get(outgoingKey);
                        addEdgeIfNotExists(nodeId, childId);
                    }
                }
            }
        });
    }
}

// Fetch incoming set for graph view with special handling for ListLinks
function fetchIncomingSetForGraph(atom, nodeId) {
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

    // Store request info for when we receive the response
    pendingIncomingRequest = {
        atom: atom,
        nodeId: nodeId,
        isGraphView: true  // Flag to indicate graph view processing
    };

    // Send the command
    socket.send(command);
}

// Process incoming set response for graph view
function processIncomingSetForGraph(incomingAtoms, targetNodeId) {
    if (!incomingAtoms || !Array.isArray(incomingAtoms)) {
        updateStatus('No incoming links found', 'connected');
        return;
    }

    // Track ListLinks that need their incoming set fetched
    const listLinksToFetch = [];

    // Use a shared visited set for all processing
    const visited = new Set();

    // First pass: add all incoming atoms and identify ListLinks with 2 nodes
    incomingAtoms.forEach(atom => {
        if (atom && typeof atom === 'object') {
            // Check if it's a ListLink with exactly 2 nodes
            if (atom.type === 'ListLink' && atom.outgoing && atom.outgoing.length === 2) {
                // Check if both outgoing atoms are nodes
                const firstIsNode = atom.outgoing[0] && typeof atom.outgoing[0] === 'object' &&
                                  atom.outgoing[0].type && atom.outgoing[0].type.endsWith('Node');
                const secondIsNode = atom.outgoing[1] && typeof atom.outgoing[1] === 'object' &&
                                   atom.outgoing[1].type && atom.outgoing[1].type.endsWith('Node');

                if (firstIsNode && secondIsNode) {
                    listLinksToFetch.push(atom);
                }
            }

            // Process this atom (might be an Edge/EvaluationLink pattern)
            processAtomForGraphView(atom, visited);
        }
    });

    // Now fetch incoming sets for qualifying ListLinks
    if (listLinksToFetch.length > 0) {
        fetchListLinkIncomingSets(listLinksToFetch, visited);
    }

    // Refresh the graph
    if (network) {
        network.fit();
    }

    updateStatus(`Added ${incomingAtoms.length} incoming link(s)`, 'connected');
}

// NOTE: WebSocket communication functions removed - now handled by atomspace-cache

// Get graph view layout options
function getGraphViewOptions() {
    return {
        nodes: {
            shape: 'ellipse',
            font: {
                size: 14,
                face: 'monospace'
            },
            margin: 8,
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
                enabled: true,
                type: 'dynamic'
            },
            font: {
                size: 12,
                align: 'middle'
            }
        },
        physics: {
            enabled: true,
            solver: 'forceAtlas2Based',
            forceAtlas2Based: {
                gravitationalConstant: -50,
                centralGravity: 0.01,
                springLength: 150,
                springConstant: 0.08,
                damping: 0.4,
                avoidOverlap: 0.5
            },
            stabilization: {
                iterations: 150,
                updateInterval: 25
            }
        },
        layout: {
            randomSeed: 2,
            improvedLayout: true
        },
        interaction: {
            hover: true,
            tooltipDelay: 200
        }
    };
}