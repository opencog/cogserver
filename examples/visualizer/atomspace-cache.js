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

        // Reverse index: Map of atom key to Set of atom keys that reference it
        // This tracks which Links contain this atom in their outgoing set
        this.referencedBy = new Map();

        // WebSocket connection
        this.socket = null;
        this.serverUrl = null;

        // Queue for sequential processing of ListLink requests
        this.pendingListLinkRequests = [];
        this.isProcessingListLink = false;
        this.operationsCancelled = false;

        // Cache size management
        this.maxCacheSize = 250;  // Default max size
        this.skippedAtoms = false;  // Track if atoms were skipped due to limit
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

    // Set the maximum cache size
    setMaxCacheSize(size) {
        this.maxCacheSize = Math.max(10, size);  // Minimum size of 10
        this.checkCacheWarning();
    }

    // Check if we can add more atoms without exceeding limit
    canAddAtom() {
        return this.atoms.size < this.maxCacheSize;
    }

    // Check if cache is near full (96% or more)
    isCacheNearFull() {
        return this.atoms.size >= (this.maxCacheSize * 0.96);
    }

    // Check and update cache warning status
    checkCacheWarning() {
        const nearFull = this.isCacheNearFull();
        this.dispatchEvent(new CustomEvent('cache-status', {
            detail: {
                size: this.atoms.size,
                maxSize: this.maxCacheSize,
                nearFull: nearFull,
                skippedAtoms: this.skippedAtoms
            }
        }));
    }

    // Add an atom to the cache
    addAtom(atom, parentAtom = null) {
        if (!atom) return null;

        const atomKey = this.atomToKey(atom);
        const isNewAtom = !this.atoms.has(atomKey);

        // Store the atom if not already present and we have space
        if (isNewAtom) {
            // Check cache limit
            if (!this.canAddAtom()) {
                this.skippedAtoms = true;
                return null;  // Can't add due to cache limit
            }
            this.atoms.set(atomKey, atom);
            this.parents.set(atomKey, new Set());
            this.children.set(atomKey, new Set());
            this.referencedBy.set(atomKey, new Set());
            this.checkCacheWarning();  // Update warning status
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

        // Update reverse index for outgoing links
        // This tracks which atoms are referenced by this atom
        if (isNewAtom && atom.outgoing && atom.outgoing.length > 0) {
            atom.outgoing.forEach(child => {
                if (typeof child === 'object' && child !== null) {
                    const childKey = this.atomToKey(child);
                    // Add this atom to the child's referencedBy set
                    if (!this.referencedBy.has(childKey)) {
                        this.referencedBy.set(childKey, new Set());
                    }
                    this.referencedBy.get(childKey).add(atomKey);

                    // Recursively add the child atom
                    this.addAtom(child, null);  // No parent relationship for structural children
                }
            });
        }

        return atomKey;
    }

    // Remove an atom and optionally its descendants
    removeAtom(atom, removeDescendants = false) {
        const atomKey = this.atomToKey(atom);
        if (!this.atoms.has(atomKey)) return 0;

        let removedCount = 0;

        // Use reverse index to find Links that reference this atom - O(1) lookup!
        const linksToRemove = new Set(this.referencedBy.get(atomKey) || []);

        // Recursively remove all dependent Links
        linksToRemove.forEach(linkKey => {
            const linkAtom = this.atoms.get(linkKey);
            if (linkAtom) {
                removedCount += this.removeAtom(linkAtom, false);
            }
        });

        // If removing descendants, remove all children first
        if (removeDescendants) {
            const children = Array.from(this.children.get(atomKey) || []);
            children.forEach(childKey => {
                const childAtom = this.atoms.get(childKey);
                if (childAtom) {
                    removedCount += this.removeAtom(childAtom, true);
                }
            });
        }

        // Only proceed if the atom still exists (might have been removed as a dependent)
        if (!this.atoms.has(atomKey)) {
            return removedCount;
        }

        // Clean up reverse index entries for this atom's outgoing links
        const atomToRemove = this.atoms.get(atomKey);
        if (atomToRemove && atomToRemove.outgoing && atomToRemove.outgoing.length > 0) {
            atomToRemove.outgoing.forEach(child => {
                if (typeof child === 'object' && child !== null) {
                    const childKey = this.atomToKey(child);
                    const referencedBySet = this.referencedBy.get(childKey);
                    if (referencedBySet) {
                        referencedBySet.delete(atomKey);
                        // Clean up empty sets
                        if (referencedBySet.size === 0) {
                            this.referencedBy.delete(childKey);
                        }
                    }
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

        // Remove the atom from all maps
        this.atoms.delete(atomKey);
        this.parents.delete(atomKey);
        this.children.delete(atomKey);
        this.roots.delete(atomKey);
        this.referencedBy.delete(atomKey);
        removedCount++;

        return removedCount;
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
    // Remove an atom and its parent chain from the cache
    removeAtomAndParents(atom) {
        if (!atom) return 0;

        const atomsToRemove = new Set();
        const atomKey = this.atomToKey(atom);

        // Recursive function to collect atom and all its parents
        const collectAtomsToRemove = (currentKey) => {
            if (!currentKey || atomsToRemove.has(currentKey)) {
                return;
            }

            atomsToRemove.add(currentKey);

            // Get all parent atoms of this atom
            const parentKeys = this.parents.get(currentKey);
            if (parentKeys) {
                parentKeys.forEach(parentKey => {
                    collectAtomsToRemove(parentKey);
                });
            }
        };

        // Start collection from the given atom
        collectAtomsToRemove(atomKey);

        // Use reverse index to find dependent Links efficiently
        const dependentLinks = new Set();
        atomsToRemove.forEach(keyToRemove => {
            // Get all Links that reference this atom from the reverse index - O(1)!
            const referencingLinks = this.referencedBy.get(keyToRemove);
            if (referencingLinks) {
                referencingLinks.forEach(linkKey => {
                    // Only add if not already marked for removal
                    if (!atomsToRemove.has(linkKey)) {
                        dependentLinks.add(linkKey);
                    }
                });
            }
        });

        // Add dependent Links to removal set
        dependentLinks.forEach(linkKey => {
            atomsToRemove.add(linkKey);
        });

        // Remove all collected atoms
        atomsToRemove.forEach(key => {
            // Get the atom before removing
            const atomToRemove = this.atoms.get(key);

            // Clean up reverse index for this atom's outgoing links
            if (atomToRemove && atomToRemove.outgoing && atomToRemove.outgoing.length > 0) {
                atomToRemove.outgoing.forEach(child => {
                    if (typeof child === 'object' && child !== null) {
                        const childKey = this.atomToKey(child);
                        const referencedBySet = this.referencedBy.get(childKey);
                        if (referencedBySet) {
                            referencedBySet.delete(key);
                            // Clean up empty sets
                            if (referencedBySet.size === 0) {
                                this.referencedBy.delete(childKey);
                            }
                        }
                    }
                });
            }

            // Remove from atoms map
            this.atoms.delete(key);

            // Clean up parent relationships
            const parents = this.parents.get(key);
            if (parents) {
                parents.forEach(parentKey => {
                    const childrenSet = this.children.get(parentKey);
                    if (childrenSet) {
                        childrenSet.delete(key);
                    }
                });
            }
            this.parents.delete(key);

            // Clean up children relationships
            const children = this.children.get(key);
            if (children) {
                children.forEach(childKey => {
                    const parentsSet = this.parents.get(childKey);
                    if (parentsSet) {
                        parentsSet.delete(key);
                        // If child has no more parents, it becomes a root
                        if (parentsSet.size === 0 && this.atoms.has(childKey)) {
                            this.roots.add(childKey);
                        }
                    }
                });
            }
            this.children.delete(key);

            // Remove from roots if present
            this.roots.delete(key);

            // Remove from referencedBy map
            this.referencedBy.delete(key);
        });

        // Check if we're below the warning threshold after removal
        if (this.skippedAtoms && !this.isCacheNearFull()) {
            this.skippedAtoms = false;
        }

        // Notify that cache has been updated
        this.dispatchEvent(new CustomEvent('update', {
            detail: {
                type: 'atoms-removed',
                count: atomsToRemove.size
            }
        }));

        this.checkCacheWarning();  // Update cache status
        return atomsToRemove.size;
    }

    clear() {
        this.atoms.clear();
        this.parents.clear();
        this.children.clear();
        this.roots.clear();
        this.referencedBy.clear();
        this.skippedAtoms = false;
        this.checkCacheWarning();
    }

    // Get atoms for graph view (only nodes and EdgeLink relationships)
    getAtomsForGraphView() {
        // Use Map to track nodes efficiently (key -> node)
        const nodeMap = new Map();
        const edges = [];

        this.atoms.forEach(atom => {
            // Check if this is an EdgeLink/EvaluationLink pattern
            if ((atom.type === 'EdgeLink' || atom.type === 'EvaluationLink') &&
                atom.outgoing && atom.outgoing.length === 2) {

                const predicate = atom.outgoing[0];
                const list = atom.outgoing[1];

                // Validate the pattern AND check if the ListLink is still in cache
                const listKey = this.atomToKey(list);
                if (predicate && (predicate.type === 'PredicateNode' || predicate.type === 'BondNode') &&
                    list && list.type === 'ListLink' &&
                    list.outgoing && list.outgoing.length === 2 &&
                    this.atoms.has(listKey)) {  // Make sure ListLink still exists in cache

                    const fromNode = list.outgoing[0];
                    const toNode = list.outgoing[1];
                    const fromKey = this.atomToKey(fromNode);
                    const toKey = this.atomToKey(toNode);

                    // Check nodes exist and are still in cache
                    if (fromNode && fromNode.type && fromNode.type.endsWith('Node') &&
                        toNode && toNode.type && toNode.type.endsWith('Node') &&
                        this.atoms.has(fromKey) && this.atoms.has(toKey)) {

                        // Add edge info
                        edges.push({
                            from: fromNode,
                            to: toNode,
                            label: predicate.name || 'edge',
                            type: atom.type
                        });

                        // Add nodes to map (O(1) operation)
                        nodeMap.set(fromKey, fromNode);
                        nodeMap.set(toKey, toNode);
                    }
                }
            } else if (atom.type && atom.type.endsWith('Node') &&
                       atom.type !== 'BondNode' && atom.type !== 'PredicateNode') {
                // Add standalone nodes (O(1) operation)
                const atomKey = this.atomToKey(atom);
                nodeMap.set(atomKey, atom);
            }
        });

        // Convert map values to array for return
        const nodes = Array.from(nodeMap.values());
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
            } else if (this.pendingRegularRequest && response && Array.isArray(response)) {
                // Process regular atom response
                this.processRegularAtomResponse(response);
            }
        } catch (error) {
            console.error('Error parsing WebSocket message:', error);
        }
    }

    // Fetch incoming set for an atom
    fetchIncomingSet(atom) {
        // Check if operations were cancelled
        if (this.operationsCancelled) {
            return;
        }

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

        // For ListLinks, queue for sequential processing
        if (atom.type === 'ListLink') {
            this.pendingListLinkRequests.push({
                atom: atom,
                command: command
            });

            if (!this.isProcessingListLink) {
                this.processNextListLink();
            }
        } else {
            // For regular atoms, track the request and send
            this.pendingRegularRequest = {
                atom: atom,
                command: command
            };
            this.socket.send(command);
        }
    }

    // Process ListLink requests sequentially
    processNextListLink() {
        // Check if operations were cancelled
        if (this.operationsCancelled) {
            this.pendingListLinkRequests = [];
            this.isProcessingListLink = false;
            return;
        }

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

        // Check if operations were cancelled
        if (this.operationsCancelled) {
            this.currentListLinkRequest = null;
            this.pendingListLinkRequests = [];
            this.isProcessingListLink = false;
            return;
        }

        const targetAtom = this.currentListLinkRequest.atom;

        // Add all atoms from the response
        // These atoms are in the incoming set of targetAtom, meaning they contain/reference it
        // So THEY are parents of targetAtom, not the other way around
        let addedCount = 0;
        let skippedCount = 0;
        response.forEach(atom => {
            if (atom && typeof atom === 'object') {
                // First add the atom to cache
                const result = this.addAtom(atom, null);
                if (result !== null) {
                    addedCount++;
                    // Then mark it as a parent of the target atom
                    const atomKey = this.atomToKey(atom);
                    const targetKey = this.atomToKey(targetAtom);
                    if (!this.parents.has(targetKey)) {
                        this.parents.set(targetKey, new Set());
                    }
                    this.parents.get(targetKey).add(atomKey);
                    if (!this.children.has(atomKey)) {
                        this.children.set(atomKey, new Set());
                    }
                    this.children.get(atomKey).add(targetKey);
                } else {
                    skippedCount++;
                }
            }
        });

        // Only dispatch update if not cancelled
        if (!this.operationsCancelled) {
            // Notify that cache has been updated
            this.dispatchEvent(new CustomEvent('update', {
                detail: {
                    type: 'incoming-set',
                    parent: targetAtom,
                    atoms: response
                }
            }));
        }

        // Process next ListLink
        this.currentListLinkRequest = null;
        setTimeout(() => this.processNextListLink(), 10);
    }

    // Process regular atom response
    processRegularAtomResponse(response) {
        if (!this.pendingRegularRequest) return;

        const targetAtom = this.pendingRegularRequest.atom;

        // Add all atoms from the response
        // These atoms are in the incoming set of targetAtom, meaning they contain/reference it
        // So THEY are parents of targetAtom, not the other way around
        let addedCount = 0;
        let skippedCount = 0;
        response.forEach(atom => {
            if (atom && typeof atom === 'object') {
                // First add the atom to cache
                const result = this.addAtom(atom, null);
                if (result !== null) {
                    addedCount++;
                    // Then mark it as a parent of the target atom
                    const atomKey = this.atomToKey(atom);
                    const targetKey = this.atomToKey(targetAtom);
                    if (!this.parents.has(targetKey)) {
                        this.parents.set(targetKey, new Set());
                    }
                    this.parents.get(targetKey).add(atomKey);
                    if (!this.children.has(atomKey)) {
                        this.children.set(atomKey, new Set());
                    }
                    this.children.get(atomKey).add(targetKey);
                } else {
                    skippedCount++;
                }
            }
        });

        // Clear the pending request BEFORE dispatching event
        // This ensures hasPendingOperations() returns correct value in event handlers
        this.pendingRegularRequest = null;

        // Notify that cache has been updated
        this.dispatchEvent(new CustomEvent('update', {
            detail: {
                type: 'incoming-set',
                parent: targetAtom,
                atoms: response
            }
        }));

        // If there are no more pending operations, dispatch completion event
        if (!this.hasPendingOperations()) {
            this.dispatchEvent(new CustomEvent('update', {
                detail: { type: 'operations-complete' }
            }));
        }
    }

    // Cancel all pending operations
    cancelAllOperations() {
        // Set cancellation flag
        this.operationsCancelled = true;

        // Clear ListLink queue
        this.pendingListLinkRequests = [];
        this.isProcessingListLink = false;
        this.currentListLinkRequest = null;

        // Clear regular request
        this.pendingRegularRequest = null;

        // Notify that operations were cancelled
        this.dispatchEvent(new CustomEvent('update', {
            detail: { type: 'operations-cancelled' }
        }));
    }

    // Reset cancellation flag (call when starting new operation)
    resetCancellation() {
        this.operationsCancelled = false;
    }

    // Check if there are pending operations
    hasPendingOperations() {
        return this.isProcessingListLink || this.pendingListLinkRequests.length > 0 || this.pendingRegularRequest !== null;
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
