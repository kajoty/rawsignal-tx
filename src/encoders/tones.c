#include "../../include/encoders/tones.h"
#include "../../include/signal_generator.h"
#include <string.h>
#include <ctype.h>
#include <stdio.h>
#include <unistd.h> // Fuer usleep
#include <stddef.h> // Fuer size_t

// Interne Funktion zum Abrufen der Frequenzen für eine DTMF-Ziffer
static int rs_get_dtmf_freqs(char digit, double *freq_low, double *freq_high) {
    digit = toupper(digit);
    
    // DTMF Frequenztabelle (Low Group / High Group)
    if (digit == '1') { *freq_low = 697.0; *freq_high = 1209.0; }
    else if (digit == '2') { *freq_low = 697.0; *freq_high = 1336.0; }
    else if (digit == '3') { *freq_low = 697.0; *freq_high = 1477.0; }
    else if (digit == 'A') { *freq_low = 697.0; *freq_high = 1633.0; }
    
    else if (digit == '4') { *freq_low = 770.0; *freq_high = 1209.0; }
    else if (digit == '5') { *freq_low = 770.0; *freq_high = 1336.0; }
    else if (digit == '6') { *freq_low = 770.0; *freq_high = 1477.0; }
    else if (digit == 'B') { *freq_low = 770.0; *freq_high = 1633.0; }
    
    else if (digit == '7') { *freq_low = 852.0; *freq_high = 1209.0; }
    else if (digit == '8') { *freq_low = 852.0; *freq_high = 1336.0; }
    else if (digit == '9') { *freq_low = 852.0; *freq_high = 1477.0; }
    else if (digit == 'C') { *freq_low = 852.0; *freq_high = 1633.0; }

    else if (digit == '*') { *freq_low = 941.0; *freq_high = 1209.0; }
    else if (digit == '0') { *freq_low = 941.0; *freq_high = 1336.0; }
    else if (digit == '#') { *freq_low = 941.0; *freq_high = 1477.0; }
    else if (digit == 'D') { *freq_low = 941.0; *freq_high = 1633.0; }
    else {
        return -1; // Unbekannte Ziffer
    }
    
    return 0;
}


/**
 * @brief Codiert und sendet eine Sequenz von DTMF-Tönen.
 */
int rs_encode_dtmf(const char *digits, int tone_duration_ms, int pause_duration_ms) {
    double freq_low = 0.0;
    double freq_high = 0.0;
    double tone_duration_s = (double)tone_duration_ms / 1000.0;
    
    // Warning-Fix: size_t fuer i (Laufvariable) und strlen (Rueckgabewert)
    for (size_t i = 0; i < strlen(digits); i++) {
        char digit = digits[i];
        
        if (rs_get_dtmf_freqs(digit, &freq_low, &freq_high) != 0) {
            fprintf(stderr, "DTMF-Encoder: Unbekannte Ziffer '%c' übersprungen.\n", digit);
            continue;
        }

        fprintf(stderr, "Sende DTMF-Ziffer: %c (%.1fHz + %.1fHz)\n", digit, freq_low, freq_high);

        // HINWEIS: Hier senden wir nur den High-Ton, um die Pipeline zu testen.
        // FÜR KORREKTES DTMF MUSS rs_generate_dual_tone() verwendet werden.
        rs_generate_tone(freq_high, tone_duration_s); 
        
        // Pause zwischen den Tönen
        usleep(pause_duration_ms * 1000); // usleep erwartet Mikrosekunden
    }
    
    return 0;
}