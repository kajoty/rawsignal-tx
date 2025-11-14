#include <stdio.h> 
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include "../include/signal_generator.h"

// Hinweis: Die Konstanten wie SYMRATE, SAMPLE_RATE und MAX_PCM_VALUE kommen aus signal_generator.h

/**
 * @brief Berechnet die Länge der PCM-Übertragung in Bytes.
 */
size_t pcmTransmissionLength(
        uint32_t sampleRate,
        uint32_t baudRate,
        size_t transmissionLength) {
    // 2 Bytes pro Sample (Int16)
    return transmissionLength * 32 * sampleRate / baudRate * 2;
}

/**
 * @brief Kodiert die 32-Bit-Wörter in ein rohes PCM-Audiosignal (Signed 16-bit, Little Endian).
 * * Diese Funktion implementiert die Rechteckwellen-FSK-Simulation, indem sie Bits 
 * durch positive oder negative Amplituden bei einer hohen Zwischenabtastrate (SYMRATE) darstellt 
 * und anschließend auf die Ziel-Sample-Rate (SAMPLE_RATE) resampelt.
 */
void pcmEncodeTransmission(
        uint32_t sampleRate,
        uint32_t baudRate,
        const uint32_t* transmission,
        size_t transmissionLength,
        uint8_t* out) {

    // Die Anzahl der Wiederholungen jedes Bits, die wir benötigen, um SYMRATE (38400 Hz) zu erreichen
    int repeatsPerBit = SYMRATE / baudRate;

    // Berechnung der Größe des Zwischenpuffers bei SYMRATE
    size_t inputSize = transmissionLength * 32 * repeatsPerBit;

    // Zwischenpuffer für Samples bei SYMRATE (16-Bit Signed)
    int16_t* samples =
        (int16_t*) malloc(sizeof(int16_t) * inputSize);

    if (samples == NULL) {
        fprintf(stderr, "Fehler: Speicherzuweisung für Zwischen-PCM-Puffer fehlgeschlagen.\n");
        return;
    }

    int16_t* psamples = samples;
    for (size_t i = 0; i < transmissionLength; i++) {
        uint32_t val = *(transmission + i);
        
        // POCSAG sendet Bits MSB zuerst für die Kodierung der Wörter.
        for (int bitNum = 0; bitNum < 32; bitNum++) {

            // Kodierung vom höchstwertigen zum niedrigstwertigen Bit
            int bit = (val >> (31 - bitNum)) & 1;
            int16_t sample;
            
            // Rechteckwelle FSK-Simulation:
            // Bit 0 -> Positive Amplitude (Low Frequenz)
            // Bit 1 -> Negative Amplitude (High Frequenz)
            if (bit == 0) {
                sample = MAX_PCM_VALUE; 
            } else {
                sample = -MAX_PCM_VALUE;
            }

            // Wiederhole so oft, wie für die aktuelle Baudrate nötig
            for (int r = 0; r < repeatsPerBit; r++) {
                *psamples = sample;
                psamples++;
            }
        }
    }

    // Resampling auf die Ziel-Abtastrate (22050 Hz) mit Nearest Neighbor
    size_t outputSize =
        pcmTransmissionLength(sampleRate, baudRate, transmissionLength);

    for (size_t i = 0; i < outputSize; i += 2) {
        // Index im 16-Bit-Array entspricht i / 2
        // Runde auf den nächsten Index in den Eingabedaten.
        size_t input_index = (i / 2) * SYMRATE / sampleRate;

        if (input_index < inputSize) {
             int16_t inSample = *(samples + input_index);
            
             // Schreibe im Little-Endian-Format (low byte first)
             *(out + i + 0) = (inSample & 0xFF);
             *(out + i + 1) = ((inSample >> 8) & 0xFF);
        } else {
             // Null-Samples (Ende des Puffers)
             *(out + i + 0) = 0;
             *(out + i + 1) = 0;
        }
    }

    free(samples);
}