#ifndef LEDS_H
#define LEDS_H

#include "pico/stdlib.h"
#include "fsm.h"

void leds_init(void);
void leds_actualizar(const FSM_Context *ctx);
void leds_tick(const FSM_Context *ctx);   // Llamar en el loop para el parpadeo

#endif // LEDS_H
