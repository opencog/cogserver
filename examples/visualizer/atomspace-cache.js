// AtomSpace Cache - Central storage for all atoms being visualized
// This maintains a single source of truth for all layout modes
// Handles all WebSocket communications with the CogServer

class AtomSpaceCache extends EventTarget {
    constructor() {
        super();
        // Map of atom key to atom object
        this.atoms = new Map();
        // Map of atom key to parent atom keys (for hierarchy tracking)
        this.parents = new Map();
        // Map of atom key to child atom keys
        this.children = new Map();
        // Set of root atom keys (atoms with no parents)
        this.roots = new Set();

        // WebSocket connection
        this.socket = null;
        this.serverUrl = null;

        // Queue for sequential processing of ListLink requests
        this.pendingListLinkRequests = [];
        this.isProcessingListLink = false;
    }

    // Generate unique key for an atom
    atomToKey(atom) {
        if (!atom) return null;
        if (atom.handle) {
            return `handle:${atom.handle}`;
        }
        if (atom.name !== undefined) {
            return `${atom.type}:${atom.name}`;
        }
        if (atom.outgoing) {
            const outgoingKeys = atom.outgoing.map(o => this.atomToKey(o)).join(',');
            return `${atom.type}:[${outgoingKeys}]`;
        }
        return `${atom.type}:${JSON.stringify(atom)}`;
    }

    // Add an atom to the cache
    addAtom(atom, parentAtom = null) {
        if (!atom) return null;

        const atomKey = this.atomToKey(atom);

        // Store the atom
        if (!this.atoms.has(atomKey)) {
            this.atoms.set(atomKey, atom);
            this.parents.set(atomKey, new Set());
            this.children.set(atomKey, new Set());
        }

        // Update parent-child relationships
        if (parentAtom) {
            const parentKey = this.atomToKey(parentAtom);
            if (parentKey) {
                // Add parent relationship
                this.parents.get(atomKey).add(parentKey);
                // Add child relationship
                if (!this.children.has(parentKey)) {
                    this.children.set(parentKey, new Set());
                }
                this.children.get(parentKey).add(atomKey);
                // Remove from roots if it has a parent
                this.roots.delete(atomKey);
            }
        } else {
            // Add to roots if no parent
            if (this.parents.get(atomKey).size === 0) {
                this.roots.add(atomKey);
            }
        }

        // Process atom's outgoing links as children
        if (atom.outgoing && atom.outgoing.length > 0) {
            atom.outgoing.forEach(child => {
                if (typeof child === 'object' && child !== null) {
                    this.addAtom(child, atom);
                }
            });
        }

        return atomKey;
    }

    // Remove an atom and optionally its descendants
    removeAtom(atom, removeDescendants = false) {
        const atomKey = this.atomToKey(atom);
        if (!this.atoms.has(atomKey)) return;

        // If removing descendants, remove all children first
        if (removeDescendants) {
            const children = Array.from(this.children.get(atomKey) || []);
            children.forEach(childKey => {
                const childAtom = this.atoms.get(childKey);
                if (childAtom) {
                    this.removeAtom(childAtom, true);
                }
            });
        }

        // Remove from parent's children
        const parents = this.parents.get(atomKey);
        if (parents) {
            parents.forEach(parentKey => {
                const parentChildren = this.children.get(parentKey);
                if (parentChildren) {
                    parentChildren.delete(atomKey);
                }
            });
        }

        // Remove from children's parents
        const children = this.children.get(atomKey);
        if (children) {
            children.forEach(childKey => {
                const childParents = this.parents.get(childKey);
                if (childParents) {
                    childParents.delete(atomKey);
                    // If child has no more parents, make it a root
                    if (childParents.size === 0 && this.atoms.has(childKey)) {
                        this.roots.add(childKey);
                    }
                }
            });
        }

        // Remove the atom
        this.atoms.delete(atomKey);
        this.parents.delete(atomKey);
        this.children.delete(atomKey);
        this.roots.delete(atomKey);
    }

    // Get all atoms
    getAllAtoms() {
        return Array.from(this.atoms.values());
    }

    // Get root atoms
    getRootAtoms() {
        return Array.from(this.roots).map(key => this.atoms.get(key)).filter(a => a);
    }

    // Get atom by key
    getAtom(atomKey) {
        return this.atoms.get(atomKey);
    }

    // Check if atom exists
    hasAtom(atom) {
        const atomKey = this.atomToKey(atom);
        return this.atoms.has(atomKey);
    }

