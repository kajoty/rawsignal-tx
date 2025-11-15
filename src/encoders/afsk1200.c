#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>
#include <ctype.h>
#include <math.h>

#include "../../include/signal_generator.h"
#include "../../include/crc.h"
#include "../../include/encoders/afsk1200.h"

#define AX25_FLAG 0x7E
#define AX25_ADDR_LEN 7
#define AX25_CONTROL_UI 0x03
#define AX25_PID_NO_PROTOCOL 0xF0
#define SAMPLES_PER_BIT (SAMPLE_RATE / AFSK_BAUD_RATE)
#define MAX_AMPLITUDE MAX_PCM_VALUE

static double current_phase = 0.0;
static int last_nrzi_state = 1;

static void emit_bit(int bit) {
    if (bit == 0) {
        last_nrzi_state = !last_nrzi_state;
    }
    
    double freq = last_nrzi_state ? AFSK_MARK_FREQ : AFSK_SPACE_FREQ;
    double phase_step = 2.0 * M_PI * freq / SAMPLE_RATE;
    
    for (int i = 0; i < SAMPLES_PER_BIT; i++) {
        int16_t sample = (int16_t)(MAX_AMPLITUDE * sin(current_phase));
        fwrite(&sample, sizeof(int16_t), 1, stdout);
        current_phase += phase_step;
        if (current_phase >= 2.0 * M_PI) current_phase -= 2.0 * M_PI;
    }
}

static void emit_byte_with_stuffing(uint8_t byte) {
    static int ones = 0;
    for (int i = 0; i < 8; i++) {
        int bit = (byte >> i) & 1;
        emit_bit(bit);
        ones = bit ? ones + 1 : 0;
        if (ones == 5) {
            emit_bit(0);
            ones = 0;
        }
    }
}

static void emit_flags(int count) {
    for (int i = 0; i < count; i++) {
        for (int j = 0; j < 8; j++) {
            emit_bit((AX25_FLAG >> j) & 1);
        }
    }
}

int rs_encode_afsk1200(const char* tx_call, const char* dest_call, const char* message) {
    current_phase = 0.0;
    last_nrzi_state = 1;
    
    size_t msg_len = strlen(message);
    size_t frame_len = 14 + 2 + msg_len + 2;
    uint8_t *frame = (uint8_t *)malloc(frame_len);
    if (!frame) return 1;
    
    size_t offset = 0;
    
    // Destination: 6 chars + SSID
    for (size_t i = 0; i < 6; i++) {
        char c = (i < strlen(dest_call) ? toupper((unsigned char)dest_call[i]) : ' ');
        frame[offset++] = (uint8_t)(c << 1);
    }
    frame[offset++] = 0x60;
    
    // Source: 6 chars + SSID + End-of-Address
    for (size_t i = 0; i < 6; i++) {
        char c = (i < strlen(tx_call) ? toupper((unsigned char)tx_call[i]) : ' ');
        frame[offset++] = (uint8_t)(c << 1);
    }
    frame[offset++] = 0x61;
    
    // Control + PID
    frame[offset++] = 0x03;
    frame[offset++] = 0xF0;
    
    // Message
    memcpy(frame + offset, message, msg_len);
    offset += msg_len;
    
    // FCS (already inverted by crc16_ccitt)
    uint16_t fcs_value = crc16_ccitt(frame, frame_len - 2);
    frame[offset++] = (uint8_t)(fcs_value & 0xFF);
    frame[offset++] = (uint8_t)(fcs_value >> 8);
    
    // Output: 16 Pre-Amble + 1 Start + Data + 1 End (per AX.25)
    emit_flags(16);
    emit_flags(1);
    for (size_t i = 0; i < frame_len; i++) {
        emit_byte_with_stuffing(frame[i]);
    }
    emit_flags(1);
    
    free(frame);
    return 0;
}