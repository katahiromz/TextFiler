#pragma once

#include <string>
#include <stdint.h>
#include "tstring.h"
#include "TextFiler.h"

bool bin_to_text_on_win(const khmz::binary_t& bin, std::wstring& text, khmz::ENCODING enc);
bool text_to_bin_on_win(const std::wstring& text, khmz::binary_t& bin, khmz::ENCODING enc);
