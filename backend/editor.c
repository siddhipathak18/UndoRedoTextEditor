#include "editor.h"
#include <stdio.h>
#include <string.h>

/*
 * editor.c
 * -------------------------------------------------------------
 * Implementation of Text Editor Undo/Redo Engine using Two Stacks
 * -------------------------------------------------------------
 */

/*
 * Initialize the text editor
 * - Sets current text buffer to an empty string.
 * - Initializes both the Undo Stack and Redo Stack.
 */
void editor_init(Editor *editor) {
    if (editor == NULL) return;

    editor->currentText[0] = '\0';
    initStack(&editor->undoStack);
    initStack(&editor->redoStack);
}

/*
 * Set / Replace the current text
 * 1. Pushes the current text snapshot to the Undo Stack.
 * 2. Overwrites current text with the new string.
 * 3. Clears the Redo Stack (any new edit invalidates forward history).
 */
void editor_set_text(Editor *editor, const char *new_text) {
    if (editor == NULL || new_text == NULL) return;

    // Save current state to Undo Stack
    push(&editor->undoStack, editor->currentText);

    // Apply new text
    strncpy(editor->currentText, new_text, MAX_TEXT_SIZE - 1);
    editor->currentText[MAX_TEXT_SIZE - 1] = '\0';

    // Clear Redo Stack on any new edit
    clear(&editor->redoStack);
}

/*
 * Insert / Append text to the current buffer
 * 1. Pushes current text snapshot to the Undo Stack.
 * 2. Safely concatenates new text.
 * 3. Clears the Redo Stack.
 * Returns 1 on success, 0 on failure/overflow.
 */
int editor_insert(Editor *editor, const char *text_to_insert) {
    if (editor == NULL || text_to_insert == NULL) return 0;

    size_t currentLen = strlen(editor->currentText);
    size_t insertLen = strlen(text_to_insert);

    // Check if total length exceeds maximum buffer capacity
    if (currentLen + insertLen >= MAX_TEXT_SIZE) {
        return 0; // Text too large
    }

    // Step 1: Save current state to Undo Stack before changing
    push(&editor->undoStack, editor->currentText);

    // Step 2: Append the requested text
    strncat(editor->currentText, text_to_insert, MAX_TEXT_SIZE - currentLen - 1);

    // Step 3: Clear Redo Stack because new action breaks redo tree
    clear(&editor->redoStack);

    return 1;
}

/*
 * Delete the last 'count' characters from the current text
 * 1. Checks if there is text to delete.
 * 2. Pushes current snapshot to the Undo Stack.
 * 3. Truncates the string safely.
 * 4. Clears the Redo Stack.
 * Returns 1 on success, 0 on invalid parameters.
 */
int editor_delete(Editor *editor, int count) {
    if (editor == NULL || count <= 0) return 0;

    size_t currentLen = strlen(editor->currentText);
    if (currentLen == 0) return 0; // Nothing to delete

    // Step 1: Save current state to Undo Stack
    push(&editor->undoStack, editor->currentText);

    // Step 2: Perform deletion safely
    if ((size_t)count >= currentLen) {
        editor->currentText[0] = '\0'; // Delete everything
    } else {
        editor->currentText[currentLen - count] = '\0'; // Truncate last 'count' chars
    }

    // Step 3: Clear Redo Stack
    clear(&editor->redoStack);

    return 1;
}

/*
 * Clear the editor text
 * 1. Saves current snapshot to Undo Stack (so clear can be undone).
 * 2. Empties the current text buffer.
 * 3. Clears the Redo Stack.
 */
void editor_clear(Editor *editor) {
    if (editor == NULL) return;

    // Save current text so user can undo accidental clear
    push(&editor->undoStack, editor->currentText);

    // Empty the text
    editor->currentText[0] = '\0';

    // Clear Redo Stack
    clear(&editor->redoStack);
}

/*
 * Undo Operation
 * Algorithm:
 * 1. Check if Undo Stack is empty. If empty, return 0 (safe exit).
 * 2. Pop previous state from Undo Stack.
 * 3. Copy popped state to local buffer to ensure complete isolation.
 * 4. Push current text onto Redo Stack.
 * 5. Restore popped state into currentText buffer.
 * Returns 1 on success, 0 if nothing to undo.
 */
