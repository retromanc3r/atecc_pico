#include "atecc_pico.h"

// Calculate CRC16-CCITT (0x8005) checksum (little-endian) taken from CryptoAuthLib
void calc_crc16_ccitt(size_t length, const uint8_t *data, uint8_t *crc_le) {
    size_t counter;
    uint16_t crc_register = 0;
    uint16_t polynom = 0x8005;
    uint8_t shift_register;
    uint8_t data_bit, crc_bit;

    for (counter = 0; counter < length; counter++) {
        for (shift_register = 0x01; shift_register > 0x00u; shift_register <<= 1) {
            data_bit = ((data[counter] & shift_register) != 0u) ? 1u : 0u;
            crc_bit = (uint8_t)(crc_register >> 15);
            crc_register <<= 1;
            if (data_bit != crc_bit) {
                crc_register ^= polynom;
            }
        }
    }
    crc_le[0] = (uint8_t)(crc_register & 0x00FFu);
    crc_le[1] = (uint8_t)(crc_register >> 8u);
}

// Function to compute CRC using calcCRC function
void compute_crc(uint8_t length, uint8_t *data, uint8_t *crc) {
    calc_crc16_ccitt(length, data, crc);
}

// Validates CRC of the received response
bool validate_crc(uint8_t *response, size_t length) {
    if (length < 3) return false; // Not enough bytes for CRC
    uint8_t computed_crc[2];
    compute_crc(length - 2, response, computed_crc);
    return (computed_crc[0] == response[length - 2] && computed_crc[1] == response[length - 1]);
}