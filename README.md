# Undo-Redo Text Editor using Two Stacks

A simple, college-level Data Structures and Algorithms (DSA) mini project demonstrating the **Two Stacks** approach to implement **Undo** and **Redo** functionality in a web-based text editor with a **C language backend**.

---

## 1. Project Objective

The primary purpose of this project is to demonstrate the practical application of the **Stack** data structure (LIFO — Last In, First Out) in modern software applications like text editors. Rather than using databases or high-level frameworks, this project builds the stack and editor engine manually in C and connects it to a clean browser frontend via a lightweight Windows socket HTTP server.

---

## 2. Technologies Used

- **Backend:** C Language (C99 standard)
- **Networking:** Native Windows Sockets (`Winsock2`, `-lws2_32`)
- **Frontend:** Plain HTML5, CSS3, JavaScript (ES6 `fetch`)
- **Storage:** Plain text file (`data/document.txt`)
- **Database:** None (No SQLite, MySQL, or MongoDB)
- **Frameworks:** None (No React, Node.js, Express, or external libraries)

---

## 3. DSA Concept: Two Stacks (Undo & Redo)

The editor maintains two manual, array-based stacks:
1. **Undo Stack:** Stores snapshots of past text states before modifications.
2. **Redo Stack:** Stores snapshots that have been undone, enabling the user to re-apply them.

### How Undo Works:
1. When user requests **Undo**, the active text is pushed onto the **Redo Stack**.
2. The previous snapshot is popped from the **Undo Stack**.
3. That popped state becomes the new active text in the editor.

### How Redo Works:
1. When user requests **Redo**, the active text is pushed onto the **Undo Stack**.
2. The next state is popped from the **Redo Stack**.
3. That state becomes the new active text in the editor.

### Why Redo Stack is Cleared on New Edit (Branch Rule):
In any text editor, when an Undo is performed and the user then performs a **new modification** (typing, deleting, or clearing), the forward history branch is invalidated. The Redo Stack is completely cleared (`clear(&editor->redoStack)`) to preserve linear timeline consistency.

```
       [Type / Edit] ──► Push old state to Undo Stack ──► Clear Redo Stack
                                  ▲                             │
                                  │                             │
                             [Undo Action]                [Redo Action]
                                  │                             ▲
                                  ▼                             │
       [Undo Action] ──► Push current to Redo Stack ────────────┘
```

---

## 4. DSA Time & Space Complexity

| Operation | Stack Time Complexity | String Copying Cost | Overall Time Complexity |
| :--- | :---: | :---: | :---: |
| `push()` | $O(1)$ | $O(N)$ (where $N$ is text length) | $O(N)$ snapshot capture |
| `pop()` | $O(1)$ | $O(1)$ pointer retrieval | $O(1)$ |
| `isEmpty()` | $O(1)$ | — | $O(1)$ |
| `clear()` | $O(1)$ | — | $O(1)$ (resets top index to -1) |

- **Stack Operations:** Basic pointer manipulations (`top++`, `top--`) run in $O(1)$ constant time.
- **Text Snapshotting:** Copying string snapshots takes $O(N)$ time proportional to the length of the text buffer ($N \le 4096$).
- **Space Complexity:** $O(M \times N)$ where $M$ is `MAX_STACK_SIZE` (100) and $N$ is `MAX_TEXT_SIZE` (4096 bytes).

---

## 5. Project Structure

```text
UndoRedoTextEditor/
├── backend/
│   ├── stack.h        # Manual Stack structure & prototypes
│   ├── stack.c        # Stack implementation (push, pop, isEmpty, clear)
│   ├── editor.h       # Text Editor state with Undo & Redo stacks
│   ├── editor.c       # Editor operations (insert, delete, undo, redo, file I/O)
│   └── main.c         # Winsock HTTP server & REST API (port 8080)
├── frontend/
│   ├── index.html     # Responsive editor layout with live stack counters
│   ├── style.css      # Clean, professional CSS styling
│   └── script.js      # Fetch controller (Zero client-side history)
├── data/
│   └── document.txt   # File-based document storage
├── docs/              # Documentation resources
└── README.md          # Project guide and viva notes
```

---

## 6. Compilation & Execution

### Prerequisites
- Windows OS
- GCC Compiler (MinGW / MSYS2 / WinLibs)

### Step 1: Compile the C Backend
Open a terminal (PowerShell or Command Prompt) in the project root directory and run:
```powershell
gcc -Wall -Wextra backend/main.c backend/stack.c backend/editor.c -o backend.exe -lws2_32
```
*Expected output: Compiles cleanly with 0 errors and 0 warnings.*

### Step 2: Run the Server
```powershell
.\backend.exe
```

