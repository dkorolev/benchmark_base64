#pragma once

#include "ensure_implementations_compile.h"

IMPLEMENTATION(vanilla) {
  std::string placeholder;
  current::strings::Chunk DoEncode(current::strings::Chunk c) { return current::ZeroCopyBase64Encode(c, placeholder); }
  current::strings::Chunk DoDecode(current::strings::Chunk c) { return current::ZeroCopyBase64Decode(c, placeholder); }
};

IMPLEMENTATION(fast) {
  std::string placeholder;
  current::strings::Chunk DoEncode(current::strings::Chunk input) {
    const char* map = current::base64::encode_map;

    const size_t result_size = 4 * (input.length() / 3) + ((input.length() % 3) ? 4 : 0);
    placeholder.resize(result_size);  // Effectively, `realloc()`, and often a no-op if it shrinks.
    char* ptr = &placeholder[0];
    char* const end = ptr + result_size;

    uint8_t const* in = reinterpret_cast<uint8_t const*>(input.c_str());  // Must be unsigned type. -- D.K.
    uint8_t const* const in_end = in + input.length();

#if 1
#define DO_BASE64ENCODE_THREE_BYTES \
  ptr[3] = map[u32 & 0x3f];         \
  u32 >>= 6;                        \
  ptr[2] = map[u32 & 0x3f];         \
  u32 >>= 6;                        \
  ptr[1] = map[u32 & 0x3f];         \
  u32 >>= 6;                        \
  ptr[0] = map[u32 & 0x3f];
#else
#define DO_BASE64ENCODE_THREE_BYTES \
  ptr[0] = map[(u32 >> 18) & 0x3f]; \
  ptr[1] = map[(u32 >> 12) & 0x3f]; \
  ptr[2] = map[(u32 >> 6) & 0x3f];  \
  ptr[3] = map[u32 & 0x3f];
#endif

#define BASE64ENCODE_THREE_BYTES            \
  u32 = static_cast<uint32_t>(*in++) << 16; \
  u32 += static_cast<uint32_t>(*in++) << 8; \
  u32 += static_cast<uint32_t>(*in++);      \
  DO_BASE64ENCODE_THREE_BYTES               \
  ptr += 4

    uint32_t u32;
    size_t total_threebyte_blocks = input.length() / 3;
    size_t total_30bytes_blocks = total_threebyte_blocks / 10;
    total_threebyte_blocks -= total_30bytes_blocks * 10;
    while (total_30bytes_blocks--) {
      BASE64ENCODE_THREE_BYTES;
      BASE64ENCODE_THREE_BYTES;
      BASE64ENCODE_THREE_BYTES;
      BASE64ENCODE_THREE_BYTES;
      BASE64ENCODE_THREE_BYTES;
      BASE64ENCODE_THREE_BYTES;
      BASE64ENCODE_THREE_BYTES;
      BASE64ENCODE_THREE_BYTES;
      BASE64ENCODE_THREE_BYTES;
      BASE64ENCODE_THREE_BYTES;
    }
    while (total_threebyte_blocks--) {
      BASE64ENCODE_THREE_BYTES;
    }
#undef BASE64ENCODE_THREE_BYTES
#undef DO_BASE64ENCODE_THREE_BYTES

    uint16_t acc = 0u;
    uint8_t nbits = 0;
    while (in != in_end) {
      acc = (acc << 8) + (*in++);
      nbits += 8;
      while (nbits >= 6) {
        nbits -= 6;
        *ptr++ = map[(acc >> nbits) & 0x3F];
      }
    }

    if (nbits > 0) {
      *ptr++ = map[((acc << 6) >> nbits) & 0x3F];
    }

    while (ptr != end) {
      *ptr++ = '=';
    }

    return placeholder;
  }

  current::strings::Chunk DoDecode(current::strings::Chunk encoded) {
    uint8_t const* map = current::base64::decode_map;

    placeholder.resize(3 * encoded.length() / 4);  // Effectively, `realloc()`, and often a no-op if it shrinks.
    char* ptr = &placeholder[0];

    char const* in = encoded.c_str();
    char const* const in_end = in + encoded.length();

#define BASE64DECODE_FOUR_BYTES                                         \
  u32 = static_cast<uint32_t>(map[static_cast<uint8_t>(*in++)]) << 18;  \
  u32 += static_cast<uint32_t>(map[static_cast<uint8_t>(*in++)]) << 12; \
  u32 += static_cast<uint32_t>(map[static_cast<uint8_t>(*in++)]) << 6;  \
  u32 += static_cast<uint32_t>(map[static_cast<uint8_t>(*in++)]);       \
  *ptr++ = static_cast<char>((u32 >> 16) & 0xFF);                       \
  *ptr++ = static_cast<char>((u32 >> 8) & 0xFF);                        \
  *ptr++ = static_cast<char>(u32 & 0xFF)

    uint32_t u32;
    size_t total_fourbyte_blocks = (encoded.length() >> 2);
    if (total_fourbyte_blocks) {
      --total_fourbyte_blocks;  // Can have pad chars at the end.
    }
    size_t totaL_32bytes_blocks = total_fourbyte_blocks / 8;
    total_fourbyte_blocks -= totaL_32bytes_blocks * 8;
    while (totaL_32bytes_blocks--) {
      BASE64DECODE_FOUR_BYTES;
      BASE64DECODE_FOUR_BYTES;
      BASE64DECODE_FOUR_BYTES;
      BASE64DECODE_FOUR_BYTES;
      BASE64DECODE_FOUR_BYTES;
      BASE64DECODE_FOUR_BYTES;
      BASE64DECODE_FOUR_BYTES;
      BASE64DECODE_FOUR_BYTES;
    }
    while (total_fourbyte_blocks--) {
      BASE64DECODE_FOUR_BYTES;
    }
#undef BASE64DECODE_FOUR_BYTES

    uint16_t acc = 0u;
    uint8_t nbits = 0u;
    while (in != in_end) {
      const char c = *in++;
      if (c == current::base64::pad_char) {
        placeholder.resize(ptr - &placeholder[0]);
        break;
      }
      acc = (acc << 6) + map[static_cast<uint8_t>(c)];
      nbits += 6;
      if (nbits >= 8) {
        nbits -= 8;
        *ptr++ = static_cast<char>((acc >> nbits) & 0xFF);
      }
    }

    return placeholder;
  }
};
