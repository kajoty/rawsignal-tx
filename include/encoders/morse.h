#ifndef MORSE_H
#define MORSE_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

// Morsecode Konstanten (Timing basiert auf der 'Dit'-Dauer)
// Wir verwenden WPM (Words Per Minute) basierend auf dem Wort "PARIS".
// Standard-Werte bei 22050 Hz Sampling-Rate.
#define SAMPLE_RATE 22050
#define DOT_LENGTH_MS 50 // Standard 'Dit'-Dauer in Millisekunden
#define DOT_SAMPLES (SAMPLE_RATE * DOT_LENGTH_MS / 1000)

// Morse-Regeln (ITU-R M.1677-1):
// 1. Dit-Dauer = 1 Einheit
// 2. Dah-Dauer = 3 Einheiten
// 3. Pause zwischen Dits/Dahs = 1 Einheit
// 4. Pause zwischen Zeichen = 3 Einheiten
// 5. Pause zwischen Wörtern = 7 Einheiten

#define DIT_SAMPLES DOT_SAMPLES
#define DAH_SAMPLES (DOT_SAMPLES * 3)
#define CHAR_GAP_SAMPLES (DOT_SAMPLES * 3)
#define WORD_GAP_SAMPLES (DOT_SAMPLES * 7)
#define ELEMENT_GAP_SAMPLES DOT_SAMPLES

// Struktur zur Darstellung des Morsecodes
// max. 6 Elemente (z.B. '5' ist '.....', '/' ist '-..-.')
#define MAX_MORSE_LENGTH 6
typedef struct {
    uint8_t length; // Länge des Codes (z.B. 4 für '....')
    bool elements[MAX_MORSE_LENGTH]; // true = Dah, false = Dit
} MorseCode;

/**
 * @brief Kodiert einen gegebenen ASCII-String in eine Sequenz von Morse-Elementen
 * und generiert das rohe Audiosignal (PCM).
 *
 * @param message Die zu sendende ASCII-Nachricht (nur Buchstaben A-Z, Zahlen 0-9, Leerzeichen).
 * @param wpm Die Geschwindigkeit in Wörtern pro Minute.
 * @param out Zeiger auf den Speicherort, an dem das PCM-Signal gespeichert wird.
 * @param buffer_size Die maximale Größe des Ausgabepuffers.
 * @return size_t Die tatsächlich geschriebene Anzahl von PCM-Samples (oder 0 bei Fehler).
 */
size_t morse_encodeTransmission(const char* message, uint32_t wpm, int16_t* out, size_t buffer_size);

/**
 * @brief Berechnet die erforderliche Puffergröße für die vollständige Nachricht.
 *
 * @param message Die Nachricht.
 * @param wpm Die Geschwindigkeit in Wörtern pro Minute.
 * @return size_t Die erforderliche Anzahl von 16-Bit-Samples.
 */
size_t morse_messageLength(const char* message, uint32_t wpm);

#endif // MORSE_H