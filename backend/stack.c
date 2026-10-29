#include "stack.h"
#include <string.h>

/*
 * stack.c
 * -------------------------------------------------------------
 * Manual Stack Implementation in C
 * Follows standard DSA LIFO principles without external libraries.
 * -------------------------------------------------------------
 */

/*
 * Initialize the stack
 * Sets top pointer to -1 indicating that the stack is currently empty.
 */
void initStack(Stack *s) {
    if (s != NULL) {
        s->top = -1;
    }
}

/*
 * Check if the stack is empty
 * Returns 1 (true) if top == -1, otherwise returns 0 (false).
 */
int isEmpty(const Stack *s) {
    if (s == NULL) {
        return 1;
    }
    return (s->top == -1);
}

/*
 * Check if the stack is full
 * Returns 1 (true) if top has reached MAX_STACK_SIZE - 1.
 */
int isFull(const Stack *s) {
    if (s == NULL) {
        return 1;
    }
    return (s->top >= MAX_STACK_SIZE - 1);
}

/*
 * Push a text snapshot onto the stack
 * 1. Checks if stack is full (stack overflow guard).
 * 2. Increments top index.
 * 3. Copies text snapshot into items[top].
 * Returns 1 on success, 0 on failure.
 */
int push(Stack *s, const char *text) {
    if (s == NULL || text == NULL) {
        return 0;
    }
    if (isFull(s)) {
        return 0; // Stack Overflow prevention
    }

    s->top++;
    strncpy(s->items[s->top], text, MAX_TEXT_SIZE - 1);
    s->items[s->top][MAX_TEXT_SIZE - 1] = '\0'; // Guarantee null-termination
    return 1;
}

/*
 * Pop the top text snapshot from the stack
 * 1. Checks if stack is empty (stack underflow guard).
 * 2. Retrieves pointer to items[top].
 * 3. Decrements top index.
 * Returns pointer to the popped string, or NULL if empty.
 */
char* pop(Stack *s) {
    if (s == NULL || isEmpty(s)) {
        return NULL; // Stack Underflow prevention
    }

    char *poppedItem = s->items[s->top];
    s->top--;
    return poppedItem;
}

/*
 * Peek at the top element without removing it
 * Returns pointer to top string, or NULL if stack is empty.
 */
const char* peek(const Stack *s) {
    if (s == NULL || isEmpty(s)) {
        return NULL;
    }
    return s->items[s->top];
}

/*
 * Clear the stack
 * Resets top index to -1 so all previous entries are discarded.
 */
void clear(Stack *s) {
    if (s != NULL) {
        s->top = -1;
    }
}

/*
 * Get the current number of elements on the stack
 */
int getStackSize(const Stack *s) {
    if (s == NULL || s->top < 0) {
        return 0;
    }
    return s->top + 1;
}