### Step 3: Open in Browser
Open your browser and navigate to:
```text
http://localhost:8080/
```

---

## 7. Features

- **Direct In-Browser Editing:** Freely type or edit text directly inside the main textarea and click **Apply Changes** to commit a state.
- **Two-Stack Undo / Redo:** Full historical rollback and rollforward powered entirely by C stacks.
- **Keyboard Shortcuts:** Native web app shortcuts (`Ctrl+Z` to Undo, `Ctrl+Y` / `Ctrl+Shift+Z` to Redo, `Ctrl+S` to Save).
- **Floating Toast Notifications:** Non-blocking, modern toast notifications for operation feedback.
- **Delete Characters:** Safely truncate $N$ characters from the end of the document.
- **Branch Invalidation:** Automatically clears Redo Stack when a new modification occurs after an Undo.
- **Document Clear:** Empties the editor while preserving the state in Undo for accidental-clear recovery.
- **File Persistence:** Save current document to `data/document.txt` and load it back on demand.
- **Live Stack Counters:** Real-time display of Undo Stack size and Redo Stack size.

---

## 8. How File Storage Works

- When the user clicks **Save** (or presses `Ctrl+S`), the browser invokes `POST /api/save`. The C backend opens `data/document.txt` using `fopen(filepath, "w")` and writes the active buffer using `fputs()`.
- When the user clicks **Load**, the browser invokes `POST /api/load`. The C backend reads `data/document.txt` using `fread()`, calls `editor_set_text()` to push the prior text onto the Undo Stack (so loading a file can also be undone), and replaces the current buffer with the file contents.

---

## 9. How to Demonstrate to Teacher

Follow this straightforward 10-step sequence for a flawless viva or practical exam presentation:

1. **Open Terminal:** Open PowerShell or Command Prompt in the `UndoRedoTextEditor/` directory.
2. **Show Clean Compilation:** Run:
   ```powershell
   gcc -Wall -Wextra backend/main.c backend/stack.c backend/editor.c -o backend.exe -lws2_32
   ```
   *Point out that the C code compiles with zero errors and zero warnings.*
3. **Launch Backend:** Run `.\backend.exe` and show the server starting on `http://localhost:8080`.
4. **Open Browser:** Navigate to `http://localhost:8080/`. Show the clean interface, top navigation bar (`C Backend: Online`), and the two stack counters initialized to `0`.
5. **Demonstrate Direct Editing & Apply Changes:**
   - Type `"Hello"` directly into the textarea and click **Apply Changes**.
   - Show the toast message *"Changes applied successfully"* and **Undo Stack = 1**.
   - Add `" World"` into the textarea and click **Apply Changes**.
   - Show the editor displays `"Hello World"` and **Undo Stack count = 2**.
6. **Demonstrate Undo (or press `Ctrl+Z`):**
   - Click **Undo** (or press `Ctrl+Z`). Show that text rolls back to `"Hello"`.
   - Point out that **Undo Stack count = 1** and **Redo Stack count = 1**.
7. **Demonstrate Redo (or press `Ctrl+Y`):**
   - Click **Redo** (or press `Ctrl+Y`). Show that text restores forward to `"Hello World"`.
   - Point out that **Redo Stack count = 0**.
8. **Demonstrate Branch Invalidation (Crucial Viva Point):**
   - Click **Undo** (text reverts to `"Hello"`).
   - Change textarea to `"Hello C"` and click **Apply Changes**.
   - Point out to the teacher that **Redo is now disabled** and **Redo Stack = 0** because a new edit invalidates the forward history branch.
9. **Demonstrate Save & Load (or press `Ctrl+S`):**
   - Click **Save** (or press `Ctrl+S`). Open `data/document.txt` to show `"Hello C"` written directly to disk.
   - Click **Clear** (text becomes empty).
   - Click **Load** (text `"Hello C"` is restored from `data/document.txt`).
10. **Explain C Code:**
    - Open `backend/stack.c` and explain `push()` and `pop()`.
    - Open `backend/editor.c` and explain `editor_undo()` and `editor_redo()`, emphasizing that **JavaScript contains zero stack logic** and all DSA operations run strictly in C.

---

## 10. Limitations & Future Scope

### Limitations
- Fixed buffer sizes (`MAX_TEXT_SIZE = 4096` bytes and `MAX_STACK_SIZE = 100` states).
- Single-client, single-document state per server process.
- Simple sequential insertion/deletion rather than arbitrary cursor-position editing.

### Future Scope
- Dynamic array allocation (`malloc`/`realloc`) or linked-list based stacks for unlimited history depth.
- Gap-buffer or piece-table data structures for advanced cursor-based text insertions and deletions.
- Multi-document tab support.

