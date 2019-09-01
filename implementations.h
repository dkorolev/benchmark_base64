#pragma once

#include "ensure_implementations_compile.h"

IMPLEMENTATION(vanilla) {
  std::string placeholder;
  current::strings::Chunk DoEncode(current::strings::Chunk c) { return current::ZeroCopyBase64Encode(c, placeholder); }
  current::strings::Chunk DoDecode(current::strings::Chunk c) { return current::ZeroCopyBase64Decode(c, placeholder); }
};
