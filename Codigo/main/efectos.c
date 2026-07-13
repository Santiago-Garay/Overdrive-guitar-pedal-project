#include "efectos.h"
#include "config.h"
#include "hardware/adc.h"
#include "hardware/pwm.h"
#include <math.h>

// ─── FILTRO SUAVIZADO ADC (igual que tu ALPHA) ────────────────────────────────

#define ALPHA 0.3f
static float filtered_adc = 0.0f;

// ─── DELAY ────────────────────────────────────────────────────────────────────

#define DELAY_BUF_SIZE  (SAMPLE_RATE_HZ / 2)   // 500ms máximo

static float   delay_buffer[DELAY_BUF_SIZE];
static uint32_t delay_write_idx = 0;

static float delay_feedback = 0.2f;
static float delay_mix      = 0.6f;
static uint16_t delay_samples_actual = DELAY_BUF_SIZE;

static inline float apply_delay(float input) {
    float delayed = delay_buffer[delay_write_idx];

    delay_buffer[delay_write_idx] = input * (1.0f - delay_feedback)
                                  + delayed * delay_feedback;

    delay_write_idx++;
    if (delay_write_idx >= delay_samples_actual) delay_write_idx = 0;

    return (1.0f - delay_mix) * input + delay_mix * delayed;
}

// ─── REVERB SCHROEDER (3 combs + 1 allpass) ──────────────────────────────────

#define COMB1_SIZE   (1500 * 5)
#define COMB2_SIZE   (1700 * 7)
#define COMB3_SIZE   (2000 * 10)
#define ALLPASS1_SIZE (300 * 1)

static float comb1_buf[COMB1_SIZE];
static float comb2_buf[COMB2_SIZE];
static float comb3_buf[COMB3_SIZE];
static float allpass1_buf[ALLPASS1_SIZE];

static uint32_t comb1_idx = 0;
static uint32_t comb2_idx = 0;
static uint32_t comb3_idx = 0;
static uint32_t allpass1_idx = 0;

static float comb_feedback    = 0.5f;
static float allpass_feedback = 0.5f;
static float reverb_mix       = 0.6f;

static inline float apply_reverb(float input) {
    // Comb filters en paralelo
    float c1 = comb1_buf[comb1_idx];
    float c2 = comb2_buf[comb2_idx];
    float c3 = comb3_buf[comb3_idx];

    comb1_buf[comb1_idx] = input + c1 * comb_feedback;
    comb2_buf[comb2_idx] = input + c2 * comb_feedback;
    comb3_buf[comb3_idx] = input + c3 * comb_feedback;

    if (++comb1_idx >= COMB1_SIZE)  comb1_idx = 0;
    if (++comb2_idx >= COMB2_SIZE)  comb2_idx = 0;
    if (++comb3_idx >= COMB3_SIZE)  comb3_idx = 0;

    float comb_sum = (c1 + c2 + c3) * 0.3333f;

    // Allpass
    float ap_delay = allpass1_buf[allpass1_idx];
    float ap_out   = -input + ap_delay;
    allpass1_buf[allpass1_idx] = input + ap_delay * allpass_feedback;
    if (++allpass1_idx >= ALLPASS1_SIZE) allpass1_idx = 0;

    float wet = comb_sum + ap_out * 0.5f;
    return (1.0f - reverb_mix) * input + reverb_mix * wet;
}

// ─── PARÁMETROS CACHEADOS DEL CONTEXTO ───────────────────────────────────────

static struct {
    bool    delay_activo;
    bool    reverb_activo;
    uint8_t delay_tiempo;    // 0-255 → longitud del buffer
    uint8_t delay_feedback_raw;
    uint8_t reverb_room;
    uint8_t reverb_mix_raw;
} params;

// ─── TIMER FLAG (igual que tu sample_and_output) ─────────────────────────────

static volatile bool    flag_sample = false;
static repeating_timer_t sample_timer;

static bool sample_callback(repeating_timer_t *t) {
    flag_sample = true;
    return true;
}

