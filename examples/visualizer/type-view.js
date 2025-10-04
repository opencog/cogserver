// AtomType Hierarchy Visualizer
// Displays the inheritance hierarchy of AtomSpace types using a left-to-right tree

let ws = null;
let typeHierarchy = null;
let svgElement = null;
let currentZoom = 1;
let currentX = 0;
let currentY = 0;
let typeData = {};
let allTypes = [];
let queryQueue = [];
let currentPhase = '';
let pendingResponses = 0;
let typeIndex = 0;

document.addEventListener('DOMContentLoaded', () => {
    setupEventListeners();
    // Auto-connect on page load
    connect();
});

function setupEventListeners() {
    const refreshBtn = document.getElementById('refresh-btn');
    const resetViewBtn = document.getElementById('reset-view');

    refreshBtn.addEventListener('click', loadTypes);
    resetViewBtn.addEventListener('click', resetView);
}

function connect() {
    const statusElement = document.getElementById('connection-status');

    // Use current hostname/IP instead of hardcoded localhost
    const serverUrl = `ws://${window.location.hostname}:18080/json`;

    showLoading(true);
    showError(null);

    try {
        ws = new WebSocket(serverUrl);

        ws.onopen = () => {
            console.log('Connected to CogServer');
            statusElement.textContent = 'Connected';
            statusElement.style.color = 'green';

            // Load types immediately after connection
            loadTypes();
        };

        ws.onmessage = (event) => {
            try {
                const response = JSON.parse(event.data);
                handleResponse(response);
            } catch (error) {
                console.error('Error parsing response:', error);
                console.log('Raw response:', event.data);
                showError('Invalid response from server: ' + error.message);
            }
        };

        ws.onerror = (error) => {
            console.error('WebSocket error:', error);
            statusElement.textContent = 'Connection Error';
            statusElement.style.color = 'red';
            showError('Connection error - make sure CogServer is running on localhost:18080');
            showLoading(false);
        };

        ws.onclose = () => {
            console.log('Disconnected from CogServer');
            statusElement.textContent = 'Disconnected';
            statusElement.style.color = 'red';
            showLoading(false);

            // Try to reconnect after 3 seconds
            setTimeout(() => {
                statusElement.textContent = 'Reconnecting...';
                statusElement.style.color = 'orange';
                connect();
            }, 3000);
        };

    } catch (error) {
        console.error('Connection error:', error);
        statusElement.textContent = 'Connection Failed';
        statusElement.style.color = 'red';
        showError('Failed to connect: ' + error.message);
        showLoading(false);
    }
}

function loadTypes() {
    if (!ws || ws.readyState !== WebSocket.OPEN) {
        showError('Not connected to server');
        return;
    }

    showLoading(true);
    showError(null);

    // Reset type data
    typeData = {};
    allTypes = [];
    queryQueue = [];
    currentPhase = 'get-all-types';
    typeIndex = 0;

    // First, get ALL types with one recursive query
    const command = 'AtomSpace.getSubTypes("TopType", true)';
    sendCommand(command);
}

function queryNextType() {
    if (typeIndex >= allTypes.length) {
        // All done
        console.log('All subtype queries complete');
        showLoading(false);
        buildHierarchyLevels();
        renderHierarchy();
        return;
    }

    const typeName = allTypes[typeIndex];
    queryQueue.push(typeName); // Track which type we're querying
    const command = `AtomSpace.getSubTypes("${typeName}", false)`;
    sendCommand(command);
}

