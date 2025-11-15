#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <time.h>
#include <strings.h>
#include <ctype.h>

#include "../include/signal_generator.h"
#include "../include/encoders/pocsag.h"
#include "../include/encoders/tones.h" 
#include "../include/encoders/morse.h" 
#include "../include/encoders/afsk1200.h"
#include "../include/encoders/fsk9600.h"
#include "../include/encoders/ufsk1200.h"

// Standardwerte
#define MAX_DELAY 10 // Sekunden
#define MIN_DELAY 1 // Sekunden
#define DTMF_TONE_MS 50
#define DTMF_PAUSE_MS 50
#define MORSE_WPM_DEFAULT 20
#define AFSK_DEFAULT_TX_CALL "RAW-TX" // NEU: Standard Sender-Callsign
#define AFSK_DEFAULT_DEST_CALL "APRS" // NEU: Standard Empfänger-Callsign

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
  fprintf(stderr, "\n MORSE_CW [NACHRICHT] [WPM]\n");
  fprintf(stderr, " Beispiel: %s MORSE_CW CQDX 20\n", progName);
  fprintf(stderr, " (Standard WPM: %d)\n", MORSE_WPM_DEFAULT);
  
  fprintf(stderr, "\n AFSK1200 [TX_CALL] [DEST_CALL] [NACHRICHT]\n");
  fprintf(stderr, " Beispiel: %s AFSK1200 DL1ABC APRS 'Hallo Welt'\n", progName);
  fprintf(stderr, " (Standard: Sender=%s, Empfänger=%s)\n", AFSK_DEFAULT_TX_CALL, AFSK_DEFAULT_DEST_CALL);
  
  fprintf(stderr, "\n FSK9600 [NACHRICHT]\n");
  fprintf(stderr, " Beispiel: %s FSK9600 'Hallo'\n", progName);
  
  fprintf(stderr, "\n UFSK1200 [NACHRICHT]\n");
  fprintf(stderr, " Beispiel: %s UFSK1200 'Hallo'\n", progName);
}


// --- HILFSFUNKTIONEN FÜR DIE MORSE-GENERIERUNG ---

/**
 * @brief Führt die Kodierung und Ausgabe für Morse durch.
 */
static int handle_morse_encoding(const char* message, uint32_t wpm) {
  // 1. Puffergröße berechnen
  size_t requiredSamples = morse_messageLength(message, wpm);
  if (requiredSamples == 0) {
    fprintf(stderr, "Fehler: Puffergröße für Morsecode konnte nicht berechnet werden.\n");
    return 1;
  }

  // 2. Speicher zuweisen (int16_t = 2 Bytes pro Sample)
  int16_t* pcm = (int16_t*) malloc(sizeof(int16_t) * requiredSamples);
  if (pcm == NULL) {
    fprintf(stderr, "Fehler: Speicherzuweisung für Morse-PCM-Puffer fehlgeschlagen.\n");
    return 1;
  }

  // 3. Kodierung
  size_t actualSamples = morse_encodeTransmission(message, wpm, pcm, requiredSamples);

  // 4. Ausgabe
  if (actualSamples > 0) {
    // Ausgabe als Signed 16-bit Little-Endian (S16_LE)
    fwrite(pcm, sizeof(int16_t), actualSamples, stdout);
  } else {
    fprintf(stderr, "Fehler: Morse-Kodierung erzeugte kein Signal.\n");
  }

  free(pcm);
  return (actualSamples > 0) ? 0 : 1;
}

