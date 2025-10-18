// win_impl.h
// Author: katahiromz
// License: MIT
#pragma once

#include <string>
#include <stdint.h>
#include "../tstring.h"
#include "../TextFiler.h"

namespace khmz {
    bool bin_to_text_on_win(const binary_t& bin, std::wstring& text, ENCODING enc);
    bool text_to_bin_on_win(const std::wstring& text, binary_t& bin, ENCODING enc);
} // namespace khmz
