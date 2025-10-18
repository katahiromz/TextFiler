// posix_impl.h
// Author: katahiromz
// License: MIT
#pragma once

#include <string>
#include "pstdint.h"

bool is_system_little_endian();
bool utf8_validate(const char *ptr, size_t size);
bool u16_to_utf8(const uint16_t *u16, size_t u16len, std::string& out);
bool utf8_to_u16(const char *ptr, size_t size, std::vector<uint16_t>& out);

bool bin_to_text_on_posix(const binary_t& bin, std::string& text, ENCODING enc);
bool text_to_bin_on_posix(const std::string& text, binary_t& bin, ENCODING enc);