function buildHierarchyLevels() {
    console.log('Building hierarchy levels using shortest path...');

    // Make sure TopType exists
    if (!typeData['TopType']) {
        console.error('TopType not found! Creating it.');
        typeData['TopType'] = {
            name: 'TopType',
            parents: [],
            children: [],
            queried: true,
            level: 0
        };
    }

    // Build a set of Arg's immediate children
    const argChildren = new Set(typeData['Arg']?.children || []);
    console.log('Arg children:', Array.from(argChildren));

    // Calculate the SHORTEST path level for each type using BFS
    // This handles multiple inheritance correctly
    const levels = {};
    const queue = [{ type: 'TopType', level: 0 }];
    const visited = new Set();

    // Initialize all types to max level
    Object.keys(typeData).forEach(type => {
        levels[type] = Number.MAX_SAFE_INTEGER;
    });
    levels['TopType'] = 0;

    // BFS to find shortest path to each type
    while (queue.length > 0) {
        const { type, level } = queue.shift();

        // Skip if we've already found a shorter or equal path to this type
        if (level > levels[type]) {
            continue;
        }

        levels[type] = level;

        // Process children
        if (typeData[type] && typeData[type].children) {
            typeData[type].children.forEach(child => {
                // Only process if this gives a shorter path
                if (level + 1 < levels[child]) {
                    queue.push({ type: child, level: level + 1 });
                }
            });
        }
    }

    // Apply the calculated levels
    Object.entries(levels).forEach(([type, level]) => {
        if (typeData[type]) {
            if (level === Number.MAX_SAFE_INTEGER) {
                // Orphaned type - not connected to TopType
                console.warn('Orphaned type (not connected to TopType):', type);
                typeData[type].level = 0;
                typeData[type].orphaned = true;
            } else {
                typeData[type].level = level;
            }
        }
    });

    // Find the primary parent for each type (parent with shortest path to TopType)
    // BUT ignore parents that are children of Arg
    Object.entries(typeData).forEach(([typeName, typeInfo]) => {
        if (typeInfo.parents.length > 0) {
            // Filter out any parents that are Arg children
            const validParents = typeInfo.parents.filter(parent => !argChildren.has(parent));

            if (validParents.length > 0) {
                // Find parent with lowest level from valid parents only
                let primaryParent = null;
                let minLevel = Number.MAX_SAFE_INTEGER;

                validParents.forEach(parent => {
                    if (typeData[parent] && typeData[parent].level < minLevel) {
                        minLevel = typeData[parent].level;
                        primaryParent = parent;
                    }
                });

                typeInfo.primaryParent = primaryParent;
                typeInfo.secondaryParents = typeInfo.parents.filter(p => p !== primaryParent);
            } else {
                // All parents are Arg children, use the original logic
                let primaryParent = null;
                let minLevel = Number.MAX_SAFE_INTEGER;

                typeInfo.parents.forEach(parent => {
                    if (typeData[parent] && typeData[parent].level < minLevel) {
                        minLevel = typeData[parent].level;
                        primaryParent = parent;
                    }
                });

                typeInfo.primaryParent = primaryParent;
                typeInfo.secondaryParents = typeInfo.parents.filter(p => p !== primaryParent);
            }

            console.log(`${typeName} has multiple parents. Primary: ${typeInfo.primaryParent}, Secondary: ${typeInfo.secondaryParents}`);
        }
    });

    const maxLevel = Math.max(...Object.values(typeData).map(t => t.level).filter(l => l < Number.MAX_SAFE_INTEGER));
    console.log('Hierarchy levels built. Total levels:', maxLevel + 1);

    // Count types at each level
    const levelCounts = {};
    Object.values(typeData).forEach(type => {
        if (!type.orphaned) {
            levelCounts[type.level] = (levelCounts[type.level] || 0) + 1;
        }
    });

    console.log('Types per level:', levelCounts);
    console.log('TopType children:', typeData['TopType']?.children);

    if (typeData['Value']) {
        console.log('Value info:', {
            level: typeData['Value'].level,
            children: typeData['Value'].children.slice(0, 5),
            parents: typeData['Value'].parents
        });
    }

    if (typeData['BoolValue']) {
        console.log('BoolValue info:', {
            level: typeData['BoolValue'].level,
            parents: typeData['BoolValue'].parents,
            primaryParent: typeData['BoolValue'].primaryParent,
            secondaryParents: typeData['BoolValue'].secondaryParents
        });
    }
}

