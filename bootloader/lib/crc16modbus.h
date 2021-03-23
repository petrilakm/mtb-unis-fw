// The _bit and _byte routines return the CRC of the len bytes at mem,
// applied to the previous CRC value, crc. If mem is NULL, then the other
// arguments are ignored, and the initial CRC, i.e. the CRC of zero bytes, is
// returned. Those routines will all return the same result, differing only in
// speed and code complexity. The _rem routine returns the CRC of the remaining
// bits in the last byte, for when the number of bits in the message is not a
// multiple of eight. The low bits bits of the low byte of val are applied to
// crc. bits must be in 0..8.

// This code is based on automatically-generated code from:
// ‹https://github.com/madler/crcany›

#include <stddef.h>
#include <stdint.h>

// Compute the CRC a bit at a time.
uint16_t crc16modbus_bit(uint16_t crc, void const *mem, size_t len);

// Compute the CRC of the low bits bits in val.
uint16_t crc16modbus_rem(uint16_t crc, unsigned val, unsigned bits);

// Compute the CRC a byte at a time.
uint16_t crc16modbus_bytes(uint16_t crc, void const *mem, size_t len);
uint16_t crc16modbus_byte(uint16_t crc, uint8_t byte);
