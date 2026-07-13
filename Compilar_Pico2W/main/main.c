#include "pico/stdlib.h"
#include "hardware/gpio.h"
#include <stdio.h>

#include "config.h"
#include "fsm.h"
#include "encoder.h"
#include "leds.h"
#include "efectos.h"

int main(void) {
    stdio_init_all();   // UART/USB para debug

    // ── Inicialización de módulos ─────────────────────────────────────────────
    efectos_init();
    leds_init();
    encoder_init();

    // ── Contexto de la FSM ────────────────────────────────────────────────────
    FSM_Context ctx;
    fsm_init(&ctx);

    printf("[PEDAL] Sistema listo\n");

    // ── Loop principal ────────────────────────────────────────────────────────
    //
    // El loop hace tres cosas en orden:
    //   1. Leer encoder y procesar evento en la FSM
    //   2. Actualizar parpadeo del LED1 (no bloqueante)
    //   3. Leer ADC, procesar audio, escribir PWM
    //
    // La latencia de audio depende de cuán rápido corra este loop.
    // Sin delay artificial, la Pico lo ejecuta en microsegundos.
    // Si necesitás sample rate exacto, reemplazá efectos_audio_tick()
    // con un repeating_timer o DMA.
 
    while (true) {
        // 1. Encoder → FSM
        FSM_Event evento = encoder_leer(); 
        fsm_process(&ctx, evento);

        // 2. Parpadeo LED1 (no bloqueante, basado en tiempo)
        leds_tick(&ctx);

        // 3. Audio
        efectos_audio_tick(&ctx);
    }

    return 0;  // Nunca se llega acá
}
