#ifndef INPUTSTATES_H
#define INPUTSTATES_H

#define MAX_KEYS 8

typedef struct MouseState_struct
{
    uint8_t buttonMask;
    int16_t x, y;
    float scrollX, scrollY;
}
MouseState;

typedef struct KeyboardState_struct
{
    uint16_t keys[MAX_KEYS];
    uint16_t keysPressed;
}
KeyboardState;

inline int32_t input_key (KeyboardState * const state, uint32_t key)
{
    for (int j = 0; j < MAX_KEYS; j++)
    {
        if (state->keys[j] == key) {
            return 1;
        }
    }
    return 0;
}

inline int32_t input_new_key (KeyboardState * const state, KeyboardState * const lastState, uint32_t key)
{
    for (int j = 0; j < MAX_KEYS; j++)
    {
        if (lastState->keys[j] == GLFW_RELEASE &&
            state->keys[j] == key) {
            return 1;
        }
    }
    return 0;
}

inline int32_t input_mouse (MouseState * const state, uint32_t button)
{
    return (state->buttonMask >> button) & 1;
}

#endif