#include "../include/crc.h"

// Implementierung der CRC-16-CCITT Berechnung für AX.25
uint16_t crc16_ccitt(const uint8_t *data, size_t length) {
    uint16_t crc = AX25_FCS_INIT;

    for (size_t i = 0; i < length; i++) {
        crc ^= (uint16_t)data[i] << 8;

        for (int j = 0; j < 8; j++) {
            if (crc & 0x8000) {
                crc = (crc << 1) ^ CRC16_CCITT_POLY;
            } else {
                crc <<= 1;
            }
        }
    }

    // Die finale CRC wird invertiert (XOR mit 0xFFFF)
    return ~crc;
}