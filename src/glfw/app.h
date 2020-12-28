#include "../gl/graphics.h"
#include "timer.h"
#include "callbacks.h"
#include "inputstates.h"

typedef void (*appEventPtr)();

typedef struct App_struct
{
    /* Application commands and game inputs to be mapped by API */
    struct appInputs 
    {
        uint16_t APP_OPEN_FILE,
            APP_MAXIMIZE,
            APP_EXIT,
            EMULATION_TOGGLE,
            EMULATION_STEP,
            EMULATION_SCANLINE,
            EMULATION_DEBUG,
            EMULATION_RESET,
            BUTTON_A,
            BUTTON_B,
            BUTTON_SELECT,
            BUTTON_START,
            BUTTON_UP,
            BUTTON_DOWN,
            BUTTON_LEFT,
            BUTTON_RIGHT;
    }
    inputs;

    /* Default event handlers */
    appEventPtr onScroll, onDrop, onWindowResize;

    /* Handles */
    GLFWwindow * window;
    char       * dropPath; 
    const char * title;

    /* Resolution (X * Y) */
    uint16_t resolution[2];

    /* Internal state */
    MouseState    mouseState,    lastMouseState;
    KeyboardState keyboardState, lastKeyboardState;
    uint16_t running;
    uint8_t  emulationRun;
    uint8_t  ppuDebug;

    /* App assets */
    Scene scene;
    Timer timer;
}
App;

/* Callback wrappers */

void app_toggle_maximize      (App * const);
void app_capture_mouse_scroll (App *, double xOffset, double yOffset);
void app_capture_drop         (App *, char * paths[]);
void app_open_dialog();

void app_init         (App *);
void app_free         (App *);

void app_init_inputs  (App *);
void app_handle_input (App *);
void app_update       (App *);
void app_draw         (App *);

uint8_t app_controller_state (KeyboardState * const, struct appInputs * const);