#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <math.h>

#include "../../include/signal_generator.h"
#include "../../include/crc.h"
#include "../../include/encoders/afsk1200.h"

// Makros für AX.25 Konstanten
#define AX25_FLAG 0x7E
#define AX25_ADDR_LEN 7
#define AX25_CONTROL_UI 0x03
#define AX25_PID_NO_PROTOCOL 0xF0
#define AX25_FCS_LEN 2

// Berechnung der Samples pro Bit
#define SAMPLES_PER_BIT (SAMPLE_RATE / AFSK_BAUD_RATE)
// Amplitude für S16_LE (aus signal_generator.h)
#define MAX_AMPLITUDE MAX_PCM_VALUE 

// --- Globale Zustände für Phasen- und NRZI-Kontinuität ---
static double current_phase = 0.0;
// Letzter Zustand des Frequenz-Wechsels für NRZI. 1 = Mark (1200 Hz), 0 = Space (2200 Hz)
static int last_nrzi_state = 1; 

/**
 * @brief Konvertiert ein Callsign (z.B. "DL1ABC") in das 7-Byte AX.25-Format.
 */
static void convert_callsign_to_ax25(const char *call, uint8_t *buffer) {
    char callsign[7];
    int ssid = 0;
    const char *hyphen = strchr(call, '-');

    if (hyphen) {
        strncpy(callsign, call, hyphen - call);
        callsign[hyphen - call] = '\0';
        ssid = atoi(hyphen + 1);
        if (ssid < 0 || ssid > 15) {
            ssid = 0;
        }
    } else {
        strncpy(callsign, call, 6);
        callsign[6] = '\0';
    }

    size_t len = strlen(callsign);
    size_t i; 
    for (i = 0; i < 6; i++) {
        char c = (i < len) ? toupper(callsign[i]) : ' ';
        buffer[i] = (uint8_t)(c << 1);
    }

    buffer[6] = (uint8_t)(ssid << 1); 
    buffer[6] |= 0b01000000; 
}


/**
 * @brief Moduliert eine Sequenz von Bits und gibt sie als PCM auf stdout aus.
 * * Die Funktion handhabt NRZI und Bit-Stuffing.
 *
 * @param bits Die zu sendenden Bits (als Bytes).
 * @param len Die Länge des Puffer in Bytes.
 */
static void ax25_modulate_data(const uint8_t *bits, size_t len) {
    int one_counter = 0; 
    int16_t pcm_buffer[SAMPLES_PER_BIT]; 

    for (size_t byte_index = 0; byte_index < len; byte_index++) {
        uint8_t current_byte = bits[byte_index];

        for (int bit_index = 0; bit_index < 8; bit_index++) {
            
            // 1. Bit extrahieren (AX.25 ist LSB zuerst)
            int current_bit = (current_byte >> bit_index) & 0x01;
            
            // --- Helper-Makro für die Ausgabe eines einzelnen Bits ---
            #define EMIT_BIT(b) \
            { \
                int output_state = last_nrzi_state; \
                /* Logisch '0' (Space) bedeutet Frequenzwechsel. */ \
                if (b == 0) { \
                    output_state = !output_state; \
                    last_nrzi_state = output_state; \
                } \
                double frequency = output_state ? AFSK_MARK_FREQ : AFSK_SPACE_FREQ; \
                double step = 2.0 * M_PI * frequency / SAMPLE_RATE; \
                /* PCM-Generierung */ \
                for (int i = 0; i < SAMPLES_PER_BIT; i++) { \
                    pcm_buffer[i] = (int16_t)(MAX_AMPLITUDE * sin(current_phase)); \
                    current_phase += step; \
                    if (current_phase >= 2.0 * M_PI) { \
                        current_phase -= 2.0 * M_PI; \
                    } \
                } \
                fwrite(pcm_buffer, sizeof(int16_t), SAMPLES_PER_BIT, stdout); \
            }

            // 2. Bit-Stuffing Logik
            if (current_bit == 1) {
                one_counter++;
            } else { // current_bit == 0
                one_counter = 0;
            }

            // A) Das eigentliche Daten-Bit senden
            EMIT_BIT(current_bit);
            
            // B) Optionales Bit-Stuffing Bit senden (immer logisch '0')
            if (one_counter == 5) {
                fprintf(stderr, "Debug: Bit-Stuffing ('0') eingefügt.\n");
                EMIT_BIT(0); // Sendet das gestuffte '0'-Bit
                one_counter = 0; // Zähler zurücksetzen
            }
        }
    }
    #undef EMIT_BIT
}


