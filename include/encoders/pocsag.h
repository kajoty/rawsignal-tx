#ifndef POCSAG_H
#define POCSAG_H

#include <stdint.h>
#include <stddef.h>

// POCSAG Konstanten
#define PREAMBLE_LENGTH 576 // Bits
#define BATCH_SIZE 16       // 16 Wörter pro Batch (plus 1 Sync-Wort)
#define FRAME_SIZE 2        // 2 Wörter pro Frame
#define TEXT_BITS_PER_WORD 20
#define TEXT_BITS_PER_CHAR 7

// Funktionscodes (Bit 1-2 des Adresswort-Datenfeldes)
typedef enum {
    FUNC_ALARM_NUMERIC = 0,
    FUNC_NUMERIC_1 = 1,
    FUNC_NUMERIC_2 = 2,
    FUNC_ALPHA_TEXT = 3
} FunctionCode;

/**
 * @brief Berechnet die Gesamtlänge der POCSAG-Übertragung in 32-Bit-Wörtern.
 */
size_t pocsag_messageLength(uint32_t address, size_t numChars, FunctionCode functionCode);

/**
 * @brief Kodiert die vollständige POCSAG-Nachricht.
 */
void pocsag_encodeTransmission(
    uint32_t address,
    const char* message,
    uint32_t* out,
    FunctionCode functionCode
);

#endif // POCSAG_H