// ─── INIT ─────────────────────────────────────────────────────────────────────

void efectos_init(void) {
    // ADC
    adc_init();
    adc_gpio_init(PIN_ADC_AUDIO);

    // PWM audio — igual que tu configuración
    gpio_set_function(PIN_PWM_AUDIO, GPIO_FUNC_PWM);
    uint slice = pwm_gpio_to_slice_num(PIN_PWM_AUDIO);
    pwm_config cfg = pwm_get_default_config();
    pwm_config_set_wrap(&cfg, PWM_WRAP);
    pwm_config_set_clkdiv(&cfg, PWM_CLKDIV);
    pwm_init(slice, &cfg, true);
    pwm_set_enabled(slice, true);

    // Timer de muestreo exacto
    add_repeating_timer_us(
        -(int32_t)(1000000 / SAMPLE_RATE_HZ),
        sample_callback,
        NULL,
        &sample_timer
    );

    // Limpiar buffers
    for (int i = 0; i < DELAY_BUF_SIZE;   i++) delay_buffer[i]   = 0.0f;
    for (int i = 0; i < COMB1_SIZE;       i++) comb1_buf[i]       = 0.0f;
    for (int i = 0; i < COMB2_SIZE;       i++) comb2_buf[i]       = 0.0f;
    for (int i = 0; i < COMB3_SIZE;       i++) comb3_buf[i]       = 0.0f;
    for (int i = 0; i < ALLPASS1_SIZE;    i++) allpass1_buf[i]     = 0.0f;
}

// ─── APLICAR PARÁMETROS DEL CONTEXTO ─────────────────────────────────────────

void efectos_aplicar(const FSM_Context *ctx) {
    params.delay_activo       = ctx->efecto_activo[EFECTO_DELAY];
    params.reverb_activo      = ctx->efecto_activo[EFECTO_REVERB];
    params.delay_tiempo       = ctx->param1[EFECTO_DELAY];
    params.delay_feedback_raw = ctx->param2[EFECTO_DELAY];
    params.reverb_room        = ctx->param1[EFECTO_REVERB];
    params.reverb_mix_raw     = ctx->param2[EFECTO_REVERB];

    // Mapear param1 del delay (0-255) a cantidad de muestras (100 a BUF_SIZE)
    delay_samples_actual = 100 + ((uint32_t)params.delay_tiempo
                         * (DELAY_BUF_SIZE - 100)) / 255;

    // Mapear param2 del delay (0-255) a feedback (0.0 a 0.90)
    delay_feedback = (params.delay_feedback_raw / 255.0f) * 0.90f;

    // Mapear param1 del reverb (0-255) a comb_feedback (0.3 a 0.85)
    comb_feedback = 0.3f + (params.reverb_room / 255.0f) * 0.55f;

    // Mapear param2 del reverb (0-255) a reverb_mix (0.0 a 1.0)
    reverb_mix = params.reverb_mix_raw / 255.0f;
}

// ─── AUDIO TICK (llamar en el loop principal) ─────────────────────────────────

void efectos_audio_tick(const FSM_Context *ctx) {
    if (!flag_sample) return;
    flag_sample = false;

    // Leer ADC y suavizar (tu filtro ALPHA)
    adc_select_input(0);
    uint16_t adc_in = adc_read();
    filtered_adc = filtered_adc + ALPHA * ((float)adc_in - filtered_adc);
    float x = filtered_adc;

    // Aplicar cadena de efectos activos
    float y = x;
    if (params.reverb_activo) y = apply_reverb(y);
    if (params.delay_activo)  y = apply_delay(y);

    // Salida PWM (>> 1 para ajustar a PWM_WRAP = 2047)
    uint16_t pwm_out = (uint16_t)y >> 1;
    uint slice = pwm_gpio_to_slice_num(PIN_PWM_AUDIO);
    pwm_set_chan_level(slice, PWM_CHAN_A, pwm_out);
}
