#ifndef STACK_H
#define STACK_H

/*
 * stack.h
 * -------------------------------------------------------------
 * Data Structure: Stack (LIFO - Last In, First Out)
 * Purpose: Stores snapshots of text states for Undo and Redo operations.
 * College DSA Mini Project: Undo-Redo Text Editor using Two Stacks
 * -------------------------------------------------------------
 */

#define MAX_STACK_SIZE 100
#define MAX_TEXT_SIZE  4096

/*
 * Stack structure definition
 * - items: 2D array storing string snapshots of the editor text
 * - top: index of the top element (-1 when empty)
 */
typedef struct {
    char items[MAX_STACK_SIZE][MAX_TEXT_SIZE];
    int top;
} Stack;

/*
 * Core Stack Function Prototypes
 */

// Initializes the stack by setting top = -1
void initStack(Stack *s);

// Checks if the stack has no elements
int isEmpty(const Stack *s);

// Checks if the stack has reached maximum capacity
int isFull(const Stack *s);

// Pushes a new text snapshot onto the stack
int push(Stack *s, const char *text);

// Pops and returns the top text snapshot from the stack
char* pop(Stack *s);

// Inspects the top snapshot without removing it
const char* peek(const Stack *s);

// Clears all elements from the stack by resetting top
void clear(Stack *s);

// Returns current number of elements stored in the stack
int getStackSize(const Stack *s);

#endif // STACK_H