function sendCommand(command) {
    if (!ws || ws.readyState !== WebSocket.OPEN) {
        showError('Not connected to server');
        return;
    }

    console.log('Sending command:', command);
    ws.send(command + '\n');
}

function handleResponse(response) {
    console.log('Received response for phase:', currentPhase);

    // Handle MCP content-based response
    if (response.content && Array.isArray(response.content)) {
        // Check for error
        if (response.isError === true) {
            const errorText = response.content[0]?.text || 'Unknown error';
            showError('Server error: ' + errorText);
            showLoading(false);
            return;
        }

        // Parse content text (may be JSON string)
        const contentText = response.content[0]?.text || '';
        try {
            response = JSON.parse(contentText);
        } catch {
            response = contentText;
        }
    }

    // Extract the array from response
    let types = [];
    if (Array.isArray(response)) {
        types = response;
    } else if (response.result && Array.isArray(response.result)) {
        types = response.result;
    }

    if (currentPhase === 'get-all-types') {
        // Phase 1: We got all type names
        allTypes = types;
        console.log(`Received ${allTypes.length} total types`);

        // Initialize typeData for all types
        allTypes.forEach(typeName => {
            typeData[typeName] = {
                name: typeName,
                parents: [],
                children: [],
                queried: false
            };
        });

        // Add TopType if not present
        if (!typeData['TopType']) {
            console.log('TopType not in response, adding it manually');
            typeData['TopType'] = {
                name: 'TopType',
                parents: [],
                children: [],
                queried: false
            };
            allTypes.unshift('TopType');
        } else {
            console.log('TopType found in response');
        }

        // Phase 2: Query each type for its immediate children
        currentPhase = 'get-subtypes';
        pendingResponses = allTypes.length;
        queryQueue = []; // Reset queue

        console.log('Starting to query immediate subtypes for each type...');

        // Send queries one at a time to maintain order
        // Start with the first type
        queryNextType();

    } else if (currentPhase === 'get-subtypes') {
        // Phase 2: Process immediate subtypes for a type
        const parentType = queryQueue.shift();

        if (parentType && typeData[parentType]) {
            typeData[parentType].children = types;
            typeData[parentType].queried = true;

            // Update parent relationships for children
            types.forEach(childType => {
                if (typeData[childType]) {
                    if (!typeData[childType].parents.includes(parentType)) {
                        typeData[childType].parents.push(parentType);
                    }
                }
            });

            if (types.length > 0) {
                console.log(`${parentType} → [${types.slice(0, 5).join(', ')}${types.length > 5 ? '...' : ''}]`);
            }
        }

        // Move to next type
        typeIndex++;
        queryNextType();
    }
}

// buildTypeHierarchy removed - now using incremental queries

function renderHierarchy() {
    console.log('renderHierarchy called');

    // Update status
    document.getElementById('types-loaded').textContent = Object.keys(typeData).length;

    // Show panels
    const treeContainer = document.getElementById('tree-container');
    if (treeContainer) {
        treeContainer.classList.remove('hidden');
        console.log('Tree container shown, classes:', treeContainer.className);

        // Check container dimensions
        const rect = treeContainer.getBoundingClientRect();
        console.log('Tree container dimensions:', rect.width, 'x', rect.height);

        if (rect.width === 0 || rect.height === 0) {
            console.error('Tree container has zero dimensions!');
        }
    } else {
        console.error('Tree container not found!');
    }

    // Render the tree
    renderTree();
}

// Removed old processTypeList and processTypeHierarchy functions
// Now using incremental hierarchy building with querySubtypes

