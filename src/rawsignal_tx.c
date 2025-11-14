#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <strings.h> 

#include "../include/signal_generator.h"
#include "../include/encoders/pocsag.h"
#include "../include/encoders/tones.h" // NEU: DTMF-Header inkludiert

#define MAX_DELAY 10 // Sekunden
#define MIN_DELAY 1  // Sekunden

// DTMF-Standarddauern
#define DTMF_TONE_MS 50
#define DTMF_PAUSE_MS 50


/**
 * @brief Gibt eine Fehlermeldung zur korrekten Nutzung des Programms aus.
 */
static void print_usage(const char* progName) {
    fprintf(stderr, "Nutzung: %s <MODULATOR> <PARAMETER>\n", progName);
    fprintf(stderr, "\nVerfügbare Modulatoren:\n");
    fprintf(stderr, " POCSAG [BAUD] [ADRESSE]:[FUNKTION]:[NACHRICHT]\n");
    fprintf(stderr, " Beispiel: %s POCSAG 512 1234567:3:HALLO\n", progName);
    fprintf(stderr, "\n DTMF [SEQUENZ] [TON_DAUER_MS] [PAUSE_DAUER_MS]\n");
    fprintf(stderr, " Beispiel: %s DTMF 123456# 50 50\n", progName);
    fprintf(stderr, " (Standarddauern: Ton=%dms, Pause=%dms)\n", DTMF_TONE_MS, DTMF_PAUSE_MS);
}

/**
 * @brief Hauptfunktion des Programms.
 */
int main(int argc, char* argv[]) {
    srand(time(NULL));

    if (argc < 3) {
        print_usage(argv[0]);
        return 1;
    }

    const char* modulator = argv[1];

    if (strcasecmp(modulator, "POCSAG") == 0) {
        if (argc < 4) {
            fprintf(stderr, "Fehler: POCSAG benötigt BAUD und ADRESSE:NACHRICHT.\n");
            print_usage(argv[0]);
            return 1;
        }
        
        // --- POCSAG-Logik (Unverändert) ---
        uint32_t baudRate = (uint32_t) strtol(argv[2], NULL, 10);
        char* input_string = argv[3];

        if (baudRate != 512 && baudRate != 1200 && baudRate != 2400) {
            fprintf(stderr, "Fehler: Ungültige POCSAG-Baudrate. Erlaubt: 512, 1200, 2400.\n");
            return 1;
        }

        uint32_t address = 0;
        FunctionCode functionCode = FUNC_ALPHA_TEXT;
        char* message = NULL;
        char inputCopy[65536]; 

        strncpy(inputCopy, input_string, sizeof(inputCopy) - 1);
        inputCopy[sizeof(inputCopy) - 1] = '\0';

        char* token = strtok(inputCopy, ":");
        if (token == NULL) {
            fprintf(stderr, "Fehler: Ungültiges POCSAG-Nachrichtenformat. Erwarte ADDR:MSG.\n");
            return 1;
        }
        address = (uint32_t) strtol(token, NULL, 10);

        char* funcOrMsgToken = strtok(NULL, ":");
        if (funcOrMsgToken == NULL) {
            fprintf(stderr, "Fehler: Nachrichtenteil fehlt.\n");
            return 1;
        }

        char* messageToken = strtok(NULL, ":");

        if (messageToken != NULL) {
            // Format: ADDR:FUNC:MSG
            functionCode = (FunctionCode) strtol(funcOrMsgToken, NULL, 10);
            message = messageToken;
        } else {
            // Format: ADDR:MSG
            message = funcOrMsgToken;
            functionCode = FUNC_ALPHA_TEXT;
        }

        if (address > 2097151) {
            fprintf(stderr, "Fehler: Adresse überschreitet 21 Bits: %u\n", address);
            return 1;
        }
        if (functionCode > 3) {
            fprintf(stderr, "Fehler: Ungültiger Funktionscode: %u. Erlaubt: 0-3.\n", functionCode);
            return 1;
        }

        // Kodierung
        size_t requiredMessageLength = pocsag_messageLength(address, strlen(message), functionCode);
        uint32_t* transmission = (uint32_t*) malloc(sizeof(uint32_t) * requiredMessageLength);
        
        if (transmission == NULL) {
            fprintf(stderr, "Fehler: Speicherzuweisung für POCSAG-Wörter fehlgeschlagen.\n");
            return 1;
        }

        pocsag_encodeTransmission(address, message, transmission, functionCode);

        // Signalerzeugung und Ausgabe
        size_t pcmLength = pcmTransmissionLength(SAMPLE_RATE, baudRate, requiredMessageLength);
        uint8_t* pcm = (uint8_t*) malloc(sizeof(uint8_t) * pcmLength);

        if (pcm == NULL) {
            fprintf(stderr, "Fehler: Speicherzuweisung für PCM-Puffer fehlgeschlagen.\n");
            free(transmission);
            return 1;
        }

        pcmEncodeTransmission(SAMPLE_RATE, baudRate, transmission, requiredMessageLength, pcm);
        fwrite(pcm, sizeof(uint8_t), pcmLength, stdout);

        free(transmission);
        free(pcm);

    } else if (strcasecmp(modulator, "DTMF") == 0) {
        // --- NEUE DTMF-Logik ---
        if (argc < 3) {
            fprintf(stderr, "Fehler: DTMF benötigt mindestens die Ton-Sequenz.\n");
            print_usage(argv[0]);
            return 1;
        }

        const char* digits = argv[2];
        int tone_duration = DTMF_TONE_MS; // Standard: 50ms
        int pause_duration = DTMF_PAUSE_MS; // Standard: 50ms

        // Optionale Argumente für Dauer
        if (argc >= 4) {
            tone_duration = (int) strtol(argv[3], NULL, 10);
        }
        if (argc >= 5) {
            pause_duration = (int) strtol(argv[4], NULL, 10);
        }

        if (tone_duration <= 0 || pause_duration <= 0) {
             fprintf(stderr, "Fehler: Ton- und Pausendauer müssen positiv sein.\n");
             return 1;
        }

        fprintf(stderr, "Info: DTMF-Kodierung: '%s' (Ton: %dms, Pause: %dms)\n", 
                digits, tone_duration, pause_duration);

        // Der DTMF-Encoder schreibt die Samples direkt an stdout
        if (rs_encode_dtmf(digits, tone_duration, pause_duration) != 0) {
            return 1; // Fehler bei der Kodierung
        }

    } else {
        fprintf(stderr, "Fehler: Unbekannter Modulator '%s'.\n", modulator);
        print_usage(argv[0]);
        return 1;
    }

    // Füge zufällige Stille am Ende hinzu (für SDR-Tools oft nützlich)
    size_t silenceLength = rand() % (SAMPLE_RATE * (MAX_DELAY - MIN_DELAY)) + MIN_DELAY;
    // Die Länge ist in Samples, wir allocieren 2 Bytes/Sample (uint16_t)
    uint16_t* silence = (uint16_t*) malloc(sizeof(uint16_t) * silenceLength); 
    
    if (silence != NULL) {
        bzero(silence, sizeof(uint16_t) * silenceLength);
        fwrite(silence, sizeof(uint16_t), silenceLength, stdout);
        free(silence);
    }
    
    return 0;
}