    // Get parents of an atom
    getParents(atom) {
        const atomKey = this.atomToKey(atom);
        const parentKeys = this.parents.get(atomKey);
        if (!parentKeys) return [];
        return Array.from(parentKeys).map(key => this.atoms.get(key)).filter(a => a);
    }

    // Get children of an atom
    getChildren(atom) {
        const atomKey = this.atomToKey(atom);
        const childKeys = this.children.get(atomKey);
        if (!childKeys) return [];
        return Array.from(childKeys).map(key => this.atoms.get(key)).filter(a => a);
    }

    // Clear all atoms
    clear() {
        this.atoms.clear();
        this.parents.clear();
        this.children.clear();
        this.roots.clear();
    }

    // Get atoms for hierarchical view (exclude EdgeLink internals)
    getAtomsForHierarchicalView() {
        const result = [];
        const processed = new Set();

        // Start with root atoms
        this.getRootAtoms().forEach(atom => {
            // Skip EdgeLinks and EvaluationLinks at root level
            if (atom.type !== 'EdgeLink' && atom.type !== 'EvaluationLink') {
                result.push({
                    atom: atom,
                    depth: 0,
                    parent: null
                });
                processed.add(this.atomToKey(atom));
            }
        });

        // Process all atoms, but skip EdgeLink components
        this.atoms.forEach((atom, atomKey) => {
            if (processed.has(atomKey)) return;

            // Skip EdgeLinks, EvaluationLinks and their typical components
            if (atom.type === 'EdgeLink' ||
                atom.type === 'EvaluationLink' ||
                atom.type === 'BondNode' ||
                atom.type === 'PredicateNode') {
                return;
            }

            // Skip ListLinks that are part of EdgeLink patterns
            if (atom.type === 'ListLink' && atom.outgoing && atom.outgoing.length === 2) {
                // Check if both children are nodes
                const bothNodes = atom.outgoing.every(child =>
                    child && typeof child === 'object' && child.type && child.type.endsWith('Node')
                );
                if (bothNodes) return;
            }

            // Add other atoms
            result.push({
                atom: atom,
                depth: 1,
                parent: null
            });
        });

        return result;
    }

    // Get atoms for graph view (only nodes and EdgeLink relationships)
    getAtomsForGraphView() {
        const nodes = [];
        const edges = [];

        this.atoms.forEach(atom => {
            // Check if this is an EdgeLink/EvaluationLink pattern
            if ((atom.type === 'EdgeLink' || atom.type === 'EvaluationLink') &&
                atom.outgoing && atom.outgoing.length === 2) {

                const predicate = atom.outgoing[0];
                const list = atom.outgoing[1];

                // Validate the pattern
                if (predicate && (predicate.type === 'PredicateNode' || predicate.type === 'BondNode') &&
                    list && list.type === 'ListLink' &&
                    list.outgoing && list.outgoing.length === 2) {

                    const fromNode = list.outgoing[0];
                    const toNode = list.outgoing[1];

                    if (fromNode && fromNode.type && fromNode.type.endsWith('Node') &&
                        toNode && toNode.type && toNode.type.endsWith('Node')) {

                        // Add edge info
                        edges.push({
                            from: fromNode,
                            to: toNode,
                            label: predicate.name || 'edge',
                            type: atom.type
                        });

                        // Add nodes if not already present
                        if (!nodes.some(n => this.atomToKey(n) === this.atomToKey(fromNode))) {
                            nodes.push(fromNode);
                        }
                        if (!nodes.some(n => this.atomToKey(n) === this.atomToKey(toNode))) {
                            nodes.push(toNode);
                        }
                    }
                }
            } else if (atom.type && atom.type.endsWith('Node') &&
                       atom.type !== 'BondNode' && atom.type !== 'PredicateNode') {
                // Add standalone nodes
                if (!nodes.some(n => this.atomToKey(n) === this.atomToKey(atom))) {
                    nodes.push(atom);
                }
            }
        });

        return { nodes, edges };
    }

    // Get statistics
    getStats() {
        return {
            totalAtoms: this.atoms.size,
            rootAtoms: this.roots.size,
            atomTypes: this.getAtomTypeDistribution()
        };
    }

    // Get distribution of atom types
    getAtomTypeDistribution() {
        const distribution = {};
        this.atoms.forEach(atom => {
            distribution[atom.type] = (distribution[atom.type] || 0) + 1;
        });
        return distribution;
    }

    // ============= WebSocket Communication Methods =============

