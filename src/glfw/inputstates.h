#ifndef INPUTSTATES_H
#define INPUTSTATES_H

#include <stdint.h>

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

uint32_t input_key     (KeyboardState * const state, uint32_t key);
uint32_t input_new_key (KeyboardState * const state, KeyboardState * const lastState, uint32_t key);
uint32_t input_mouse   (MouseState    * const state, uint32_t button);

#endif