function renderTree() {
    const container = document.getElementById('type-tree');

    console.log('renderTree called. TypeData size:', Object.keys(typeData).length);
    console.log('Container element:', container);
    console.log('Container parent:', container?.parentElement);

    // Clear previous content
    while (container.firstChild) {
        container.removeChild(container.firstChild);
    }

    // Now continue with the real tree
    if (Object.keys(typeData).length === 0) {
        showError('No type data available');
        return;
    }

    // Calculate tree dimensions - make sure container has size
    const containerRect = container.getBoundingClientRect();
    const width = containerRect.width || 1200;
    // Ensure adequate height for all nodes - use more space per node
    const height = Math.max(800, Object.keys(typeData).length * 30);

    console.log('Container getBoundingClientRect:', containerRect);
    console.log('Container dimensions:', width, 'x', height);

    // Container IS the SVG element, so use it directly
    const svg = container;
    svg.setAttribute('width', width);
    svg.setAttribute('height', height);
    svg.setAttribute('viewBox', `0 0 ${width} ${height}`);
    svg.setAttribute('style', 'background: white;');

    // Add a background rectangle to capture all mouse events
    const bgRect = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
    bgRect.setAttribute('width', width);
    bgRect.setAttribute('height', height);
    bgRect.setAttribute('fill', 'white');
    bgRect.setAttribute('pointer-events', 'all');
    svg.appendChild(bgRect);

    // Create main group for zooming/panning
    const mainGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    mainGroup.setAttribute('id', 'main-group');
    svg.appendChild(mainGroup);

    // TopType is the root
    let rootTypes = ['TopType'];

    // Make sure TopType exists and has no parents
    if (typeData['TopType']) {
        typeData['TopType'].parents = [];
        console.log('TopType found with children:', typeData['TopType'].children);
    } else {
        console.error('TopType not found in hierarchy!');
        console.log('Available types sample:', Object.keys(typeData).slice(0, 10));
        // Try to continue anyway with what we have
        // showError('TopType not found in hierarchy');
        // return;
    }

    console.log('Root type: TopType');
    console.log('TopType children:', typeData['TopType'].children);
    console.log('Total types:', Object.keys(typeData).length);

    // Layout the tree
    const nodePositions = layoutTree(rootTypes, width, height);

    console.log('Node positions calculated:', Object.keys(nodePositions).length);
    console.log('Sample positions:', Object.entries(nodePositions).slice(0, 3));

    // Only render if we have positions
    if (Object.keys(nodePositions).length === 0) {
        console.error('No node positions calculated!');
        showError('Failed to calculate node positions');
        return;
    }

    // Recalculate height based on actual positions
    const actualHeight = Math.max(...Object.values(nodePositions).map(p => p.y)) + 50;
    if (actualHeight > height) {
        svg.setAttribute('height', actualHeight);
        svg.setAttribute('viewBox', `0 0 ${width} ${actualHeight}`);
        bgRect.setAttribute('height', actualHeight);
        console.log('Adjusted SVG height to:', actualHeight);
    }

    // Draw edges (connections)
    drawEdges(mainGroup, nodePositions);

    // Draw nodes
    drawNodes(mainGroup, nodePositions);

    // Don't append svg since container IS the svg
    svgElement = svg;

    console.log('SVG appended to container');
    console.log('SVG dimensions:', svg.getAttribute('width'), 'x', svg.getAttribute('height'));
    console.log('Container now has', container.children.length, 'children');

    // Check the main group
    const mainGroupCheck = document.getElementById('main-group');
    console.log('Main group found:', mainGroupCheck ? 'yes' : 'no');
    if (mainGroupCheck) {
        console.log('Main group children:', mainGroupCheck.children.length);
    }

    // Enable panning
    enablePanning(svg, mainGroup);
}

