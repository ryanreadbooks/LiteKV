#include <cstring>
#include "encoding.h"

size_t Time33Hash(const char* str, size_t len) {
  unsigned long hash = 5381;
  for (size_t i = 0; i < len; ++i) {
    hash = ((hash << 5) + hash) + (unsigned)str[i];
  }
  return hash;
}

uint8_t EncodeVarUnsignedInt64(uint64_t value, unsigned char* buf) {
  uint8_t* ptr = reinterpret_cast<uint8_t*>(buf);
  uint8_t cnt = 0;
  while (value >= 0b10000000) {
    *(ptr++) = value | 0b10000000;
    value >>= 7;
    cnt++;
  }
  *(ptr++) = static_cast<uint8_t>(value);
  cnt++;
  return cnt;
}

uint64_t DecodeVarUnsignedInt64(unsigned char* enc) {
  uint64_t value = 0;
  uint8_t i = 0; /* uint64_t can be encoded in 1~10 bytes */
  for (uint32_t shift = 0; shift <= 63 && i < 10; shift += 7, ++i) {
    uint64_t byte = *(reinterpret_cast<uint8_t*>(enc++));
    if (byte & 0b10000000) {
      value |= ((byte & 0b01111111) << shift);
    } else {
      value |= (byte << shift);
      return value;
    }
  }
  return value;
}

uint8_t EstimateUnsignedIntBytes(uint64_t value) {
  if (value <= UINT64_BYTE_1_MAX) {
    return 1;
  } else if (value <= UINT64_BYTE_2_MAX) {
    return 2;
  } else if (value <= UINT64_BYTE_3_MAX) {
    return 3;
  } else if (value <= UINT64_BYTE_4_MAX) {
    return 4;
  } else if (value <= UINT64_BYTE_5_MAX) {
    return 5;
  } else if (value <= UINT64_BYTE_6_MAX) {
    return 6;
  } else if (value <= UINT64_BYTE_7_MAX) {
    return 7;
  } else if (value <= UINT64_BYTE_8_MAX) {
    return 8;
  } else if (value <= UINT64_BYTE_9_MAX) {
    return 9;
  } else {
    return 10;
  }
}

uint8_t EncodeVarSignedInt64(int64_t value, unsigned char* buf) {
  return EncodeVarUnsignedInt64(value, buf);
}

int64_t DecodeVarSignedInt64(unsigned char* buf) { return DecodeVarUnsignedInt64(buf); }

void EncodeFixed64BitInteger(int64_t value, unsigned char* buf) {
  unsigned char* ptr = reinterpret_cast<unsigned char*>(&value);
  memcpy(buf, ptr, 8);
}