int editor_undo(Editor *editor) {
    if (editor == NULL || isEmpty(&editor->undoStack)) {
        return 0; // Undo stack is empty: safe no-op
    }

    // 1. Pop previous state from Undo Stack
    char *prevState = pop(&editor->undoStack);
    if (prevState == NULL) {
        return 0;
    }

    // Copy to temporary buffer to prevent any pointer aliasing
    char temp[MAX_TEXT_SIZE];
    strncpy(temp, prevState, MAX_TEXT_SIZE - 1);
    temp[MAX_TEXT_SIZE - 1] = '\0';

    // 2. Save current state to Redo Stack
    push(&editor->redoStack, editor->currentText);

    // 3. Make popped state the current text
    strncpy(editor->currentText, temp, MAX_TEXT_SIZE - 1);
    editor->currentText[MAX_TEXT_SIZE - 1] = '\0';

    return 1;
}

/*
 * Redo Operation
 * Algorithm:
 * 1. Check if Redo Stack is empty. If empty, return 0 (safe exit).
 * 2. Pop next state from Redo Stack.
 * 3. Copy popped state to local buffer to ensure complete isolation.
 * 4. Push current text onto Undo Stack.
 * 5. Restore popped state into currentText buffer.
 * Returns 1 on success, 0 if nothing to redo.
 */
int editor_redo(Editor *editor) {
    if (editor == NULL || isEmpty(&editor->redoStack)) {
        return 0; // Redo stack is empty: safe no-op
    }

    // 1. Pop next state from Redo Stack
    char *nextState = pop(&editor->redoStack);
    if (nextState == NULL) {
        return 0;
    }

    // Copy to temporary buffer to prevent any pointer aliasing
    char temp[MAX_TEXT_SIZE];
    strncpy(temp, nextState, MAX_TEXT_SIZE - 1);
    temp[MAX_TEXT_SIZE - 1] = '\0';

    // 2. Save current state to Undo Stack
    push(&editor->undoStack, editor->currentText);

    // 3. Make popped state the current text
    strncpy(editor->currentText, temp, MAX_TEXT_SIZE - 1);
    editor->currentText[MAX_TEXT_SIZE - 1] = '\0';

    return 1;
}

/*
 * File-based Persistence
 */

int editor_save_to_file(const Editor *editor, const char *filepath) {
    if (editor == NULL || filepath == NULL) return 0;

    FILE *fp = fopen(filepath, "w");
    if (!fp) {
        return 0; // File open error
    }

    fputs(editor->currentText, fp);
    fclose(fp);
    return 1;
}

int editor_load_from_file(Editor *editor, const char *filepath) {
    if (editor == NULL || filepath == NULL) return 0;

    FILE *fp = fopen(filepath, "r");
    if (!fp) {
        return 0; // Missing or inaccessible file
    }

    char buffer[MAX_TEXT_SIZE] = {0};
    size_t bytesRead = fread(buffer, 1, sizeof(buffer) - 1, fp);
    buffer[bytesRead] = '\0';
    fclose(fp);

    // Update editor text (saves previous to undo, clears redo)
    editor_set_text(editor, buffer);
    return 1;
}

/*
 * Helper / Query Functions
 */

const char* editor_get_text(const Editor *editor) {
    if (editor == NULL) return "";
    return editor->currentText;
}

int editor_can_undo(const Editor *editor) {
    if (editor == NULL) return 0;
    return !isEmpty(&editor->undoStack);
}

int editor_can_redo(const Editor *editor) {
    if (editor == NULL) return 0;
    return !isEmpty(&editor->redoStack);
}

int editor_get_undo_count(const Editor *editor) {
    if (editor == NULL) return 0;
    return getStackSize(&editor->undoStack);
}

int editor_get_redo_count(const Editor *editor) {
    if (editor == NULL) return 0;
    return getStackSize(&editor->redoStack);
}