function layoutTree(rootTypes, width, height) {
    const positions = {};
    const levelWidth = 150;  // Horizontal spacing between levels (columns)
    const nodeHeight = 35;   // Vertical spacing between nodes
    const padding = 20;
    const subtreeSpacing = 20; // Extra space between subtrees

    // Use recursive approach to layout subtrees properly
    let currentY = padding;

    // Use the argChildren set from buildHierarchyLevels (already defined earlier)
    const argChildrenSet = new Set(typeData['Arg']?.children || []);

    function layoutSubtree(nodeName, level, parentY = null) {
        if (positions[nodeName]) return 0; // Already positioned

        const nodeInfo = typeData[nodeName];
        if (!nodeInfo) return 0;

        // Position this node
        positions[nodeName] = {
            x: padding + level * levelWidth,
            y: currentY,
            level: level
        };

        let subtreeHeight = nodeHeight;
        currentY += nodeHeight;

        // If this is an Arg child, stop here - don't layout its children
        if (argChildrenSet.has(nodeName)) {
            console.log(`Stopping at ${nodeName} (Arg child)`);
            return subtreeHeight;
        }

        // Get children and group them by this parent
        const children = nodeInfo.children || [];

        // Layout all children
        children.forEach((child, idx) => {
            if (typeData[child] && typeData[child].primaryParent === nodeName) {
                // Only layout if this is the primary parent
                const childHeight = layoutSubtree(child, level + 1, positions[nodeName].y);
                subtreeHeight += childHeight;
            }
        });

        return subtreeHeight;
    }

    // Start with TopType
    if (typeData['TopType']) {
        // Layout TopType itself
        positions['TopType'] = {
            x: padding,
            y: currentY,
            level: 0
        };
        currentY += nodeHeight;

        // Layout Arg subtree
        if (typeData['Arg']) {
            const argHeight = layoutSubtree('Arg', 1);
            // Add extra spacing after Arg subtree
            currentY += subtreeSpacing * 2;
        }

        // Layout Value subtree
        if (typeData['Value']) {
            layoutSubtree('Value', 1);
        }

        // Layout any other direct children of TopType
        const topChildren = typeData['TopType'].children || [];
        topChildren.forEach(child => {
            if (child !== 'Arg' && child !== 'Value' && !positions[child]) {
                layoutSubtree(child, 1);
            }
        });
    }

    // Handle any orphaned nodes (shouldn't happen but just in case)
    Object.keys(typeData).forEach(typeName => {
        if (!positions[typeName]) {
            const nodeInfo = typeData[typeName];
            positions[typeName] = {
                x: padding + (nodeInfo.level || 0) * levelWidth,
                y: currentY,
                level: nodeInfo.level || 0
            };
            currentY += nodeHeight;
        }
    });

    // Calculate actual tree bounds
    const maxLevel = Math.max(0, ...Object.values(positions).map(p => p.level));
    const maxY = Math.max(...Object.values(positions).map(p => p.y)) + nodeHeight;

    console.log('Layout complete. Total levels:', maxLevel + 1);
    console.log('Total nodes positioned:', Object.keys(positions).length);
    console.log('Tree height:', maxY);

    // Log the structure
    const level1Nodes = Object.entries(positions)
        .filter(([name, pos]) => pos.level === 1)
        .map(([name]) => name);
    const argChildren = typeData['Arg']?.children || [];
    const valueChildren = typeData['Value']?.children || [];

    console.log('Level 1 nodes:', level1Nodes);
    console.log('Arg children:', argChildren.slice(0, 5), '...');
    console.log('Value children:', valueChildren.slice(0, 5), '...');

    return positions;
}

