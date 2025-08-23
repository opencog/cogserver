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

    // First outgoing must be a Predicate
    const predicate = atom.outgoing[0];
    if (typeof predicate !== 'object' || predicate.type !== 'PredicateNode') {
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
    // Check if edge already exists
    const existingEdges = edges.get({
        filter: function(edge) {
            return edge.from === fromId && edge.to === toId && edge.label === label;
        }
    });

    if (existingEdges.length === 0) {
        // Different style for graph edges
        const edgeColor = edgeType === 'EdgeLink' ? '#FF6B6B' : '#4ECDC4';

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
                type: 'curvedCW',
                roundness: 0.2
            },
            width: 2,
            dashes: edgeType === 'EvaluationLink' ? [5, 5] : false
        });
    }
}

// Initialize graph view mode
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

// Check if this is a ListLink that belongs to an Edge/EvaluationLink pattern
function isListLinkInEdgePattern(atom, parent) {
    if (!atom || atom.type !== 'ListLink') {
        return false;
    }
    // Check if parent is an Edge/EvaluationLink with this ListLink as second argument
    if (parent && (parent.type === 'EdgeLink' || parent.type === 'EvaluationLink')) {
        if (parent.outgoing && parent.outgoing.length === 2 && parent.outgoing[1] === atom) {
            // Check if first argument is a PredicateNode
            const predicate = parent.outgoing[0];
            if (predicate && typeof predicate === 'object' && predicate.type === 'PredicateNode') {
                return true;
            }
        }
    }

    return false;
}

// Process an atom and its children for graph view
function processAtomForGraphView(atom, visited = new Set(), parent = null) {
    const atomKey = atomToKey(atom);

    if (visited.has(atomKey)) {
        return;
    }
    visited.add(atomKey);

    // Handle special edge patterns
    if (isGraphEdgePattern(atom)) {
        const edgeInfo = extractGraphEdgeInfo(atom);

        // Add nodes
        const fromNodeId = addNodeToGraph(edgeInfo.fromNode);
        const toNodeId = addNodeToGraph(edgeInfo.toNode);

        // Add labeled edge
        addLabeledEdge(fromNodeId, toNodeId, edgeInfo.edgeLabel, edgeInfo.edgeType);

        // Mark the ListLink and PredicateNode as visited so they won't be processed separately
        const predicate = atom.outgoing[0];
        const list = atom.outgoing[1];
        if (predicate && typeof predicate === 'object') {
            const predicateKey = atomToKey(predicate);
            visited.add(predicateKey);
        }
        if (list && typeof list === 'object') {
            const listKey = atomToKey(list);
            visited.add(listKey);
        }

        // Don't recurse into the nodes - they're already added
    } else if (!isListLinkInEdgePattern(atom, parent)) {
        // Regular atom - add as node only if not a ListLink in edge pattern
        const nodeId = addNodeToGraph(atom);

        // Process outgoing links
        if (atom.outgoing && atom.outgoing.length > 0) {
            atom.outgoing.forEach(outgoing => {
                if (typeof outgoing === 'object' && outgoing !== null) {
                    // First process the child
                    processAtomForGraphView(outgoing, visited, atom);

                    // Then add edge only if the child is not a special pattern
                    if (!isGraphEdgePattern(outgoing) && !isListLinkInEdgePattern(outgoing, atom)) {
                        // Check if the outgoing was actually added as a node
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
    console.log('Getting incoming set for atom (graph view):', command);

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
            processAtomForGraphView(atom);
        }
    });

    // Now fetch incoming sets for qualifying ListLinks
    if (listLinksToFetch.length > 0) {
        fetchListLinkIncomingSets(listLinksToFetch);
    }

    // Refresh the graph
    if (network) {
        network.fit();
    }

    updateStatus(`Added ${incomingAtoms.length} incoming link(s)`, 'connected');
}

// Fetch incoming sets for multiple ListLinks
function fetchListLinkIncomingSets(listLinks) {
    if (!socket || socket.readyState !== WebSocket.OPEN) {
        return;
    }

    // Process each ListLink
    listLinks.forEach(listLink => {
        const atomSpec = JSON.stringify(listLink);
        const command = `AtomSpace.getIncoming(${atomSpec})`;
        console.log('Getting incoming set for ListLink:', command);

        // Create a temporary handler for this specific request
        const messageHandler = function(event) {
            try {
                const response = JSON.parse(event.data);
                if (response && Array.isArray(response)) {
                    // Process the incoming atoms for this ListLink
                    response.forEach(atom => {
                        if (atom && typeof atom === 'object') {
                            // Check if it's an Edge/EvaluationLink pattern
                            if (isGraphEdgePattern(atom)) {
                                processAtomForGraphView(atom);
                            }
                        }
                    });

                    // Update the network
                    if (network) {
                        network.fit();
                    }
                }
            } catch (error) {
                console.error('Error processing ListLink incoming set:', error);
            }

            // Remove this handler after processing
            socket.removeEventListener('message', messageHandler);
        };

        // Add the handler and send the command
        socket.addEventListener('message', messageHandler);
        socket.send(command);
    });
}

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