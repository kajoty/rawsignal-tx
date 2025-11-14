#ifndef TONES_H
#define TONES_H

#include <stdio.h> // Für int
#include <unistd.h> // Für usleep (obwohl oft in der .c-Datei, hier zur Vollständigkeit)

/**
 * @brief Codiert und sendet eine Sequenz von DTMF-Tönen (Dual-Tone Multi-Frequency).
 * * Diese Funktion generiert die Audiosignale für DTMF-Ziffern und gibt sie als
 * Raw-Audio-Daten an stdout aus.
 * * @param digits Eine Zeichenkette der zu sendenden Ziffern (0-9, *, #, A-D).
 * @param tone_duration_ms Die Dauer eines Tones in Millisekunden (z.B. 50ms).
 * @param pause_duration_ms Die Pause zwischen den Tönen in Millisekunden (z.B. 50ms).
 * @return 0 bei Erfolg, ungleich 0 bei unbekanntem Zeichen.
 */
int rs_encode_dtmf(const char *digits, int tone_duration_ms, int pause_duration_ms);

#endif // TONES_H