function drawEdges(container, positions) {
    const edgeGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    edgeGroup.setAttribute('id', 'edges');

    let edgeCount = 0;

    for (const [typeName, typeInfo] of Object.entries(typeData)) {
        if (!positions[typeName]) continue;
        if (typeInfo.orphaned) continue;  // Skip orphaned types

        const childPos = positions[typeName];

        // Only draw edge to primary parent for now (shortest path)
        if (typeInfo.primaryParent && positions[typeInfo.primaryParent]) {
            const parentPos = positions[typeInfo.primaryParent];

            // Create squared-off path
            const path = document.createElementNS('http://www.w3.org/2000/svg', 'path');

            // Calculate path with right angles
            const startX = parentPos.x + 100;  // Box width
            const startY = parentPos.y + 15;   // Half box height
            const endX = childPos.x;
            const endY = childPos.y + 15;
            const midX = startX + (endX - startX) / 2;

            const d = `M ${startX} ${startY} L ${midX} ${startY} L ${midX} ${endY} L ${endX} ${endY}`;
            path.setAttribute('d', d);
            path.setAttribute('stroke', '#333');
            path.setAttribute('stroke-width', '1.5');
            path.setAttribute('fill', 'none');

            edgeGroup.appendChild(path);
            edgeCount++;
        }

        // Optionally draw secondary parents with dashed lines
        // Commented out for now to keep it simple
        /*
        if (typeInfo.secondaryParents) {
            typeInfo.secondaryParents.forEach(parent => {
                if (positions[parent]) {
                    // Draw dashed line for secondary parents
                    // ...
                }
            });
        }
        */
    }

    console.log('Drew', edgeCount, 'edges');
    container.appendChild(edgeGroup);
}

function drawNodes(container, positions) {
    console.log('drawNodes called with', Object.keys(positions).length, 'positions');

    const nodeGroup = document.createElementNS('http://www.w3.org/2000/svg', 'g');
    nodeGroup.setAttribute('id', 'nodes');

    let nodeCount = 0;
    for (const [typeName, position] of Object.entries(positions)) {
        const g = document.createElementNS('http://www.w3.org/2000/svg', 'g');
        g.setAttribute('class', 'tree-node');
        g.setAttribute('transform', `translate(${position.x}, ${position.y})`);

        // Rectangle background
        const rect = document.createElementNS('http://www.w3.org/2000/svg', 'rect');
        rect.setAttribute('width', '100');
        rect.setAttribute('height', '30');
        rect.setAttribute('rx', '3');
        rect.setAttribute('ry', '3');
        rect.setAttribute('fill', 'white');
        rect.setAttribute('stroke', '#333');
        rect.setAttribute('stroke-width', '1');

        // Text label
        const text = document.createElementNS('http://www.w3.org/2000/svg', 'text');
        text.setAttribute('x', '50');
        text.setAttribute('y', '20');
        text.setAttribute('text-anchor', 'middle');
        text.setAttribute('fill', 'black');
        text.setAttribute('font-size', '12px');
        text.setAttribute('font-family', 'sans-serif');
        text.textContent = typeName;

        // Add tooltip
        const title = document.createElementNS('http://www.w3.org/2000/svg', 'title');
        title.textContent = `Click to open documentation for ${typeName}`;
        g.appendChild(title);

        // Add click handler to open wiki documentation
        g.addEventListener('click', (e) => {
            // Open wiki documentation in new tab
            const wikiUrl = `https://wiki.opencog.org/w/${typeName}`;
            window.open(wikiUrl, '_blank');

            // Also update the info panel
            selectType(typeName);

            // Prevent event bubbling
            e.stopPropagation();
        });

        // Add hover effect
        g.addEventListener('mouseenter', () => {
            rect.setAttribute('fill', '#e3f2fd');
            rect.setAttribute('stroke', '#1976d2');
            rect.setAttribute('stroke-width', '2');
        });

        g.addEventListener('mouseleave', () => {
            rect.setAttribute('fill', 'white');
            rect.setAttribute('stroke', '#333');
            rect.setAttribute('stroke-width', '1');
        });

        // Change cursor to pointer
        g.style.cursor = 'pointer';

        g.appendChild(rect);
        g.appendChild(text);
        nodeGroup.appendChild(g);
        nodeCount++;
    }

    console.log('Created', nodeCount, 'node elements');
    container.appendChild(nodeGroup);

    // Check if nodes are actually in the DOM
    const addedNodes = container.querySelectorAll('.tree-node');
    console.log('Nodes in DOM after adding:', addedNodes.length);
}

