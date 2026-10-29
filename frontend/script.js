/*
 * script.js
 * -------------------------------------------------------------
 * Frontend Controller for Undo-Redo Text Editor Web Application
 *
 * CRITICAL DSA & ARCHITECTURAL RULE:
 * This script DOES NOT maintain any client-side history or stacks.
 * All stack operations (push, pop, clear, undo, redo, commit)
 * are executed strictly inside the C Winsock backend.
 * -------------------------------------------------------------
 */

// Determine API base URL (supports http://127.0.0.1:8080, http://localhost:8080, and file:// preview)
const API_BASE = (window.location.origin && window.location.origin.startsWith("http"))
    ? window.location.origin
    : "http://127.0.0.1:8080";

// DOM Elements
const editorArea = document.getElementById("editorArea");
const btnApply = document.getElementById("btnApply");
const btnUndo = document.getElementById("btnUndo");
const btnRedo = document.getElementById("btnRedo");
const btnSave = document.getElementById("btnSave");
const btnLoad = document.getElementById("btnLoad");
const btnClear = document.getElementById("btnClear");

const deleteCount = document.getElementById("deleteCount");
const btnDelete = document.getElementById("btnDelete");
const insertInput = document.getElementById("insertInput");
const btnInsert = document.getElementById("btnInsert");

const undoCountDisplay = document.getElementById("undoCountDisplay");
const redoCountDisplay = document.getElementById("redoCountDisplay");
const textLengthDisplay = document.getElementById("textLengthDisplay");
const serverStatusDisplay = document.getElementById("serverStatusDisplay");
const navServerStatus = document.getElementById("navServerStatus");
const toastContainer = document.getElementById("toastContainer");

/* -------------------------------------------------------------
 * Floating Toast Notification System (Municipal Webapp Style)
 * ------------------------------------------------------------- */
function showToast(message, type = "info", duration = 3000) {
    if (!message || !toastContainer) return;

    const toast = document.createElement("div");
    toast.className = `toast toast-${type}`;

    let icon = "ℹ️";
    if (type === "success") icon = "✅";
    else if (type === "error") icon = "⚠️";
    else if (type === "warning") icon = "🔔";

    toast.innerHTML = `<span>${icon}</span> <span>${message}</span>`;
    toastContainer.appendChild(toast);

    // Auto fade-out and removal
    setTimeout(() => {
        toast.classList.add("toast-fade-out");
        setTimeout(() => {
            if (toast.parentNode) {
                toast.parentNode.removeChild(toast);
            }
        }, 300);
    }, duration);
}

/* -------------------------------------------------------------
 * Server Connection Status Helper
 * ------------------------------------------------------------- */
function setServerStatus(online) {
    if (serverStatusDisplay) {
        serverStatusDisplay.textContent = online ? "Online" : "Offline";
        serverStatusDisplay.className = `metric-value status-${online ? "online" : "offline"}`;
    }
    if (navServerStatus) {
        const dot = navServerStatus.querySelector(".status-dot");
        if (dot) {
            dot.className = `status-dot ${online ? "online" : "offline"}`;
        }
        const text = document.getElementById("navStatusText");
        if (text) {
            text.textContent = `C Backend: ${online ? "Online" : "Offline"}`;
        }
    }
}

/* -------------------------------------------------------------
 * Update UI from C Backend Response
 * ------------------------------------------------------------- */
function updateUI(data, isCompletedOperation = true, showFeedback = true) {
    if (!data) return;

    // 1. Synchronize Textarea Content
    // Only update if it is an explicit operation/initial load,
    // or if the user is not actively typing in the editorArea.
    if (data.text !== undefined) {
        if (isCompletedOperation || document.activeElement !== editorArea) {
            editorArea.value = data.text;
            editorArea.defaultValue = data.text;
            editorArea.textContent = data.text;
        }
    }

    // 2. Dashboard Metrics and Stack Counters
    if (undoCountDisplay && data.undoCount !== undefined) {
        undoCountDisplay.textContent = data.undoCount;
    }
    if (redoCountDisplay && data.redoCount !== undefined) {
        redoCountDisplay.textContent = data.redoCount;
    }
    if (textLengthDisplay) {
        textLengthDisplay.textContent = data.length !== undefined ? data.length : editorArea.value.length;
    }

    // 3. Undo / Redo Buttons Active States (Strictly driven by C backend)
    if (btnUndo && data.canUndo !== undefined) {
        btnUndo.disabled = !data.canUndo;
    }
    if (btnRedo && data.canRedo !== undefined) {
        btnRedo.disabled = !data.canRedo;
    }

    // 4. Server Online Status Indicators
    setServerStatus(true);

    // 5. Toast Feedback
    if (showFeedback && data.message) {
        const type = data.success ? "success" : "warning";
        showToast(data.message, type);
    }
}

/* -------------------------------------------------------------
 * Generic API Call Helper
 * ------------------------------------------------------------- */
