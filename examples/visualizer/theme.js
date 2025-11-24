// Theme management for AtomSpace Visualizer
// Shared across all visualizer pages

function initTheme() {
    // Check URL parameter first, then saved preference, otherwise default to dark
    const urlParams = new URLSearchParams(window.location.search);
    const urlTheme = urlParams.get('theme');
    const savedTheme = localStorage.getItem('theme') || 'dark';
    const theme = urlTheme || savedTheme;
    setTheme(theme);
}

function setTheme(theme) {
    document.body.setAttribute('data-theme', theme);
    localStorage.setItem('theme', theme);

    // Update icon
    const themeIcon = document.getElementById('theme-icon');
    if (themeIcon) {
        themeIcon.textContent = theme === 'dark' ? '☀️' : '🌙';
    }
}

function toggleTheme() {
    const currentTheme = document.body.getAttribute('data-theme');
    const newTheme = currentTheme === 'dark' ? 'light' : 'dark';
    setTheme(newTheme);
}

// Set up theme when DOM is ready
document.addEventListener('DOMContentLoaded', function() {
    // Initialize theme first
    initTheme();

    // Then set up the toggle button
    const themeToggle = document.getElementById('theme-toggle');
    if (themeToggle) {
        themeToggle.addEventListener('click', toggleTheme);
    }
});