/**
 * @brief Generiert eine Sequenz von AX.25 Flags (0x7E).
 *
 * @param count Anzahl der 0x7E Flags.
 */
static void generate_preamble_and_flag(int count) {
    uint8_t flag = AX25_FLAG;
    // Das Flag (0x7E = 01111110) enthält nie 5 Einsen, daher keine Bit-Stuffing-Gefahr.
    for (int i = 0; i < count; i++) {
        ax25_modulate_data(&flag, 1);
    }
}


// --- HAUPTFUNKTION (Final) ---

int rs_encode_afsk1200(const char* tx_call, const char* dest_call, const char* message) {
    
    // Globale Zustände zurücksetzen (Für mehrere Übertragungen wichtig)
    current_phase = 0.0;
    last_nrzi_state = 1; 

    // Längen und Offsets
    size_t info_len = strlen(message);
    size_t data_len = (2 * AX25_ADDR_LEN) + 1 + 1 + info_len; // Adressen + Control + PID + Info
    size_t data_and_fcs_len = data_len + AX25_FCS_LEN;
    
    // Wir bauen den Frame ohne die Flags, da Flags separat behandelt werden
    uint8_t *frame_data = (uint8_t *)malloc(data_and_fcs_len);
    if (frame_data == NULL) {
        fprintf(stderr, "Fehler: Speicherzuweisung für AX.25 Frame fehlgeschlagen.\n");
        return 1;
    }

    // ------------------------------------
    // 1. FRAME AUFBAU (Callsigns, Header, Info, FCS)
    // ------------------------------------
    uint8_t dest_addr[AX25_ADDR_LEN];
    uint8_t source_addr[AX25_ADDR_LEN];
    convert_callsign_to_ax25(dest_call, dest_addr);
    convert_callsign_to_ax25(tx_call, source_addr);
    source_addr[AX25_ADDR_LEN - 1] |= 0x01; // End-of-Address-Bit

    size_t offset = 0;
    memcpy(frame_data + offset, dest_addr, AX25_ADDR_LEN); offset += AX25_ADDR_LEN;
    memcpy(frame_data + offset, source_addr, AX25_ADDR_LEN); offset += AX25_ADDR_LEN;
    frame_data[offset++] = AX25_CONTROL_UI; // Control
    frame_data[offset++] = AX25_PID_NO_PROTOCOL; // PID
    memcpy(frame_data + offset, message, info_len); offset += info_len; // Info

    // FCS Berechnung und Einfügen
    uint16_t fcs_value = crc16_ccitt(frame_data, data_len); 
    frame_data[offset++] = (uint8_t)(fcs_value & 0xFF);     // Low Byte
    frame_data[offset++] = (uint8_t)(fcs_value >> 8);      // High Byte
    
    fprintf(stderr, "Info: Datenlänge (ohne Flags): %zu Bytes.\n", data_and_fcs_len);
    fprintf(stderr, "Info: CRC-16 (FCS): 0x%04X (invertiert 0x%04X).\n", ~fcs_value, fcs_value);

    // ------------------------------------
    // 2. MODULATION und AUSGABE
    // ------------------------------------
    
    // A) PRE-AMBLE (Synchronisation/HDLC-Header). Sende 16 Flags (128 Bits)
    fprintf(stderr, "Info: Sende 16 Pre-Amble Flags (0x7E).\n");
    generate_preamble_and_flag(16); 
    
    // B) START-FLAG
    fprintf(stderr, "Info: Sende Start-Flag (0x7E).\n");
    generate_preamble_and_flag(1); 
    
    // C) DATEN-BLOCK (Adressen, Control, PID, Info, FCS)
    fprintf(stderr, "Info: Sende Daten (inkl. Stuffing & NRZI).\n");
    ax25_modulate_data(frame_data, data_and_fcs_len);
    
    // D) END-FLAG
    fprintf(stderr, "Info: Sende End-Flag (0x7E).\n");
    generate_preamble_and_flag(1);

    free(frame_data);
    return 0; 
}