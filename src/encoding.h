#ifndef __ENCODING_H__
#define __ENCODING_H__

#include <cstddef>
#include <cstdint>

#define UINT64_BYTE_1_MAX (1ul << 7) - 1
#define UINT64_BYTE_2_MAX (1ul << 14) - 1
#define UINT64_BYTE_3_MAX (1ul << 21) - 1
#define UINT64_BYTE_4_MAX (1ul << 28) - 1
#define UINT64_BYTE_5_MAX (1ul << 35) - 1
#define UINT64_BYTE_6_MAX (1ul << 42) - 1
#define UINT64_BYTE_7_MAX (1ul << 49) - 1
#define UINT64_BYTE_8_MAX (1ul << 56) - 1
#define UINT64_BYTE_9_MAX (1ul << 63) - 1

/**
 * @brief Bernstein's hash for string
 */
size_t Time33Hash(const char* str, size_t len);

/**
 * @brief encode a uint64_t value into buf
 *
 * @param value the uint64_t value to be encoded
 * @param buf the buf for encoded value
 * @return uint8_t length of the encoded buf
 */
uint8_t EncodeVarUnsignedInt64(uint64_t value, unsigned char* buf);

/**
 * @brief decode a buf to retrieve a uint64_t value
 *
 * @param enc encoded buffer
 * @return uint64_t decoded value
 */
uint64_t DecodeVarUnsignedInt64(unsigned char* enc);

/**
 * @brief calculate how many bytes needed to encode value
 *
 * @param value the value to be encoded
 * @return uint8_t the number of bytes needed to encode value
 */
uint8_t EstimateUnsignedIntBytes(uint64_t value);

/**
 * @brief encode a int64_t value into buf
 *
 * @param value the int64_t value to be encoded
 * @param buf the buf for encoded value
 * @return uint8_t length of the encoded buf
 */
uint8_t EncodeVarSignedInt64(int64_t value, unsigned char* buf);

/**
 * @brief decode a buf to retrieve a int64_t value
 *
 * @param buf encoded buffer
 * @return int64_t decoded value
 */
int64_t DecodeVarSignedInt64(unsigned char* buf);

/**
 * @brief encode a 64-bit integer into a 8-bytes buffer
 * 
 * @param value encoding 64-bit value
 * @param buf encoded buffer
 */
void EncodeFixed64BitInteger(int64_t value, unsigned char* buf);

#endif  // __ENCODING_H__