#include "fsm.h"
#include "leds.h"
#include "efectos.h"
#include <stdio.h>

// ─── HELPERS INTERNOS ─────────────────────────────────────────────────────────

static void ajustar_parametro(uint8_t *param, FSM_Event event) {
    if (event == EVENT_ROT_CW) {
        *param = (*param + PARAM_STEP > PARAM_MAX) ? PARAM_MAX : *param + PARAM_STEP;
    } else if (event == EVENT_ROT_CCW) {
        *param = (*param < PARAM_STEP) ? PARAM_MIN : *param - PARAM_STEP;
    }
}

static void avanzar_efecto(FSM_Context *ctx, FSM_Event event) {
    if (event == EVENT_ROT_CW) {
        ctx->efecto_seleccionado = (ctx->efecto_seleccionado + 1) % NUM_EFECTOS;
    } else if (event == EVENT_ROT_CCW) {
        ctx->efecto_seleccionado = (ctx->efecto_seleccionado == 0)
            ? NUM_EFECTOS - 1
            : ctx->efecto_seleccionado - 1;
    }
}

// ─── INIT ─────────────────────────────────────────────────────────────────────

void fsm_init(FSM_Context *ctx) {
    ctx->state = STATE_IDLE;
    ctx->efecto_seleccionado = EFECTO_DELAY;

    for (int i = 0; i < NUM_EFECTOS; i++) {
        ctx->efecto_activo[i] = true;  // Todos los efectos activos por defecto
        ctx->param1[i] = 128;   // Valor inicial al 50%
        ctx->param2[i] = 64;
    }

    leds_actualizar(ctx);
   // printf("[FSM] Inicializado en IDLE\n");
}

// ─── TRANSICIONES ─────────────────────────────────────────────────────────────

void fsm_process(FSM_Context *ctx, FSM_Event event) {
    if (event == EVENT_NONE) return;

    FSM_State estado_anterior = ctx->state;
    EfectoID  ef = ctx->efecto_seleccionado;

    switch (ctx->state) {

        // ── IDLE ──────────────────────────────────────────────────────────────
        case STATE_IDLE:
            if (event == EVENT_CLICK) {
                ctx->state = STATE_SELECT_EFFECT;
            }
            // Encoder bloqueado: ROT_CW y ROT_CCW se ignoran
            break;

        // ── SELECT_EFFECT ─────────────────────────────────────────────────────
        case STATE_SELECT_EFFECT:
            if (event == EVENT_CLICK) {
                // Activar el efecto seleccionado y pasar a editar param1
                ctx->efecto_activo[ef] = true;
                efectos_aplicar(ctx);
                ctx->state = STATE_EDIT_PARAM1;

            } else if (event == EVENT_HOLD) {
                // Apagar efecto activo y volver a IDLE
                ctx->efecto_activo[ef] = false;
                efectos_aplicar(ctx);
                ctx->state = STATE_IDLE;

            } else if (event == EVENT_ROT_CW || event == EVENT_ROT_CCW) {
               // printf("[FSM] Cambiando efecto seleccionado\n");
                avanzar_efecto(ctx, event);
            }
            break;

        // ── EDIT_PARAM1 ───────────────────────────────────────────────────────
        case STATE_EDIT_PARAM1:
            if (event == EVENT_CLICK) {
                ctx->state = STATE_EDIT_PARAM2;

            } else if (event == EVENT_HOLD) {
                ctx->efecto_activo[ef] = false;
                efectos_aplicar(ctx);
                ctx->state = STATE_IDLE;

            } else if (event == EVENT_ROT_CW || event == EVENT_ROT_CCW) {
               // printf("[FSM] Ajustando parámetro 1 del efecto %d\n", ef);
                ajustar_parametro(&ctx->param1[ef], event);
                efectos_aplicar(ctx);
            }
            break;

        // ── EDIT_PARAM2 ───────────────────────────────────────────────────────
        case STATE_EDIT_PARAM2:
            if (event == EVENT_CLICK) {
                // Ciclo completo: vuelve a IDLE con encoder bloqueado
                ctx->state = STATE_IDLE;

            } else if (event == EVENT_HOLD) {
                ctx->efecto_activo[ef] = false;
                efectos_aplicar(ctx);
                ctx->state = STATE_IDLE;

            } else if (event == EVENT_ROT_CW || event == EVENT_ROT_CCW) {
                ajustar_parametro(&ctx->param2[ef], event);
              //  printf("[FSM] Ajustando parámetro 2 del efecto %d\n", ef);
                efectos_aplicar(ctx);
            }
            break;
    }

    // Actualizar LEDs en cualquier cambio de estado o parámetro
    leds_actualizar(ctx);

   /* if (ctx->state != estado_anterior) {
        printf("[FSM] %s → %s | efecto=%d activo=%d p1=%d p2=%d\n",
            fsm_state_name(estado_anterior),
            fsm_state_name(ctx->state),
            ef,
            ctx->efecto_activo[ef],
            ctx->param1[ef],
            ctx->param2[ef]
        );
    }*/
}

// ─── DEBUG ────────────────────────────────────────────────────────────────────

const char* fsm_state_name(FSM_State state) {
    switch (state) {
        case STATE_IDLE:          return "IDLE";
        case STATE_SELECT_EFFECT: return "SELECT_EFFECT";
        case STATE_EDIT_PARAM1:   return "EDIT_PARAM1";
        case STATE_EDIT_PARAM2:   return "EDIT_PARAM2";
        default:                  return "DESCONOCIDO";
    }
}
