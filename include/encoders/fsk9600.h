#ifndef FSK9600_H
#define FSK9600_H

#include <stdint.h>

// FSK9600 Konstanten
#define FSK9600_BAUD_RATE 9600
#define FSK9600_MARK_FREQ 4800    // Logisch '1'
#define FSK9600_SPACE_FREQ 8400   // Logisch '0'

/**
 * @brief Enkodiert eine Nachricht als FSK9600-Signal und schreibt PCM auf stdout.
 * 
 * @param message Die zu sendende Nachricht (Bytes).
 * @return 0 bei Erfolg, 1 bei Fehler.
 */
int rs_encode_fsk9600(const char* message);

#endif // FSK9600_H
