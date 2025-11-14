#ifndef AFSK1200_H
#define AFSK1200_H

#include <stdint.h>
#include <stddef.h>

// --- AFSK1200 (AX.25) Konstanten ---

// Die Baudrate für AFSK (Bits pro Sekunde)
#define AFSK_BAUD_RATE 1200

// Tonfrequenzen in Hz für Mark (Logisch 1) und Space (Logisch 0)
#define AFSK_MARK_FREQ 1200
#define AFSK_SPACE_FREQ 2200

/**
 * @brief Kodiert die übergebenen Daten in einen AX.25-Frame und generiert
 * das AFSK1200-Audiosignal direkt an stdout (S16_LE).
 *
 * Die Funktion übernimmt die gesamte Verarbeitung: AX.25-Frame-Erstellung,
 * CRC-Berechnung, Bit-Stuffing, NRZI-Kodierung und AFSK-Modulation.
 *
 * @param tx_call Sender-Rufzeichen (z.B. "DL1ABC-1").
 * @param dest_call Empfänger-Rufzeichen (z.B. "APRS").
 * @param message Die zu übertragende Nachricht (Info-Feld).
 * @return int 0 bei Erfolg, 1 bei Fehler.
 */
int rs_encode_afsk1200(const char* tx_call, const char* dest_call, const char* message);

#endif // AFSK1200_H