#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <math.h>

#include "../../include/signal_generator.h"
#include "../../include/encoders/morse.h"

// Frequenz des Tones (Hz) für Morsecode (Standard: 700 Hz)
#define MORSE_TONE_FREQ 700.0
// Der Zähler (1.2) basiert auf dem standardisierten Wort "PARIS" (50 Dits)
#define DOT_TIME_SECONDS_NUMERATOR 1.2

// Struktur zur Darstellung des Morsecodes (DIT = false, DAH = true)
// Max. 6 Elemente, da dies die längsten Codes abdeckt (z.B. '?')
static const MorseCode morse_codes[] = {
    // Alphabet A-Z (Groß-/Kleinschreibung wird ignoriert)
    // A: .-
    [0] = { .length = 2, .elements = { false, true } }, 
    // B: -...
    [1] = { .length = 4, .elements = { true, false, false, false } },
    // C: -.-.
    [2] = { .length = 4, .elements = { true, false, true, false } },
    // D: -..
    [3] = { .length = 3, .elements = { true, false, false } },
    // E: .
    [4] = { .length = 1, .elements = { false } },
    // F: ..-.
    [5] = { .length = 4, .elements = { false, false, true, false } },
    // G: --.
    [6] = { .length = 3, .elements = { true, true, false } },
    // H: ....
    [7] = { .length = 4, .elements = { false, false, false, false } },
    // I: ..
    [8] = { .length = 2, .elements = { false, false } },
    // J: .---
    [9] = { .length = 4, .elements = { false, true, true, true } },
    // K: -.-
    [10] = { .length = 3, .elements = { true, false, true } },
    // L: .-..
    [11] = { .length = 4, .elements = { false, true, false, false } },
    // M: --
    [12] = { .length = 2, .elements = { true, true } },
    // N: -.
    [13] = { .length = 2, .elements = { true, false } },
    // O: ---
    [14] = { .length = 3, .elements = { true, true, true } },
    // P: .--.
    [15] = { .length = 4, .elements = { false, true, true, false } },
    // Q: --.-
    [16] = { .length = 4, .elements = { true, true, false, true } },
    // R: .-.
    [17] = { .length = 3, .elements = { false, true, false } },
    // S: ...
    [18] = { .length = 3, .elements = { false, false, false } },
    // T: -
    [19] = { .length = 1, .elements = { true } },
    // U: ..-
    [20] = { .length = 3, .elements = { false, false, true } },
    // V: ...-
    [21] = { .length = 4, .elements = { false, false, false, true } },
    // W: .--
    [22] = { .length = 3, .elements = { false, true, true } },
    // X: -..-
    [23] = { .length = 4, .elements = { true, false, false, true } },
    // Y: -.--
    [24] = { .length = 4, .elements = { true, false, true, true } },
    // Z: --..
    [25] = { .length = 4, .elements = { true, true, false, false } },

    // Ziffern 0-9 (ASCII-Code 48-57)
    // 0: ----- (Index 26)
    [26] = { .length = 5, .elements = { true, true, true, true, true } },
    // 1: .----
    [27] = { .length = 5, .elements = { false, true, true, true, true } },
    // 2: ..---
    [28] = { .length = 5, .elements = { false, false, true, true, true } },
    // 3: ...--
    [29] = { .length = 5, .elements = { false, false, false, true, true } },
    // 4: ....-
    [30] = { .length = 5, .elements = { false, false, false, false, true } },
    // 5: .....
    [31] = { .length = 5, .elements = { false, false, false, false, false } },
    // 6: -....
    [32] = { .length = 5, .elements = { true, false, false, false, false } },
    // 7: --...
    [33] = { .length = 5, .elements = { true, true, false, false, false } },
    // 8: ---..
    [34] = { .length = 5, .elements = { true, true, true, false, false } },
    // 9: ----. (Index 35)
    [35] = { .length = 5, .elements = { true, true, true, true, false } },

    // Sonderzeichen (Index 36-39)
    // Leerzeichen: wird durch die Wortpause (7 Dits) implementiert (kein Code nötig)
    // Punkt (.): .-.-.- (Index 36)
    [36] = { .length = 6, .elements = { false, true, false, true, false, true } },
    // Komma (,): --..-- (Index 37)
    [37] = { .length = 6, .elements = { true, true, false, false, true, true } },
    // Fragezeichen (?): ..--.. (Index 38)
    [38] = { .length = 6, .elements = { false, false, true, true, false, false } },
    // Schrägstrich (/): -..-. (Index 39)
    [39] = { .length = 5, .elements = { true, false, false, true, false } },
};
// Größe des Lookup-Tabelle
#define MORSE_CODES_SIZE (sizeof(morse_codes) / sizeof(morse_codes[0]))

