#ifndef EDITOR_H
#define EDITOR_H

#include "stack.h"

/*
 * editor.h
 * -------------------------------------------------------------
 * Text Editor state and operations using Two Stacks:
 * 1. Undo Stack: stores past snapshots of the text.
 * 2. Redo Stack: stores undone snapshots for re-application.
 * -------------------------------------------------------------
 */

typedef struct {
    char currentText[MAX_TEXT_SIZE];
    Stack undoStack;
    Stack redoStack;
} Editor;

/*
 * Core Editor Lifecycle and Modification Functions
 */

// Initializes editor with empty text and empty undo/redo stacks
void editor_init(Editor *editor);

// Replaces the current text with new text (saves previous to undo, clears redo)
void editor_set_text(Editor *editor, const char *new_text);

// Appends/inserts text at the end of current text (saves to undo, clears redo)
int editor_insert(Editor *editor, const char *text_to_insert);

// Deletes the last 'count' characters safely (saves to undo, clears redo)
int editor_delete(Editor *editor, int count);

// Clears the document text (saves previous state to undo, clears redo)
void editor_clear(Editor *editor);

/*
 * Core DSA Two-Stack Operations
 */

// Undoes the last operation (moves state from Undo Stack to Redo Stack)
// Returns 1 on success, 0 if Undo Stack is empty.
int editor_undo(Editor *editor);

// Redoes the last undone operation (moves state from Redo Stack to Undo Stack)
// Returns 1 on success, 0 if Redo Stack is empty.
int editor_redo(Editor *editor);

/*
 * File-based Persistence (Document Save & Load)
 */

// Saves currentText to a plain text file. Returns 1 on success, 0 on error.
int editor_save_to_file(const Editor *editor, const char *filepath);

// Loads text from a plain text file into editor. Returns 1 on success, 0 on error.
int editor_load_from_file(Editor *editor, const char *filepath);

/*
 * Helper / State Query Functions
 */

// Returns pointer to the current text buffer
const char* editor_get_text(const Editor *editor);

// Returns 1 if an Undo operation is available, 0 otherwise
int editor_can_undo(const Editor *editor);

// Returns 1 if a Redo operation is available, 0 otherwise
int editor_can_redo(const Editor *editor);

// Returns number of available Undo states
int editor_get_undo_count(const Editor *editor);

// Returns number of available Redo states
int editor_get_redo_count(const Editor *editor);

#endif // EDITOR_H
