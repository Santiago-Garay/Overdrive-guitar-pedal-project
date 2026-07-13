#ifndef ENCODER_H
#define ENCODER_H

#include "pico/stdlib.h"
#include "fsm.h"

void encoder_init(void);
FSM_Event encoder_leer(void);   // Llamar en el loop principal

#endif // ENCODER_H