/**
 * @brief Berechnet die Dit-Dauer in Samples basierend auf Wörtern pro Minute (WPM).
 * @param wpm Die Geschwindigkeit in Wörtern pro Minute.
 * @return size_t Die Dauer eines Dits in 22050 Hz Samples.
 */
static size_t calculate_dot_samples(uint32_t wpm) {
    if (wpm == 0) return 0;
    // Dot-Dauer (Sekunden) = 1.2 / WPM.
    // Dot-Dauer (Samples) = SAMPLE_RATE * (1.2 / WPM)
    double dot_duration_seconds = DOT_TIME_SECONDS_NUMERATOR / (double)wpm;
    return (size_t)round((double)SAMPLE_RATE * dot_duration_seconds);
}

/**
 * @brief Ruft die MorseCode-Struktur für ein Zeichen ab.
 * @param c Das Zeichen.
 * @return const MorseCode* Zeiger auf die Struktur oder NULL bei unbekanntem Zeichen.
 */
static const MorseCode* get_morse_code(char c) {
    // Konvertiere zu Großbuchstaben für die Konsistenz
    if (c >= 'a' && c <= 'z') {
        c = c - 'a' + 'A';
    }

    if (c >= 'A' && c <= 'Z') {
        return &morse_codes[c - 'A'];
    }
    if (c >= '0' && c <= '9') {
        // '0' ist Index 26, '1' ist Index 27, etc.
        return &morse_codes[c - '0' + 26];
    }
    
    switch (c) {
        case '.': return &morse_codes[36];
        case ',': return &morse_codes[37];
        case '?': return &morse_codes[38];
        case '/': return &morse_codes[39];
        // Leerzeichen wird durch die Pausenlogik außerhalb dieser Funktion behandelt
        case ' ':
            return NULL;
        default:
            return NULL; // Unbekanntes Zeichen
    }
}

/**
 * @brief Berechnet die erforderliche Puffergröße für die vollständige Nachricht.
 */
size_t morse_messageLength(const char* message, uint32_t wpm) {
    if (wpm == 0 || message == NULL || *message == '\0') {
        return 0;
    }

    size_t dot_samples = calculate_dot_samples(wpm);
    if (dot_samples == 0) return 0;

    // Verwende lokale Makros, die auf der berechneten dot_samples basieren
    const size_t DIT = dot_samples;
    const size_t DAH = dot_samples * 3;
    const size_t ELEMENT_GAP = dot_samples;
    const size_t CHAR_GAP = dot_samples * 3; // Lücke zwischen Zeichen (Dauer = 3 Dits)
    const size_t WORD_GAP = dot_samples * 7; // Lücke zwischen Wörtern (Dauer = 7 Dits)

    size_t total_samples = 0;
    bool previous_char_was_space = true; // Starte mit einer impliziten Wortlücke

    for (const char *p = message; *p != '\0'; p++) {
        char c = *p;

        if (c == ' ') {
            // Wenn das vorherige Zeichen kein Leerzeichen war, füge die Wortpause hinzu
            if (!previous_char_was_space) {
                total_samples += WORD_GAP;
            }
            previous_char_was_space = true;
            continue;
        }

        const MorseCode* code = get_morse_code(c);
        if (code == NULL) {
            fprintf(stderr, "Warnung: Unbekanntes Morse-Zeichen '%c' übersprungen.\n", c);
            previous_char_was_space = false;
            continue;
        }

        // 1. Zähle die Elemente (Dits/Dahs) und die Lücken zwischen ihnen
        for (uint8_t i = 0; i < code->length; i++) {
            // Elementdauer (DIT oder DAH)
            total_samples += code->elements[i] ? DAH : DIT;

            // Lücke nach dem Element (außer nach dem letzten Element)
            if (i < code->length - 1) {
                total_samples += ELEMENT_GAP;
            }
        }

        // 2. Lücke nach dem Zeichen (3 Dits, ABER: die letzte Element-Lücke ist 1 Dit, 
        // also müssen wir nur 2 weitere Dits hinzufügen, um auf 3 zu kommen)
        // Die Lücke zwischen Elementen ist 1 Dit, die Lücke zwischen Zeichen ist 3 Dits.
        // Die Lücke nach dem letzten Element eines Zeichens muss 3 Dits sein.
        // Da wir nach dem letzten Element KEINE ELEMENT_GAP hinzugefügt haben,
        // müssen wir CHAR_GAP (3 Dits) hinzufügen.
        total_samples += CHAR_GAP;

        previous_char_was_space = false;
    }

    // Entferne die CHAR_GAP oder WORD_GAP nach dem letzten Zeichen
    if (total_samples > CHAR_GAP) {
        total_samples -= CHAR_GAP;
    } else if (total_samples > WORD_GAP) {
        // Dies sollte nicht passieren, aber eine Sicherheitsmaßnahme
        total_samples -= WORD_GAP;
    }

    return total_samples;
}