function selectType(typeName) {
    const info = typeData[typeName];
    if (!info) return;

    document.getElementById('selected-type').classList.remove('hidden');
    document.getElementById('type-name').textContent = typeName;
    document.getElementById('type-parents').textContent =
        info.parents.length > 0 ? info.parents.join(', ') : 'None (Root Type)';
    document.getElementById('type-children').textContent =
        info.children.length > 0 ? info.children.join(', ') : 'None (Leaf Type)';
}

function enablePanning(svg, group) {
    let isPanning = false;
    let startX = 0;
    let startY = 0;

    // Mouse drag for panning
    svg.addEventListener('mousedown', (e) => {
        if (e.button === 0) { // Left mouse button only
            isPanning = true;
            startX = e.clientX - currentX;
            startY = e.clientY - currentY;
            svg.classList.add('grabbing');
            e.preventDefault();
            e.stopPropagation();
        }
    });

    svg.addEventListener('mousemove', (e) => {
        if (!isPanning) return;
        e.preventDefault();

        currentX = e.clientX - startX;
        currentY = e.clientY - startY;

        group.setAttribute('transform',
            `translate(${currentX}, ${currentY}) scale(${currentZoom})`);
    });

    svg.addEventListener('mouseup', (e) => {
        if (isPanning) {
            isPanning = false;
            svg.classList.remove('grabbing');
            e.preventDefault();
        }
    });

    svg.addEventListener('mouseleave', () => {
        if (isPanning) {
            isPanning = false;
            svg.classList.remove('grabbing');
        }
    });

    // Mouse wheel for zooming - attached to container for better capture
    const container = svg.parentElement;
    const zoomHandler = (e) => {
        // Only handle if mouse is over the SVG area
        const rect = svg.getBoundingClientRect();
        if (e.clientX < rect.left || e.clientX > rect.right ||
            e.clientY < rect.top || e.clientY > rect.bottom) {
            return;
        }

        e.preventDefault();
        e.stopPropagation();

        // Get mouse position relative to SVG
        const mouseX = e.clientX - rect.left;
        const mouseY = e.clientY - rect.top;

        // Calculate zoom factor
        const zoomFactor = e.deltaY > 0 ? 0.9 : 1.1;
        const newZoom = currentZoom * zoomFactor;

        // Limit zoom range
        if (newZoom < 0.1 || newZoom > 5) return;

        // Calculate new position to zoom towards mouse cursor
        // We need to adjust for the current transform
        const scaleDiff = newZoom / currentZoom;
        const dx = mouseX * (1 - scaleDiff);
        const dy = mouseY * (1 - scaleDiff);

        currentX = currentX * scaleDiff + dx;
        currentY = currentY * scaleDiff + dy;
        currentZoom = newZoom;

        group.setAttribute('transform',
            `translate(${currentX}, ${currentY}) scale(${currentZoom})`);
    };

    // Add wheel event to both svg and container for better capture
    svg.addEventListener('wheel', zoomHandler, { passive: false });
    if (container) {
        container.addEventListener('wheel', zoomHandler, { passive: false });
    }
}

function resetView() {
    currentZoom = 1;
    currentX = 0;
    currentY = 0;
    const group = document.getElementById('main-group');
    if (group) {
        group.setAttribute('transform', 'translate(0, 0) scale(1)');
    }
}

function showLoading(show) {
    const panel = document.getElementById('loading-panel');
    if (show) {
        panel.classList.remove('hidden');
    } else {
        panel.classList.add('hidden');
    }
}

function showError(message) {
    const panel = document.getElementById('error-panel');
    const messageElement = document.getElementById('error-message');

    if (message) {
        messageElement.textContent = message;
        panel.classList.remove('hidden');
    } else {
        panel.classList.add('hidden');
    }
}