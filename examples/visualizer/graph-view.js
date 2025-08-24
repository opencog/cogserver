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

// Batching mechanism for graph updates
let pendingGraphUpdate = null;
let pendingListLinkFetches = 0;

// Handle cache updates for graph view - performs double fetch for ListLinks
function handleGraphViewCacheUpdate(parent, atoms) {
    // Check if operation was cancelled (from tree-view.js)
    if (typeof operationCancelled !== 'undefined' && operationCancelled) {
        pendingListLinkFetches = 0;
        if (pendingGraphUpdate) {
            clearTimeout(pendingGraphUpdate);
            pendingGraphUpdate = null;
        }
        return;
    }

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

    // Track how many ListLink fetches we're about to start
    if (listLinksToFetch.length > 0) {
        pendingListLinkFetches += listLinksToFetch.length;

        // Fetch incoming sets for qualifying ListLinks (second-level fetch)
        listLinksToFetch.forEach(listLink => {
            atomSpaceCache.fetchIncomingSet(listLink);
        });
    }

    // If this was a ListLink response, decrement the counter
    if (parent && parent.type === 'ListLink') {
        pendingListLinkFetches = Math.max(0, pendingListLinkFetches - 1);
    }

    // Clear any existing update timer
    if (pendingGraphUpdate) {
        clearTimeout(pendingGraphUpdate);
        pendingGraphUpdate = null;
    }

    // If we're waiting for ListLink fetches, defer the update
    if (pendingListLinkFetches > 0) {
        // Set a timeout as a fallback in case some fetches fail
        pendingGraphUpdate = setTimeout(() => {
            // Check again for cancellation before rebuilding
            if (typeof operationCancelled === 'undefined' || !operationCancelled) {
                pendingListLinkFetches = 0;  // Reset counter
                initializeGraphViewWithAtomCache();
            }
            // Always end operation when timeout completes
            if (typeof endOperation === 'function') {
                endOperation();
            }
            pendingGraphUpdate = null;
        }, 1000);  // 1 second timeout
    } else {
        // No pending fetches, update immediately
        if (typeof operationCancelled === 'undefined' || !operationCancelled) {
            initializeGraphViewWithAtomCache();
        }
        // End operation only if there are no more pending operations
        // This handles the case where the initial atom had no ListLinks
        if (typeof endOperation === 'function' && !atomSpaceCache.hasPendingOperations()) {
            endOperation();
        }
    }
}

// NOTE: Old atom processing functions removed - now handled by atomspace-cache

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
