#ifndef FSM_H
#define FSM_H

#include "pico/stdlib.h"
#include "config.h"

// ─── ESTADOS ─────────────────────────────────────────────────────────────────

typedef enum {
    STATE_IDLE,             // Estado raíz: encoder bloqueado, efectos corren
    STATE_SELECT_EFFECT,    // Rotar cambia efecto seleccionado
    STATE_EDIT_PARAM1,      // Rotar ajusta parámetro 1 del efecto activo
    STATE_EDIT_PARAM2,      // Rotar ajusta parámetro 2 del efecto activo
} FSM_State;

// ─── EVENTOS ─────────────────────────────────────────────────────────────────

typedef enum {
    EVENT_NONE,
    EVENT_CLICK,            // Click corto del encoder
    EVENT_HOLD,             // Hold >= HOLD_MS
    EVENT_ROT_CW,           // Rotación horaria
    EVENT_ROT_CCW,          // Rotación antihoraria
} FSM_Event;

// ─── CONTEXTO GLOBAL ─────────────────────────────────────────────────────────

typedef struct {
    FSM_State   state;
    EfectoID    efecto_seleccionado;
    bool        efecto_activo[NUM_EFECTOS];  // Cada efecto tiene su propio on/off
    uint8_t     param1[NUM_EFECTOS];         // Param1 por efecto (ej: tiempo delay)
    uint8_t     param2[NUM_EFECTOS];         // Param2 por efecto (ej: feedback)
} FSM_Context;

// ─── API ─────────────────────────────────────────────────────────────────────

void fsm_init(FSM_Context *ctx);
void fsm_process(FSM_Context *ctx, FSM_Event event);
const char* fsm_state_name(FSM_State state);  // Para debug por USB serial

#endif // FSM_H
