#ifndef EFECTOS_H
#define EFECTOS_H

#include "pico/stdlib.h"
#include "hardware/timer.h"
#include "fsm.h"

void efectos_init(void);
void efectos_aplicar(const FSM_Context *ctx);
void efectos_audio_tick(const FSM_Context *ctx);

#endif // EFECTOS_H
