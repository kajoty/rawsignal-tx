#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <unistd.h>

#include "../../include/signal_generator.h"
#include "../../include/encoders/tones.h"

// DTMF-Frequenzen (Hz) nach ITU-T Q.23
// Niedrige Frequenzen (Reihe)
#define F_LOW_1 697.0
#define F_LOW_2 770.0
#define F_LOW_3 852.0
#define F_LOW_4 941.0

// Hohe Frequenzen (Spalte)
#define F_HIGH_1 1209.0
#define F_HIGH_2 1336.0
#define F_HIGH_3 1477.0
#define F_HIGH_4 1633.0

/**
 * @brief Holt die DTMF-Frequenzen für eine gegebene Ziffer/Zeichen.
 * @param digit Die DTMF-Ziffer (0-9, *, #, A-D).
 * @param f_low Zeiger auf die niedrige Frequenz.
 * @param f_high Zeiger auf die hohe Frequenz.
 * @return 0 bei Erfolg, 1 bei ungültigem Zeichen.
 */
static int get_dtmf_frequencies(char digit, double *f_low, double *f_high) {
    switch (digit) {
        case '1': *f_low = F_LOW_1; *f_high = F_HIGH_1; break;
        case '2': *f_low = F_LOW_1; *f_high = F_HIGH_2; break;
        case '3': *f_low = F_LOW_1; *f_high = F_HIGH_3; break;
        case 'A': case 'a': *f_low = F_LOW_1; *f_high = F_HIGH_4; break;

        case '4': *f_low = F_LOW_2; *f_high = F_HIGH_1; break;
        case '5': *f_low = F_LOW_2; *f_high = F_HIGH_2; break;
        case '6': *f_low = F_LOW_2; *f_high = F_HIGH_3; break;
        case 'B': case 'b': *f_low = F_LOW_2; *f_high = F_HIGH_4; break;

        case '7': *f_low = F_LOW_3; *f_high = F_HIGH_1; break;
        case '8': *f_low = F_LOW_3; *f_high = F_HIGH_2; break;
        case '9': *f_low = F_LOW_3; *f_high = F_HIGH_3; break;
        case 'C': case 'c': *f_low = F_LOW_3; *f_high = F_HIGH_4; break;

        case '*': *f_low = F_LOW_4; *f_high = F_HIGH_1; break;
        case '0': *f_low = F_LOW_4; *f_high = F_HIGH_2; break;
        case '#': *f_low = F_LOW_4; *f_high = F_HIGH_3; break;
        case 'D': case 'd': *f_low = F_LOW_4; *f_high = F_HIGH_4; break;
        
        default:
            return 1; // Unbekanntes Zeichen
    }
    return 0;
}

/**
 * @brief Schreibt ein 16-bit Sample im Little-Endian-Format an stdout.
 */
static void write_sample(int16_t sample) {
    uint8_t output[2];
    // Little-Endian-Format: Low Byte zuerst
    output[0] = (uint8_t)(sample & 0xFF);       
    output[1] = (uint8_t)((sample >> 8) & 0xFF); 
    fwrite(output, 1, 2, stdout);
}

/**
 * @brief Codiert und sendet eine Sequenz von DTMF-Tönen (Dual-Tone Multi-Frequency).
 * * Die Funktion generiert die Audiosignale für DTMF-Ziffern und gibt sie als
 * Raw-Audio-Daten an stdout aus.
 */
int rs_encode_dtmf(const char *digits, int tone_duration_ms, int pause_duration_ms) {
    uint32_t sample_rate = SAMPLE_RATE;
    
    // Berechne die Anzahl der Samples für Ton und Pause
    size_t tone_samples = (size_t)((double)tone_duration_ms * sample_rate / 1000.0);
    size_t pause_samples = (size_t)((double)pause_duration_ms * sample_rate / 1000.0);
    
    // Verfolgt den globalen Zeitindex für die Sinusberechnung
    size_t overall_time_index = 0; 

    for (const char *p = digits; *p != '\0'; p++) {
        char digit = *p;
        double f_low, f_high;

        if (get_dtmf_frequencies(digit, &f_low, &f_high) != 0) {
            fprintf(stderr, "Fehler: Unbekannte DTMF-Ziffer '%c' übersprungen.\n", digit);
            continue; 
        }

        // --- 1. TONGENERIERUNG ---
        for (size_t i = 0; i < tone_samples; i++) {
            
            // Generiere Samples für beide Frequenzen
            int16_t sample_low = rs_generate_tone_sample(f_low, overall_time_index, sample_rate);
            int16_t sample_high = rs_generate_tone_sample(f_high, overall_time_index, sample_rate);
            
            // DTMF: Die Samples werden addiert und auf die Hälfte skaliert, um Clipping zu vermeiden.
            int16_t combined_sample = (int16_t)((sample_low + sample_high) / 2);
            
            write_sample(combined_sample);
            overall_time_index++;
        }

        // --- 2. PAUSENGENERIERUNG ---
        for (size_t i = 0; i < pause_samples; i++) {
            write_sample(0);
            overall_time_index++;
        }
    }
    
    return 0;
}