    // Connect to CogServer
    connect(serverUrl) {
        if (this.socket && this.socket.readyState === WebSocket.OPEN) {
            this.socket.close();
        }

        this.serverUrl = serverUrl;
        this.socket = new WebSocket(serverUrl);

        this.socket.onopen = () => {
            console.log('Connected to CogServer');
            this.dispatchEvent(new CustomEvent('connection', {
                detail: { status: 'connected', message: 'Connected to server' }
            }));
        };

        this.socket.onclose = () => {
            console.log('Disconnected from CogServer');
            this.dispatchEvent(new CustomEvent('connection', {
                detail: { status: 'disconnected', message: 'Disconnected from server' }
            }));
        };

        this.socket.onerror = (error) => {
            console.error('WebSocket error:', error);
            this.dispatchEvent(new CustomEvent('connection', {
                detail: { status: 'error', message: 'Connection error' }
            }));
        };

        this.socket.onmessage = (event) => {
            this.handleWebSocketMessage(event);
        };
    }

    // Handle incoming WebSocket messages
    handleWebSocketMessage(event) {
        try {
            const rawResponse = JSON.parse(event.data);
            let response;

            // Handle wrapped response format
            if (rawResponse.hasOwnProperty('success')) {
                if (rawResponse.success && rawResponse.result) {
                    response = rawResponse.result;
                }
            } else {
                response = rawResponse;
            }

            // Process the response based on what we're waiting for
            if (this.isProcessingListLink && response && Array.isArray(response)) {
                this.processListLinkResponse(response);
            }
        } catch (error) {
            console.error('Error parsing WebSocket message:', error);
        }
    }

    // Fetch incoming set for an atom
    fetchIncomingSet(atom) {
        if (!this.socket || this.socket.readyState !== WebSocket.OPEN) {
            this.dispatchEvent(new CustomEvent('error', {
                detail: { message: 'Not connected to server' }
            }));
            return;
        }

        // Construct the command
        let atomSpec;
        if (atom.name !== undefined) {
            const escapedName = JSON.stringify(atom.name);
            atomSpec = `{"type": "${atom.type}", "name": ${escapedName}}`;
        } else {
            atomSpec = JSON.stringify(atom);
        }

        const command = `AtomSpace.getIncoming(${atomSpec})`;
        console.log('Getting incoming set:', command);

        // For regular atoms, send immediately
        if (atom.type !== 'ListLink') {
            this.socket.send(command);
            return;
        }

        // For ListLinks, queue for sequential processing
        this.pendingListLinkRequests.push({
            atom: atom,
            command: command
        });

        if (!this.isProcessingListLink) {
            this.processNextListLink();
        }
    }

    // Process ListLink requests sequentially
    processNextListLink() {
        if (this.pendingListLinkRequests.length === 0) {
            this.isProcessingListLink = false;
            this.dispatchEvent(new CustomEvent('update', {
                detail: { type: 'listlinks-complete' }
            }));
            return;
        }

        this.isProcessingListLink = true;
        const request = this.pendingListLinkRequests.shift();
        this.currentListLinkRequest = request;
        this.socket.send(request.command);
    }

    // Process ListLink response
    processListLinkResponse(response) {
        if (!this.currentListLinkRequest) return;

        const parentAtom = this.currentListLinkRequest.atom;

        // Add all atoms from the response
        response.forEach(atom => {
            if (atom && typeof atom === 'object') {
                this.addAtom(atom, parentAtom);
            }
        });

        // Notify that cache has been updated
        this.dispatchEvent(new CustomEvent('update', {
            detail: {
                type: 'incoming-set',
                parent: parentAtom,
                atoms: response
            }
        }));

        // Process next ListLink
        this.currentListLinkRequest = null;
        setTimeout(() => this.processNextListLink(), 10);
    }

    // Remove an atom and its incoming edges
    removeIncomingEdges(atom) {
        const atomKey = this.atomToKey(atom);

        // Find all atoms that have this atom as a child
        const parents = this.getParents(atom);
        parents.forEach(parent => {
            const parentKey = this.atomToKey(parent);
            const children = this.children.get(parentKey);
            if (children) {
                children.delete(atomKey);
            }
        });

        // Clear this atom's parents
        this.parents.set(atomKey, new Set());

        // If atom has no parents, it becomes a root
        if (this.atoms.has(atomKey)) {
            this.roots.add(atomKey);
        }

        // Notify update
        this.dispatchEvent(new CustomEvent('update', {
            detail: {
                type: 'edges-removed',
                atom: atom
            }
        }));
    }

    // Disconnect from server
    disconnect() {
        if (this.socket) {
            this.socket.close();
            this.socket = null;
        }
    }
}

// Create global instance
const atomSpaceCache = new AtomSpaceCache();