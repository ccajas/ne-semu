#include "inputstates.h"

uint32_t input_key (KeyboardState * const state, uint32_t key)
{
    for (int j = 0; j < MAX_KEYS; j++)
    {
        if (state->keys[j] == key) return 1;
    }
    return 0;
}

uint32_t input_new_key (KeyboardState * const state, KeyboardState * const lastState, uint32_t key)
{
    for (int j = 0; j < MAX_KEYS; j++)
    {
        if (lastState->keys[j] == 0 &&
            state->keys[j] == key) {
            return 1;
        }
    }
    return 0;
}

uint32_t input_mouse (MouseState * const state, uint32_t button)
{
    return (state->buttonMask >> button) & 1;
}