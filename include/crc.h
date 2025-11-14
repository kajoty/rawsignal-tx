#ifndef CRC_H
#define CRC_H

#include <stdint.h>
#include <stddef.h>

#define CRC16_CCITT_POLY 0x1021
#define AX25_FCS_INIT 0xFFFF

uint16_t crc16_ccitt(const uint8_t *data, size_t length);

#endif // CRC_H