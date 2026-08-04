#ifndef ROTARY_ENCODER_H
#define ROTARY_ENCODER_H

#include <stdbool.h>
#include <stdint.h>

/**
 * @brief Feste Hardware- und Verhaltenseinstellungen.
 */
typedef struct
{
    uint8_t gpio_clk;
    uint8_t gpio_dt;
    uint8_t gpio_sw;

    uint8_t steps_per_detent;
    uint8_t hysteresis;
} rotary_config_t;

/**
 * @brief Von der Anwendung lesbare Encoderwerte.
 */
typedef struct
{
    int32_t position;
    int32_t delta;

    bool button;
    bool pressed;
    bool released;
} rotary_output_t;


/**
 * @brief Interner Zustand des Treibers.
 *
 * Nicht direkt durch die Anwendung verändern.
 */
typedef struct
{
    uint8_t state;

    int8_t accumulator;
    int8_t direction_bias;

    bool switch_state;
} rotary_internal_t;


/**
 * @brief Vollständige Encoderinstanz.
 */
typedef struct
{
    rotary_config_t config;
    rotary_output_t output;
    rotary_internal_t internal;
} rotary_t;


/**
 * @brief Initialisiert eine Encoderinstanz und die zugehörigen GPIOs.
 *
 * @param rotary Encoderinstanz mit ausgefüllter Konfiguration.
 *
 * @return true bei erfolgreicher Initialisierung.
 * @return false bei ungültiger Konfiguration.
 */
bool rotary_init(rotary_t *rotary);


/**
 * @brief Setzt Position, Delta und interne Bewegungserkennung zurück.
 *
 * Die GPIO-Konfiguration bleibt erhalten.
 *
 * @param rotary Encoderinstanz.
 */
void rotary_reset(rotary_t *rotary);


/**
 * @brief Liest und löscht die seit dem letzten Aufruf erfasste Bewegung.
 *
 * @param rotary Encoderinstanz.
 *
 * @return Positive Werte für Rechtsbewegung.
 * @return Negative Werte für Linksbewegung.
 */
int32_t rotary_take_delta(rotary_t *rotary);


/**
 * @brief Liefert die aktuelle absolute Position.
 *
 * @param rotary Encoderinstanz.
 *
 * @return Aktuelle Position in Rastschritten.
 */
int32_t rotary_get_position(const rotary_t *rotary);


/**
 * @brief Liest und löscht das Ereignis "Taster gedrückt".
 *
 * @param rotary Encoderinstanz.
 *
 * @return true, wenn seit dem letzten Aufruf gedrückt wurde.
 */
bool rotary_take_pressed(rotary_t *rotary);


/**
 * @brief Liest und löscht das Ereignis "Taster losgelassen".
 *
 * @param rotary Encoderinstanz.
 *
 * @return true, wenn seit dem letzten Aufruf losgelassen wurde.
 */
bool rotary_take_released(rotary_t *rotary);


/**
 * @brief Liefert den aktuellen logischen Tasterzustand.
 *
 * @param rotary Encoderinstanz.
 *
 * @return true bei gedrücktem Taster.
 */
bool rotary_is_pressed(const rotary_t *rotary);


/**
 * @brief Ändert die Hysterese zur Laufzeit.
 *
 * @param rotary Encoderinstanz.
 * @param hysteresis Neue Hysterese in Quadraturübergängen.
 */
void rotary_set_hysteresis(
    rotary_t *rotary,
    uint8_t hysteresis
);

#endif /* ROTARY_ENCODER_H */
