#ifndef CONFIG_H
#define CONFIG_H

#include "pico/stdlib.h"

// ─── PINES ───────────────────────────────────────────────────────────────────

// Encoder rotativo
#define PIN_ENC_A       3      // Canal A del encoder
#define PIN_ENC_B       4      // Canal B del encoder
#define PIN_ENC_SW      2      // Botón del encoder

// LEDs (PWM)
#define PIN_LED1        6      // LED efecto activo
#define PIN_LED2        7     // LED parámetro 1
#define PIN_LED3        8      // LED parámetro 2
 
// Audio
#define PIN_ADC_AUDIO   26      // GP26 = ADC1 (entrada de señal)
#define PIN_PWM_AUDIO   14       // GP14 = PWM audio (salida)

// ─── TIEMPOS ─────────────────────────────────────────────────────────────────

#define HOLD_MS             5000    // Tiempo de hold para apagar efecto (ms)
#define DEBOUNCE_MS         20      // Debounce del botón (ms)
#define BLINK_PERIOD_MS     500     // Período base de parpadeo LED1

// ─── AUDIO ───────────────────────────────────────────────────────────────────

#define SAMPLE_RATE_HZ      44100
#define PWM_WRAP        2047    // 11 bits (igual que tu config)
#define PWM_CLKDIV      1.0f
#define ADC_RESOLUTION      4096    // 12 bits

// ─── PARÁMETROS ──────────────────────────────────────────────────────────────

#define PARAM_MIN           0
#define PARAM_MAX           255
#define PARAM_STEP          4       // Incremento por paso de encoder

// ─── LEDs PWM ────────────────────────────────────────────────────────────────

#define LED_PWM_WRAP        255
#define LED_TENUE           100      // Brillo en IDLE (efecto activo)
#define LED_BRILLANTE       255     // Brillo máximo en modo EDIT

// ─── EFECTOS ─────────────────────────────────────────────────────────────────

#define NUM_EFECTOS         2       // Delay y Reverb (expandible)

typedef enum {
    EFECTO_DELAY  = 0,
    EFECTO_REVERB = 1,
    // EFECTO_CHORUS = 2,   // Descomentar al agregar
    // EFECTO_TREMOLO = 3,
} EfectoID;

// Patrones de parpadeo LED1 por efecto (cantidad de blinks)
#define BLINK_DELAY         1       // 1 parpadeo = Delay
#define BLINK_REVERB        2       // 2 parpadeos = Reverb

#endif // CONFIG_H
