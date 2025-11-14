#ifndef SIGNAL_GENERATOR_H
#define SIGNAL_GENERATOR_H

#include <stdint.h>
#include <stddef.h>
#include <math.h> 

// --- KONSTANTEN ---
#define SAMPLE_RATE 22050 
#define SYMRATE 38400
#define MAX_PCM_VALUE 32767 // Amplitude (Signed 16-bit)

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/**
 * @brief Berechnet die Länge der PCM-Übertragung in SAMPLES.
 */
size_t pcmTransmissionLength(
        uint32_t sampleRate,
        uint32_t baudRate,
        size_t transmissionLength);

/**
 * @brief Kodiert die 32-Bit-Wörter in ein rohes PCM-Audiosignal (Signed 16-bit, Little Endian).
 */
void pcmEncodeTransmission(
        uint32_t sampleRate,
        uint32_t baudRate,
        const uint32_t* transmission,
        size_t transmissionLength,
        int16_t* out); // KORREKT: int16_t*

// NEUE HILFSFUNKTION FÜR SINUSWELLEN-GENERIERUNG
/**
 * @brief Generiert ein 16-bit PCM Sample bei einer bestimmten Frequenz und Zeit.
 * * @param frequency Die Frequenz des Tons (Hz).
 * @param time_index Das aktuelle Sample-Index (0, 1, 2, ...).
 * @param sample_rate Die Abtastrate.
 * @return int16_t Das generierte PCM-Sample.
 */
int16_t rs_generate_tone_sample(double frequency, size_t time_index, uint32_t sample_rate);


#endif // SIGNAL_GENERATOR_H