async function apiCall(endpoint, method = "GET", bodyData = null, isCompletedOperation = true, showFeedback = true) {
    try {
        const options = {
            method: method,
            headers: {
                "Content-Type": "application/json"
            }
        };
        if (bodyData !== null) {
            options.body = JSON.stringify(bodyData);
        }

        const response = await fetch(`${API_BASE}${endpoint}`, options);
        if (!response.ok) {
            throw new Error(`HTTP ${response.status}: ${response.statusText}`);
        }

        const data = await response.json();
        updateUI(data, isCompletedOperation, showFeedback);
        return data;
    } catch (err) {
        console.error("API Call Error:", err);
        setServerStatus(false);
        if (btnUndo) btnUndo.disabled = true;
        if (btnRedo) btnRedo.disabled = true;
        if (showFeedback) {
            showToast("Cannot connect to C Backend at " + API_BASE + ". Ensure backend.exe is running.", "error", 4000);
        }
        return null;
    }
}

/* -------------------------------------------------------------
 * Editor Actions (All state transitions handled in C)
 * ------------------------------------------------------------- */

// 1. Apply Changes (Commits direct textarea edits to C Undo Stack)
async function onApplyChanges() {
    const text = editorArea.value;
    await apiCall("/api/set-text", "POST", { text: text }, true, true);
}

// 2. Undo (Moves state from Undo Stack to Redo Stack in C)
async function onUndo() {
    await apiCall("/api/undo", "POST", null, true, true);
}

// 3. Redo (Moves state from Redo Stack to Undo Stack in C)
async function onRedo() {
    await apiCall("/api/redo", "POST", null, true, true);
}

// 4. Save to data/document.txt
async function onSave() {
    await apiCall("/api/save", "POST", null, true, true);
}

// 5. Load from data/document.txt
async function onLoad() {
    await apiCall("/api/load", "POST", null, true, true);
}

// 6. Clear Document (State saved to Undo Stack in C)
async function onClear() {
    await apiCall("/api/clear", "POST", null, true, true);
}

// 7. Delete Characters from End
async function onDelete() {
    const count = parseInt(deleteCount.value, 10);
    if (isNaN(count) || count <= 0) {
        showToast("Delete count must be a positive number.", "warning");
        deleteCount.focus();
        return;
    }
    await apiCall("/api/delete", "POST", { count: count }, true, true);
}

// 8. Quick Append
async function onInsert() {
    const text = insertInput.value;
    if (!text || text.trim() === "") {
        showToast("Please enter text to append.", "warning");
        insertInput.focus();
        return;
    }

    const res = await apiCall("/api/insert", "POST", { text: text }, true, true);
    if (res && res.success) {
        insertInput.value = "";
    }
}

/* -------------------------------------------------------------
 * Keyboard Shortcuts (Purely dispatching to C Backend API)
 * ------------------------------------------------------------- */
function setupKeyboardShortcuts() {
    document.addEventListener("keydown", (e) => {
        const isMac = navigator.platform.toUpperCase().indexOf("MAC") >= 0;
        const modifier = isMac ? e.metaKey : e.ctrlKey;

        if (!modifier) return;

        const key = e.key.toLowerCase();

        // 1. Ctrl + Z -> Undo
        if (key === "z" && !e.shiftKey) {
            e.preventDefault();
            if (!btnUndo.disabled) {
                onUndo();
            } else {
                showToast("Undo Stack is empty", "info", 1500);
            }
        }
        // 2. Ctrl + Y OR Ctrl + Shift + Z -> Redo
        else if (key === "y" || (key === "z" && e.shiftKey)) {
            e.preventDefault();
            if (!btnRedo.disabled) {
                onRedo();
            } else {
                showToast("Redo Stack is empty", "info", 1500);
            }
        }
        // 3. Ctrl + S -> Save (Prevent browser save dialog)
        else if (key === "s") {
            e.preventDefault();
            onSave();
        }
    });
}

/* -------------------------------------------------------------
 * Initialization Function
 * ------------------------------------------------------------- */
function init() {
    // Bind Event Listeners
    if (btnApply) btnApply.addEventListener("click", onApplyChanges);
    if (btnUndo) btnUndo.addEventListener("click", onUndo);
    if (btnRedo) btnRedo.addEventListener("click", onRedo);
    if (btnSave) btnSave.addEventListener("click", onSave);
    if (btnLoad) btnLoad.addEventListener("click", onLoad);
    if (btnClear) btnClear.addEventListener("click", onClear);
    if (btnDelete) btnDelete.addEventListener("click", onDelete);
    if (btnInsert) btnInsert.addEventListener("click", onInsert);

    // Live character count while user is actively typing in the textarea
    if (editorArea) {
        editorArea.addEventListener("input", () => {
            if (textLengthDisplay) {
                textLengthDisplay.textContent = editorArea.value.length;
            }
        });
    }

    if (insertInput) {
        insertInput.addEventListener("keydown", (e) => {
            if (e.key === "Enter") {
                e.preventDefault();
                onInsert();
            }
        });
    }

    // Bind Web App Keyboard Shortcuts
    setupKeyboardShortcuts();

    // Initial state fetch from C backend (silent without toast)
    apiCall("/api/status", "GET", null, true, false);
}

// Safely execute initialization even if DOM is already parsed
if (document.readyState === "loading") {
    document.addEventListener("DOMContentLoaded", init);
} else {
    init();
}
