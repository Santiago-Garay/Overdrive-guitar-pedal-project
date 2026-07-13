#include "encoder.h"
#include "config.h"
#include "hardware/gpio.h"
#include "pico/time.h"
#include <stdio.h>


// ─── ESTADO INTERNO ───────────────────────────────────────────────────────────

static volatile int  enc_delta     = 0;   // Acumulador de pasos (+/-)
static volatile bool btn_presionado = false;

static uint8_t enc_ultimo_estado   = 0;

// Para debounce del botón
static absolute_time_t btn_tiempo_press;
static bool            btn_hold_emitido  = false;
static bool            btn_estaba_abajo  = false;

// ─── ISR ENCODER (canales A y B) ─────────────────────────────────────────────
//
// Tabla de cuadratura de 4 estados:
//   Estado anterior | Estado nuevo | Delta
//        00         |     01       |  +1  (CW)
//        01         |     11       |  +1
//        11         |     10       |  +1
//        10         |     00       |  +1
//        00         |     10       |  -1  (CCW)
//        ...
//
static const int8_t tabla_cuadratura[16] = {
    0, +1, -1,  0,
   -1,  0,  0, +1,
   +1,  0,  0, -1,
    0, -1, +1,  0
};

static void gpio_callback(uint gpio, uint32_t events) {
    if (gpio == PIN_ENC_A || gpio == PIN_ENC_B) {
        uint8_t a = gpio_get(PIN_ENC_A);
        uint8_t b = gpio_get(PIN_ENC_B);
        uint8_t nuevo_estado = (a << 1) | b;
        uint8_t idx = (enc_ultimo_estado << 2) | nuevo_estado;
        enc_delta += tabla_cuadratura[idx];
        enc_ultimo_estado = nuevo_estado;
    }
}

// ─── INIT ─────────────────────────────────────────────────────────────────────

void encoder_init(void) {
    // Canales A y B con pull-up interno
    gpio_init(PIN_ENC_A);
    gpio_set_dir(PIN_ENC_A, GPIO_IN);
    gpio_pull_up(PIN_ENC_A);

    gpio_init(PIN_ENC_B);
    gpio_set_dir(PIN_ENC_B, GPIO_IN);
    gpio_pull_up(PIN_ENC_B);

    // Botón con pull-up interno (activo en LOW)
    gpio_init(PIN_ENC_SW);
    gpio_set_dir(PIN_ENC_SW, GPIO_IN);
    gpio_pull_up(PIN_ENC_SW);

    // ISR para canales A y B (ambos flancos)
    gpio_set_irq_enabled_with_callback(PIN_ENC_A,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true, &gpio_callback);
    gpio_set_irq_enabled(PIN_ENC_B,
        GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    // Estado inicial
    enc_ultimo_estado = (gpio_get(PIN_ENC_A) << 1) | gpio_get(PIN_ENC_B);
    btn_tiempo_press  = get_absolute_time();
}

// ─── LECTURA (llamar en loop principal) ───────────────────────────────────────

FSM_Event encoder_leer(void) {

        // Leer delta atómicamente
    gpio_set_irq_enabled(PIN_ENC_A, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
    gpio_set_irq_enabled(PIN_ENC_B, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, false);
    int delta = enc_delta;
    enc_delta = 0;
    gpio_set_irq_enabled(PIN_ENC_A, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);
    gpio_set_irq_enabled(PIN_ENC_B, GPIO_IRQ_EDGE_RISE | GPIO_IRQ_EDGE_FALL, true);

    // Acumulador persistente entre llamadas
    static int acumulador = 0;
    acumulador += delta;

    if (acumulador >= 1) {
        acumulador = 0;
        return EVENT_ROT_CW;
    }
    if (acumulador <= -1) {
        acumulador = 0;
        return EVENT_ROT_CCW;
    }

    // ── Botón ─────────────────────────────────────────────────────────────────
    bool btn_abajo = !gpio_get(PIN_ENC_SW);  // Activo en LOW

    if (btn_abajo && !btn_estaba_abajo) {
        // Flanco descendente: empieza el press
        btn_tiempo_press  = get_absolute_time();
        btn_hold_emitido  = false;
        btn_estaba_abajo  = true;
    }

    if (btn_abajo && !btn_hold_emitido) {
        // Verificar si ya se cumplieron los 5 segundos
        int64_t elapsed_ms = absolute_time_diff_us(btn_tiempo_press, get_absolute_time()) / 1000;
        if (elapsed_ms >= HOLD_MS) {
            btn_hold_emitido = true;
            return EVENT_HOLD;
        }
    }

    if (!btn_abajo && btn_estaba_abajo) {
        // Flanco ascendente: soltó el botón
        btn_estaba_abajo = false;
        if (!btn_hold_emitido) {
            // Solo emitir CLICK si no se emitió HOLD antes
            int64_t elapsed_ms = absolute_time_diff_us(btn_tiempo_press, get_absolute_time()) / 1000;
            if (elapsed_ms >= DEBOUNCE_MS) {
                return EVENT_CLICK;
            }
        }
    }

    return EVENT_NONE;
}
