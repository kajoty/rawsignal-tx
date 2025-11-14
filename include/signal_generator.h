#ifndef SIGNAL_GENERATOR_H
#define SIGNAL_GENERATOR_H

#include <stdint.h>
#include <stddef.h>

// FSK-Modulationskonstanten
#define SAMPLE_RATE 22050
#define SYMRATE 38400
// Maximale Amplitude für Signed 16-bit PCM.
// Wir verwenden hier 32767/2, um die Clipping-Gefahr zu minimieren.
#define MAX_PCM_VALUE 16383

/**
 * @brief Berechnet die Länge der PCM-Übertragung in Bytes.
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
        uint8_t* out);

#endif // SIGNAL_GENERATOR_H