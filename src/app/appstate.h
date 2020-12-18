#ifndef APP_STATE
#define APP_STATE

typedef struct AppState_struct AppState;

typedef AppState* (*stateFunc)(double);
typedef void      (*stateFuncVoid)();

typedef struct AppState_struct
{
    stateFuncVoid init;
    stateFunc     update, deinit;
} 
AppState;

inline void appState_init (AppState* state, void* init, void* update, void* deinit)
{
    state->init   = init;
    state->update = update;
    state->deinit = deinit;

    state->init();
};

inline AppState* appState_update (AppState* tempState, double appTime)
{
    return tempState->update(appTime);
};

#endif