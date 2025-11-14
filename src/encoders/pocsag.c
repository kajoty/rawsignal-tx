/*
 * POCSAG Encoder - C-Implementierung
 * * Der Großteil der Logik in dieser Datei (CRC, Parity, Kodierung der Wörter und Batch-Struktur) 
 * basiert auf dem ursprünglichen pocsag-encoder Projekt.
 * * Original Copyright (c) [2024] faithanalog
 * Quelle: https://github.com/faithanalog/pocsag-encoder
 * * Dieses Modul ist lizenziert unter der MIT-Lizenz.
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#include "../../include/signal_generator.h"
#include "../../include/encoders/pocsag.h"

// Interne POCSAG Konstanten
#define SYNC 0x7CD215D8
#define IDLE 0x7A89C197
#define FLAG_ADDRESS 0x000000
#define FLAG_MESSAGE 0x100000
#define CRC_BITS 10
#define CRC_GENERATOR 0b11101101001
#define FRAME_SIZE 2
#define PREAMBLE_LENGTH 576

/**
 * Berechnet den CRC-Fehlerprüfcode für das gegebene Wort (21 Datenbits).
 */
static uint32_t crc(uint32_t inputMsg) {
    uint32_t denominator = CRC_GENERATOR << 20;
    uint32_t msg = inputMsg << CRC_BITS;

    for (int column = 0; column <= 20; column++) {
        int msgBit = (msg >> (30 - column)) & 1;
        if (msgBit != 0) {
            msg ^= denominator;
        }
        denominator >>= 1;
    }
    return msg & 0x3FF;
}

/**
 * Berechnet das gerade Paritätsbit für eine Nachricht (31 Bits).
 */
static uint32_t parity(uint32_t x) {
    uint32_t p = 0;
    for (int i = 0; i < 32; i++) {
        p ^= (x & 1);
        x >>= 1;
    }
    return p;
}

/**
 * Kodiert eine 21-Bit-Nachricht durch Hinzufügen von CRC und Parität.
 */
static uint32_t encodeCodeword(uint32_t msg) {
    uint32_t fullCRC = (msg << CRC_BITS) | crc(msg);
    uint32_t p = parity(fullCRC);
    return (fullCRC << 1) | p;
}

/**
 * Berechnet den Offset des Adressworts (Anzahl der vorangehenden IDLE-Wörter).
 */
static uint32_t addressOffset(uint32_t address) {
    return (address & 0x7) * FRAME_SIZE;
}

/**
 * Kodiert eine Zeichenkette (string) als eine Reihe von Codewörtern.
 * Gibt die Anzahl der geschriebenen Codewörter zurück.
 */
static uint32_t encodeASCII(uint32_t initial_offset, const char* str, uint32_t* out) {
    uint32_t numWordsWritten = 0;
    uint32_t currentWord = 0;
    uint32_t currentNumBits = 0;
    uint32_t wordPosition = initial_offset; // Position im aktuellen Batch (0-15)

    while (*str != 0) {
        unsigned char c = *str;
        str++;

        // Kodiert die 7 Zeichenbits LSB zuerst
        for (int i = 0; i < TEXT_BITS_PER_CHAR; i++) {
            currentWord <<= 1;
            currentWord |= (c >> i) & 1;
            currentNumBits++;

            if (currentNumBits == TEXT_BITS_PER_WORD) {
                // Word ist voll (20 Bits)
                *out = encodeCodeword(currentWord | FLAG_MESSAGE);
                out++;
                numWordsWritten++;

                currentWord = 0;
                currentNumBits = 0;

                wordPosition++;
                if (wordPosition == BATCH_SIZE) {
                    // Batch ist voll, füge SYNC-Wort ein
                    *out = SYNC;
                    out++;
                    numWordsWritten++;
                    wordPosition = 0; // Beginne neuen Batch bei Position 0 (nach SYNC)
                }
            }
        }
    }

    // Schreibe das letzte, unvollständige Wort (wenn vorhanden)
    if (currentNumBits > 0) {
        // Fülle das Wort mit Nullen auf 20 Bits auf
        currentWord <<= 20 - currentNumBits;
        *out = encodeCodeword(currentWord | FLAG_MESSAGE);
        out++;
        numWordsWritten++;
        wordPosition++;
    }

    return numWordsWritten;
}


// =========================================================
// ÖFFENTLICHE FUNKTIONEN
// =========================================================

/**
 * @brief Berechnet die Gesamtlänge der POCSAG-Übertragung in 32-Bit-Wörtern.
 */
size_t pocsag_messageLength(uint32_t address, size_t numChars, FunctionCode functionCode) {
    size_t numWords = 0;

    // 1. Präambel
    numWords += PREAMBLE_LENGTH / 32;

    // 2. Sync-Wort (am Anfang des ersten Batches)
    numWords++;

    // 3. Füllung (Padding) vor dem Adresswort
    numWords += addressOffset(address);

    // 4. Das Adresswort selbst
    numWords++;

    // 5. Nachrichtenwörter
    size_t numMessageWords = (numChars * TEXT_BITS_PER_CHAR + (TEXT_BITS_PER_WORD - 1))
                               / TEXT_BITS_PER_WORD;
    numWords += numMessageWords;

    // 6. Leerlaufwort, das das Ende der Nachricht darstellt
    numWords++;
    
    // 7. SYNC-Wörter und Padding zur Auffüllung des letzten Batches
    size_t contentWords = numWords - (PREAMBLE_LENGTH / 32); 

    size_t wordsPerBatch = BATCH_SIZE + 1; // 17
    
    size_t contentWordsWritten = contentWords;
    
    size_t remainder = contentWordsWritten % wordsPerBatch;

    if (remainder != 0) {
        numWords += wordsPerBatch - remainder;
    }
    
    return numWords;
}

/**
 * @brief Kodiert die vollständige POCSAG-Nachricht.
 */
void pocsag_encodeTransmission(
    uint32_t address,
    const char* message,
    uint32_t* out,
    FunctionCode functionCode
) {
    // Kodiert die Präambel (alternierende 1, 0, 1, 0...)
    for (int i = 0; i < PREAMBLE_LENGTH / 32; i++) {
        *out = 0xAAAAAAAA;
        out++;
    }

    // Startzeiger (nach der Präambel)
    uint32_t* batchStart = out;

    // 1. Sync-Wort (Beginn des ersten Batches)
    *out = SYNC;
    out++;

    // 2. Füllung (Padding) vor dem Adresswort
    int prefixLength = addressOffset(address);
    for (int i = 0; i < prefixLength; i++) {
        *out = IDLE;
        out++;
    }

    // 3. Adresswort
    uint32_t addressData = ((address >> 3) << 2) | functionCode;
    *out = encodeCodeword(addressData);
    out++;

    // 4. Nachrichtenwörter
    out += encodeASCII( (addressOffset(address) + 1) % BATCH_SIZE, message, out);


    // 5. Abschließendes IDLE-Wort (Ende der Nachricht)
    *out = IDLE;
    out++;
    
    // 6. Padding, um den letzten Batch abzuschließen
    size_t contentWordsWritten = out - batchStart;
    size_t wordsPerBatch = BATCH_SIZE + 1; // 17

    size_t remainder = contentWordsWritten % wordsPerBatch;

    if (remainder != 0) {
        size_t padding = wordsPerBatch - remainder;
        for (size_t i = 0; i < padding; i++) {
            *out = IDLE;
            out++;
        }
    }
}