#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <math.h>

#include "../../include/signal_generator.h"
#include "../../include/encoders/ufsk1200.h"

#define SAMPLES_PER_BIT (SAMPLE_RATE / UFSK1200_BAUD_RATE)
#define MAX_AMPLITUDE MAX_PCM_VALUE

static double current_phase = 0.0;

/**
 * @brief Sendet ein einzelnes Bit mit FSK-Modulation.
 */
static void emit_bit(int bit) {
    double freq = bit ? UFSK1200_MARK_FREQ : UFSK1200_SPACE_FREQ;
    double phase_step = 2.0 * M_PI * freq / SAMPLE_RATE;
    
    for (int i = 0; i < SAMPLES_PER_BIT; i++) {
        int16_t sample = (int16_t)(MAX_AMPLITUDE * sin(current_phase));
        fwrite(&sample, sizeof(int16_t), 1, stdout);
        current_phase += phase_step;
        if (current_phase >= 2.0 * M_PI) current_phase -= 2.0 * M_PI;
    }
}

/**
 * @brief Sendet ein Byte Bit für Bit (LSB zuerst).
 */
static void emit_byte(uint8_t byte) {
    for (int i = 0; i < 8; i++) {
        emit_bit((byte >> i) & 1);
    }
}

int rs_encode_ufsk1200(const char* message) {
    current_phase = 0.0;
    
    size_t msg_len = strlen(message);
    if (msg_len == 0) return 1;
    
    // Preamble: alternating 0/1 for synchronization (20 bits)
    for (int i = 0; i < 20; i++) {
        emit_bit(i % 2);
    }
    
    // Message bytes
    for (size_t i = 0; i < msg_len; i++) {
        emit_byte((uint8_t)message[i]);
    }
    
    return 0;
}
