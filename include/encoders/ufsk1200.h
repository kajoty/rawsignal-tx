#ifndef UFSK1200_H
#define UFSK1200_H

#include <stdint.h>

// UFSK1200 Konstanten  
#define UFSK1200_BAUD_RATE 1200
#define UFSK1200_MARK_FREQ 1200    // Logisch '1'
#define UFSK1200_SPACE_FREQ 2200   // Logisch '0'

/**
 * @brief Enkodiert eine Nachricht als UFSK1200-Signal und schreibt PCM auf stdout.
 * UFSK1200 ist eine einfachere Version von AFSK1200 ohne HDLC-Framing.
 * 
 * @param message Die zu sendende Nachricht (Bytes).
 * @return 0 bei Erfolg, 1 bei Fehler.
 */
int rs_encode_ufsk1200(const char* message);

#endif // UFSK1200_H
