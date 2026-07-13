#include "leds.h"
#include "config.h"
#include "hardware/pwm.h"
#include "hardware/gpio.h"
#include "pico/time.h"

// ─── ESTADO INTERNO DE PARPADEO ───────────────────────────────────────────────

// El parpadeo de LED1 en SELECT_EFFECT sigue este esquema:
//   - Se emiten N blinks rápidos (N = número de efecto)
//   - Luego pausa larga
//   - Se repite
//
// Ejemplo EFECTO_DELAY  (1 blink): [ON][OFF][pausa larga]
// Ejemplo EFECTO_REVERB (2 blinks): [ON][OFF][ON][OFF][pausa larga]

#define BLINK_ON_MS     300
#define BLINK_OFF_MS    150
#define BLINK_PAUSA_MS  800

typedef struct {
    uint8_t  blinks_total;      // Cuántos blinks para el efecto actual
    uint8_t  blink_actual;      // En qué blink estamos
    bool     led_encendido;     // Estado actual del LED
    absolute_time_t proximo_cambio;
} BlinkState;

static BlinkState blink = {0};

// ─── HELPER PWM ───────────────────────────────────────────────────────────────

static void led_set_brillo(uint pin, uint8_t brillo) {
    uint slice = pwm_gpio_to_slice_num(pin);
    uint chan  = pwm_gpio_to_channel(pin);
    pwm_set_chan_level(slice, chan, brillo);
}

static void led_init_pin(uint pin) {
    gpio_set_function(pin, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(pin);
    pwm_set_wrap(slice, LED_PWM_WRAP);
    pwm_set_enabled(slice, true);
    led_set_brillo(pin, 0);
}

// ─── INIT ─────────────────────────────────────────────────────────────────────

void leds_init(void) {
    led_init_pin(PIN_LED1);
    led_init_pin(PIN_LED2);
    led_init_pin(PIN_LED3);
}

// ─── ACTUALIZAR (en cada transición de estado) ────────────────────────────────

void leds_actualizar(const FSM_Context *ctx) {
    EfectoID ef = ctx->efecto_seleccionado;

    switch (ctx->state) {

        case STATE_IDLE:
            // LED1 tenue si el efecto está activo, apagado si no
            led_set_brillo(PIN_LED1, ctx->efecto_activo[ef] ? LED_TENUE : 0);
            led_set_brillo(PIN_LED2, 0);
            led_set_brillo(PIN_LED3, 0);
            break;

        case STATE_SELECT_EFFECT:
            // LED1 lo maneja leds_tick() con el parpadeo
            // Inicializar estado de parpadeo según el efecto
            blink.blinks_total  = (ef == EFECTO_DELAY) ? BLINK_DELAY : BLINK_REVERB;
            blink.blink_actual  = 0;
            blink.led_encendido = true;
            blink.proximo_cambio = make_timeout_time_ms(BLINK_ON_MS);
            led_set_brillo(PIN_LED1, LED_BRILLANTE);
            led_set_brillo(PIN_LED2, 0);
            led_set_brillo(PIN_LED3, 0);
            break;

        case STATE_EDIT_PARAM1:
            led_set_brillo(PIN_LED1, LED_BRILLANTE);
            led_set_brillo(PIN_LED2, ctx->param1[ef]);  // Brillo = valor del parámetro
            led_set_brillo(PIN_LED3, 0);
            break;

        case STATE_EDIT_PARAM2:
            led_set_brillo(PIN_LED1, LED_BRILLANTE);
            led_set_brillo(PIN_LED2, 0);
            led_set_brillo(PIN_LED3, ctx->param2[ef]);  // Brillo = valor del parámetro
            break;
    }
}

// ─── TICK (llamar en el loop para mantener el parpadeo vivo) ─────────────────

void leds_tick(const FSM_Context *ctx) {
    if (ctx->state != STATE_SELECT_EFFECT) return;
    if (!time_reached(blink.proximo_cambio)) return;

    if (blink.led_encendido) {
        // Apagar LED y avanzar contador de blinks
        led_set_brillo(PIN_LED1, 0);
        blink.led_encendido = false;
        blink.blink_actual++;

        if (blink.blink_actual >= blink.blinks_total) {
            // Todos los blinks emitidos: pausa larga antes de reiniciar
            blink.proximo_cambio = make_timeout_time_ms(BLINK_PAUSA_MS);
        } else {
            // Hay más blinks: pausa corta entre blinks
            blink.proximo_cambio = make_timeout_time_ms(BLINK_OFF_MS);
        }
    } else {
        if (blink.blink_actual >= blink.blinks_total) {
            // Fin de pausa larga: reiniciar secuencia
            blink.blink_actual = 0;
        }
        // Encender LED para el próximo blink
        led_set_brillo(PIN_LED1, LED_BRILLANTE);
        blink.led_encendido  = true;
        blink.proximo_cambio = make_timeout_time_ms(BLINK_ON_MS);
    }
}