// --- HAUPTPROGRAMM ---

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
  int result = 0; // Ergebnis der Kodierung

  // --- 1. POCSAG-Logik ---
  if (strcasecmp(modulator, "POCSAG") == 0) {
    if (argc < 4) {
      fprintf(stderr, "Fehler: POCSAG benötigt BAUD und ADRESSE:NACHRICHT.\n");
      print_usage(argv[0]);
      return 1;
    }
    
    uint32_t baudRate = (uint32_t) strtol(argv[2], NULL, 10);
    char* input_string = argv[3];

    if (baudRate != 512 && baudRate != 1200 && baudRate != 2400) {
      fprintf(stderr, "Fehler: Ungültige POCSAG-Baudrate. Erlaubt: 512, 1200, 2400.\n");
      return 1;
    }

    uint32_t address = 0;
    FunctionCode functionCode = FUNC_ALPHA_TEXT;
    char* message = NULL;
    // Puffer für strtok Kopie
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
    fprintf(stderr, "Info: POCSAG-Kodierung: Adresse %u, Baud %u, Code %u.\n", address, baudRate, functionCode);


    // Kodierung
    size_t requiredMessageLength = pocsag_messageLength(address, strlen(message), functionCode);
    uint32_t* transmission = (uint32_t*) malloc(sizeof(uint32_t) * requiredMessageLength);
    
    if (transmission == NULL) {
      fprintf(stderr, "Fehler: Speicherzuweisung für POCSAG-Wörter fehlgeschlagen.\n");
      return 1;
    }

    pocsag_encodeTransmission(address, message, transmission, functionCode);

    // Signalerzeugung und Ausgabe
    size_t pcmLength = pcmTransmissionLength(SAMPLE_RATE, baudRate, requiredMessageLength) * sizeof(int16_t); // Länge in Bytes
    int16_t* pcm = (int16_t*) malloc(pcmLength); // Größe in int16_t Samples
    
    if (pcm == NULL) {
      fprintf(stderr, "Fehler: Speicherzuweisung für PCM-Puffer fehlgeschlagen.\n");
      free(transmission);
      return 1;
    }

    // Annahme: pcmEncodeTransmission ist jetzt für int16_t optimiert
    pcmEncodeTransmission(SAMPLE_RATE, baudRate, transmission, requiredMessageLength, pcm);
    fwrite(pcm, sizeof(int16_t), pcmLength / sizeof(int16_t), stdout);

    free(transmission);
    free(pcm);
    
    result = 0; // Erfolg

  // --- 2. DTMF-Logik ---
  } else if (strcasecmp(modulator, "DTMF") == 0) {
    if (argc < 3) {
      fprintf(stderr, "Fehler: DTMF benötigt mindestens die Ton-Sequenz.\n");
      print_usage(argv[0]);
      return 1;
    }

    const char* digits = argv[2];
    int tone_duration = DTMF_TONE_MS;
    int pause_duration = DTMF_PAUSE_MS;

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

    // Der DTMF-Encoder schreibt die Samples direkt an stdout (als S16_LE)
    result = rs_encode_dtmf(digits, tone_duration, pause_duration);
    
  // --- 3. MORSE_CW-Logik ---
  } else if (strcasecmp(modulator, "MORSE_CW") == 0) {
    if (argc < 3) {
      fprintf(stderr, "Fehler: MORSE_CW benötigt die Nachricht.\n");
      print_usage(argv[0]);
      return 1;
    }
    
    const char* message = argv[2];
    uint32_t wpm = MORSE_WPM_DEFAULT;

    if (argc >= 4) {
      wpm = (uint32_t) strtol(argv[3], NULL, 10);
    }
    
    if (wpm == 0 || wpm > 60) {
      fprintf(stderr, "Fehler: Ungültige WPM. Erlaubt: 1-60.\n");
      return 1;
    }

    fprintf(stderr, "Info: MORSE_CW-Kodierung: '%s' bei %u WPM.\n", message, wpm);
    
    result = handle_morse_encoding(message, wpm);

  
  // --- 4. AFSK1200-Logik (NEU) ---
  } else if (strcasecmp(modulator, "AFSK1200") == 0) {
    
    if (argc < 4) {
      fprintf(stderr, "Fehler: AFSK1200 benötigt mindestens Sender-Call und Nachricht.\n");
      print_usage(argv[0]);
      return 1;
    }

    // Argumente: [0]prog | [1]AFSK1200 | [2]TX_CALL | [3]DEST_CALL | [4]MESSAGE
    
    const char* tx_call = argv[2];
    const char* dest_call = argv[3];
    const char* message = (argc >= 5) ? argv[4] : ""; 

    // Rudimentäre Validierung
    if (strlen(tx_call) > 10 || strlen(dest_call) > 10) {
      fprintf(stderr, "Fehler: Callsigns dürfen 10 Zeichen nicht überschreiten (AX.25-Limit).\n");
      return 1;
    }
    
    fprintf(stderr, "Info: AFSK1200: %s an %s, Nachricht: '%.20s...'\n", 
        tx_call, dest_call, message);
    
    // Aufruf der AFSK1200 Hauptfunktion
    result = rs_encode_afsk1200(tx_call, dest_call, message);
    
  } else if (strcasecmp(modulator, "FSK9600") == 0) {
    if (argc < 3) {
      fprintf(stderr, "Fehler: FSK9600 benötigt eine Nachricht.\n");
      print_usage(argv[0]);
      return 1;
    }
    
    const char* message = argv[2];
    result = rs_encode_fsk9600(message);
    
  } else if (strcasecmp(modulator, "UFSK1200") == 0) {
    if (argc < 3) {
      fprintf(stderr, "Fehler: UFSK1200 benötigt eine Nachricht.\n");
      print_usage(argv[0]);
      return 1;
    }
    
    const char* message = argv[2];
    result = rs_encode_ufsk1200(message);
    
  } else {
    fprintf(stderr, "Fehler: Unbekannter Modulator '%s'.\n", modulator);
    print_usage(argv[0]);
    result = 1;
  }

  // --- ZUFÄLLIGE END-STILLE (Wird nur bei Erfolg hinzugefügt) ---
  if (result == 0) {
    // Füge zufällige Stille am Ende hinzu (für SDR-Tools oft nützlich)
    // Die Berechnung ergibt die Anzahl der Samples (int16_t)
    size_t silenceSamples = rand() % (SAMPLE_RATE * (MAX_DELAY - MIN_DELAY)) + (SAMPLE_RATE * MIN_DELAY);
    
    uint16_t* silence = (uint16_t*) malloc(sizeof(uint16_t) * silenceSamples); 
    
    if (silence != NULL) {
      // Fülle den Puffer mit Nullen (Stille)
      memset(silence, 0, sizeof(uint16_t) * silenceSamples);
      // Schreibe die Samples (int16_t)
      fwrite(silence, sizeof(uint16_t), silenceSamples, stdout);
      free(silence);
    }
  }
  
  return result;
}