/**
 * @brief Kodiert einen gegebenen ASCII-String in eine Sequenz von Morse-Elementen
 * und generiert das rohe Audiosignal (PCM).
 */
size_t morse_encodeTransmission(const char* message, uint32_t wpm, int16_t* out, size_t buffer_size) {
    if (wpm == 0 || message == NULL || out == NULL || buffer_size == 0) {
        return 0;
    }

    // Berechne die genauen Timings
    size_t dot_samples = calculate_dot_samples(wpm);
    if (dot_samples == 0) return 0;

    const size_t DIT = dot_samples;
    const size_t DAH = dot_samples * 3;
    const size_t ELEMENT_GAP = dot_samples;
    const size_t CHAR_GAP = dot_samples * 3; // Lücke zwischen Zeichen (3 Dits)
    const size_t WORD_GAP = dot_samples * 7; // Lücke zwischen Wörtern (7 Dits)
    
    size_t written_samples = 0;
    size_t overall_time_index = 0; // Für die kontinuierliche Sinus-Berechnung

    // Generiert eine Pause (0-Samples)
    #define GENERATE_SILENCE(duration) \
        for (size_t i = 0; i < duration; i++) { \
            if (written_samples >= buffer_size) return written_samples; \
            out[written_samples++] = 0; \
            overall_time_index++; \
        }

    // Generiert einen Ton (Morsecode-Element)
    #define GENERATE_TONE(duration) \
        for (size_t i = 0; i < duration; i++) { \
            if (written_samples >= buffer_size) return written_samples; \
            out[written_samples++] = rs_generate_tone_sample(MORSE_TONE_FREQ, overall_time_index, SAMPLE_RATE); \
            overall_time_index++; \
        }

    bool previous_char_was_space = true; // Behandelt den ersten Wortabstand

    for (const char *p = message; *p != '\0'; p++) {
        char c = *p;

        if (c == ' ') {
            // Wortpause: 7 Dits
            if (!previous_char_was_space) {
                GENERATE_SILENCE(WORD_GAP);
            }
            previous_char_was_space = true;
            continue;
        }

        const MorseCode* code = get_morse_code(c);
        if (code == NULL) {
            fprintf(stderr, "Warnung: Unbekanntes Morse-Zeichen '%c' übersprungen.\n", c);
            previous_char_was_space = false;
            continue;
        }

        // Kodierung der Elemente
        for (uint8_t i = 0; i < code->length; i++) {
            size_t element_duration = code->elements[i] ? DAH : DIT;
            
            // 1. Ton (DIT oder DAH)
            GENERATE_TONE(element_duration);

            // 2. Element-Lücke (1 Dit) - Nur zwischen Elementen, NICHT nach dem letzten
            if (i < code->length - 1) {
                GENERATE_SILENCE(ELEMENT_GAP);
            }
        }

        // 3. Zeichen-Lücke (3 Dits)
        // Die Lücke nach dem letzten Element muss 3 Dits sein.
        // Die 1-Dit-Lücke wird von ELEMENT_GAP nicht hinzugefügt, also fügen wir hier die vollen CHAR_GAP hinzu.
        GENERATE_SILENCE(CHAR_GAP);
        
        previous_char_was_space = false;
    }

    // Entferne die am Ende hinzugefügte Zeichen- oder Wort-Lücke
    if (written_samples >= CHAR_GAP) {
        written_samples -= CHAR_GAP;
    } else if (written_samples >= WORD_GAP) {
         written_samples -= WORD_GAP;
    }

    return written_samples;
}