#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <strings.h> 

#include "../include/signal_generator.h"
#include "../include/encoders/pocsag.h"
// TODO: #include "../include/encoders/tones.h"

#define MAX_DELAY 10 // Sekunden
#define MIN_DELAY 1  // Sekunden

/**
 * @brief Gibt eine Fehlermeldung zur korrekten Nutzung des Programms aus.
 */
static void print_usage(const char* progName) {
    fprintf(stderr, "Nutzung: %s <MODULATOR> <PARAMETER>\n", progName);
    fprintf(stderr, "\nVerfügbare Modulatoren:\n");
    fprintf(stderr, "  POCSAG [BAUD] [ADRESSE]:[FUNKTION]:[NACHRICHT]\n");
    fprintf(stderr, "  Beispiel: %s POCSAG 512 1234567:3:HALLO\n", progName);
    fprintf(stderr, "  (Standard-Funktion ist 3 (Alpha) wenn weggelassen, z.B. 1234567:HALLO)\n");
    fprintf(stderr, "  DTMF [TONE_SEQUENZ] (NICHT IMPLEMENTIERT)\n");
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

        // --- Parameter-Parsing
        uint32_t baudRate = (uint32_t) strtol(argv[2], NULL, 10);
        char* input_string = argv[3];

        if (baudRate != 512 && baudRate != 1200 && baudRate != 2400) {
            fprintf(stderr, "Fehler: Ungültige POCSAG-Baudrate. Erlaubt: 512, 1200, 2400.\n");
            return 1;
        }

        // --- Input-Parsing
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


        // --- Kodierung und Ausgabe
        size_t requiredMessageLength = pocsag_messageLength(address, strlen(message), functionCode);
        uint32_t* transmission = (uint32_t*) malloc(sizeof(uint32_t) * requiredMessageLength);
        
        if (transmission == NULL) {
            fprintf(stderr, "Fehler: Speicherzuweisung für POCSAG-Wörter fehlgeschlagen.\n");
            return 1;
        }

        pocsag_encodeTransmission(address, message, transmission, functionCode);

        size_t pcmLength = pcmTransmissionLength(SAMPLE_RATE, baudRate, requiredMessageLength);
        uint8_t* pcm = (uint8_t*) malloc(sizeof(uint8_t) * pcmLength);

        if (pcm == NULL) {
            fprintf(stderr, "Fehler: Speicherzuweisung für PCM-Puffer fehlgeschlagen.\n");
            free(transmission);
            return 1;
        }

        pcmEncodeTransmission(SAMPLE_RATE, baudRate, transmission, requiredMessageLength, pcm);

        // Schreibe die PCM-Samples (S16_LE) an stdout
        fwrite(pcm, sizeof(uint8_t), pcmLength, stdout);

        free(transmission);
        free(pcm);

    } else if (strcasecmp(modulator, "DTMF") == 0) {
        fprintf(stderr, "\n--- DTMF-Modulator ist noch nicht implementiert ---\n");
        return 1;
    } else {
        fprintf(stderr, "Fehler: Unbekannter Modulator '%s'.\n", modulator);
        print_usage(argv[0]);
        return 1;
    }

    // Füge zufällige Stille am Ende hinzu
    size_t silenceLength = rand() % (SAMPLE_RATE * (MAX_DELAY - MIN_DELAY)) + MIN_DELAY;
    uint16_t* silence = (uint16_t*) malloc(sizeof(uint16_t) * silenceLength);
    if (silence != NULL) {
        bzero(silence, sizeof(uint16_t) * silenceLength);
        fwrite(silence, sizeof(uint16_t), silenceLength, stdout);
        free(silence);
    }
    
    return 0;
}