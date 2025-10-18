#include <stddef.h>
#include <stdint.h>
#include "../TextFiler.h"

bool is_system_little_endian() {
    uint16_t v = 1;
    return (*(uint8_t *)&v) == 1;
}

/* UTF-8 validation for POSIX */
bool utf8_validate(const char *ptr, size_t size) {
    size_t i = 0;
    while (i < size) {
        byte_t c = (byte_t)ptr[i];
        if (c <= 0x7F) {
            // ASCII
            i++;
            continue;
        } else if ((c >> 5) == 0x6) {
            // 2-byte
            if (i + 1 >= size) return false;
            byte_t c1 = (byte_t)ptr[i+1];
            if ((c1 >> 6) != 0x2) return false;
            uint32_t code = ((c & 0x1F) << 6) | (c1 & 0x3F);
            if (code < 0x80) return false; // overlong
            i += 2;
        } else if ((c >> 4) == 0xE) {
            // 3-byte
            if (i + 2 >= size) return false;
            byte_t c1 = (byte_t)ptr[i+1];
            byte_t c2 = (byte_t)ptr[i+2];
            if ((c1 >> 6) != 0x2 || (c2 >> 6) != 0x2) return false;
            uint32_t code = ((c & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
            if (code < 0x800) return false; // overlong
            // surrogate halves are invalid in UTF-8
            if (code >= 0xD800 && code <= 0xDFFF) return false;
            i += 3;
        } else if ((c >> 3) == 0x1E) {
            // 4-byte
            if (i + 3 >= size) return false;
            byte_t c1 = (byte_t)ptr[i+1];
            byte_t c2 = (byte_t)ptr[i+2];
            byte_t c3 = (byte_t)ptr[i+3];
            if ((c1 >> 6) != 0x2 || (c2 >> 6) != 0x2 || (c3 >> 6) != 0x2) return false;
            uint32_t code = ((c & 0x07) << 18) | ((c1 & 0x3F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
            if (code < 0x10000 || code > 0x10FFFF) return false; // overlong or out of range
            i += 4;
        } else {
            return false;
        }
    }
    return true;
}

/* Helpers for UTF-8 <-> UTF-16 (u16 = uint16_t code units) on POSIX */
bool u16_to_utf8(const uint16_t *u16, size_t u16len, std::string& out) {
    out.clear();
    for (size_t i = 0; i < u16len; ++i) {
        uint32_t code;
        uint16_t w = u16[i];
        if (w >= 0xD800 && w <= 0xDBFF) {
            // high surrogate
            if (i + 1 >= u16len) return false;
            uint16_t w2 = u16[i+1];
            if (!(w2 >= 0xDC00 && w2 <= 0xDFFF)) return false;
            code = 0x10000 + (((uint32_t)(w - 0xD800) << 10) | (uint32_t)(w2 - 0xDC00));
            ++i;
        } else if (w >= 0xDC00 && w <= 0xDFFF) {
            // unexpected low surrogate
            return false;
        } else {
            code = w;
        }

        if (code <= 0x7F) {
            out.push_back((char)code);
        } else if (code <= 0x7FF) {
            out.push_back((char)(0xC0 | ((code >> 6) & 0x1F)));
            out.push_back((char)(0x80 | (code & 0x3F)));
        } else if (code <= 0xFFFF) {
            out.push_back((char)(0xE0 | ((code >> 12) & 0x0F)));
            out.push_back((char)(0x80 | ((code >> 6) & 0x3F)));
            out.push_back((char)(0x80 | (code & 0x3F)));
        } else {
            out.push_back((char)(0xF0 | ((code >> 18) & 0x07)));
            out.push_back((char)(0x80 | ((code >> 12) & 0x3F)));
            out.push_back((char)(0x80 | ((code >> 6) & 0x3F)));
            out.push_back((char)(0x80 | (code & 0x3F)));
        }
    }
    return true;
}

bool utf8_to_u16(const char *ptr, size_t size, std::vector<uint16_t>& out) {
    out.clear();
    size_t i = 0;
    while (i < size) {
        byte_t c = (byte_t)ptr[i];
        uint32_t code = 0;
        if (c <= 0x7F) {
            code = c;
            ++i;
        } else if ((c >> 5) == 0x6) {
            if (i + 1 >= size) return false;
            byte_t c1 = (byte_t)ptr[i+1];
            if ((c1 >> 6) != 0x2) return false;
            code = ((c & 0x1F) << 6) | (c1 & 0x3F);
            i += 2;
        } else if ((c >> 4) == 0xE) {
            if (i + 2 >= size) return false;
            byte_t c1 = (byte_t)ptr[i+1];
            byte_t c2 = (byte_t)ptr[i+2];
            if ((c1 >> 6) != 0x2 || (c2 >> 6) != 0x2) return false;
            code = ((c & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
            i += 3;
        } else if ((c >> 3) == 0x1E) {
            if (i + 3 >= size) return false;
            byte_t c1 = (byte_t)ptr[i+1];
            byte_t c2 = (byte_t)ptr[i+2];
            byte_t c3 = (byte_t)ptr[i+3];
            if ((c1 >> 6) != 0x2 || (c2 >> 6) != 0x2 || (c3 >> 6) != 0x2) return false;
            code = ((c & 0x07) << 18) | ((c1 & 0x3F) << 12) | ((c2 & 0x3F) << 6) | (c3 & 0x3F);
            i += 4;
        } else {
            return false;
        }

        if (code <= 0xFFFF) {
            if (code >= 0xD800 && code <= 0xDFFF) return false; // invalid
            out.push_back((uint16_t)code);
        } else if (code <= 0x10FFFF) {
            code -= 0x10000;
            uint16_t high = (uint16_t)((code >> 10) + 0xD800);
            uint16_t low = (uint16_t)((code & 0x3FF) + 0xDC00);
            out.push_back(high);
            out.push_back(low);
        } else {
            return false;
        }
    }
    return true;
}

// binary-to-narrow
bool bin_to_text_on_posix(const binary_t& bin, std::string& text, ENCODING enc) {
    text.clear();
    size_t size = bin.size();
    const byte_t *ptr = (const byte_t*)bin.c_str();

    // POSIX
    if (enc == ENCODING_BINARY)
        enc = ENCODING_DEFAULT;

    if (enc == ENCODING_UTF8_WITH_BOM) {
        if (size >= 3) {
            text.assign((const char *)(ptr + 3), size - 3);
            return true;
        } else {
            text.clear();
            return true;
        }
    } else if (enc == ENCODING_UTF8_WITHOUT_BOM) {
        text.assign((const char *)ptr, size);
        return true;
    } else if (enc == ENCODING_UTF16_LE_WITH_BOM || enc == ENCODING_UTF16_LE_WITHOUT_BOM ||
               enc == ENCODING_UTF16_BE_WITH_BOM || enc == ENCODING_UTF16_BE_WITHOUT_BOM) {
        size_t offset = 0;
        if ((enc == ENCODING_UTF16_LE_WITH_BOM || enc == ENCODING_UTF16_BE_WITH_BOM) && size >= 2) offset = 2;
        if ((size - offset) % 2 != 0) return false;
        size_t u16len = (size - offset) / 2;
        std::vector<uint16_t> u16(u16len);
        if ((enc == ENCODING_UTF16_BE_WITH_BOM) || (enc == ENCODING_UTF16_BE_WITHOUT_BOM)) {
            for (size_t i = 0; i < u16len; ++i) {
                u16[i] = (uint16_t)(((uint16_t)ptr[offset + i*2] << 8) | (uint16_t)ptr[offset + i*2 + 1]);
            }
        } else {
            for (size_t i = 0; i < u16len; ++i) {
                u16[i] = (uint16_t)((uint16_t)ptr[offset + i*2] | ((uint16_t)ptr[offset + i*2 + 1] << 8));
            }
        }
        std::string out;
        if (!u16_to_utf8(u16.empty() ? NULL : &u16[0], u16.size(), out)) return false;
        text.swap(out);
        return true;
    } else if (enc == ENCODING_ANSI || enc == ENCODING_ASCII) {
        // Treat as UTF-8 (or ASCII)
        text.assign((const char*)ptr, size);
        return true;
    }
    return false;
}

// narrow-to-binary
bool text_to_bin_on_posix(const std::string& text, binary_t& bin, ENCODING enc) {
    bin.clear();

    // POSIX: assume input text is UTF-8 (std::string)
    if (enc == ENCODING_UTF8_WITH_BOM || enc == ENCODING_UTF8_WITHOUT_BOM) {
        size_t add = (enc == ENCODING_UTF8_WITH_BOM) ? 3 : 0;
        try {
            bin.resize(add + text.size());
        } catch (...) { return false; }
        if (add) { bin[0]=0xEF; bin[1]=0xBB; bin[2]=0xBF; }
        if (!text.empty()) memcpy(&bin[add], text.c_str(), text.size());
        return true;
    } else if (enc == ENCODING_UTF16_LE_WITH_BOM || enc == ENCODING_UTF16_LE_WITHOUT_BOM ||
               enc == ENCODING_UTF16_BE_WITH_BOM || enc == ENCODING_UTF16_BE_WITHOUT_BOM) {
        std::vector<uint16_t> u16;
        if (!utf8_to_u16(text.c_str(), text.size(), u16)) return false;
        size_t add = ((enc == ENCODING_UTF16_LE_WITH_BOM) || (enc == ENCODING_UTF16_BE_WITH_BOM)) ? 2 : 0;
        try {
            bin.resize(add + u16.size() * 2);
        } catch (...) { return false; }
        size_t pos = 0;
        if (add) {
            if (enc == ENCODING_UTF16_LE_WITH_BOM) { bin[0]=0xFF; bin[1]=0xFE; } else { bin[0]=0xFE; bin[1]=0xFF; }
            pos = 2;
        }
        if ((enc == ENCODING_UTF16_LE_WITH_BOM) || (enc == ENCODING_UTF16_LE_WITHOUT_BOM)) {
            for (size_t i = 0; i < u16.size(); ++i) {
                uint16_t v = u16[i];
                bin[pos + i*2] = (uint8_t)(v & 0xFF);
                bin[pos + i*2 + 1] = (uint8_t)((v >> 8) & 0xFF);
            }
            return true;
        } else {
            for (size_t i = 0; i < u16.size(); ++i) {
                uint16_t v = u16[i];
                bin[pos + i*2] = (uint8_t)((v >> 8) & 0xFF);
                bin[pos + i*2 + 1] = (uint8_t)(v & 0xFF);
            }
            return true;
        }
    } else if (enc == ENCODING_ANSI || enc == ENCODING_ASCII) {
        // Treat as UTF-8 bytes
        try {
            bin.assign(text.begin(), text.end());
        } catch (...) { return false; }
        return true;
    }
    return false;
}
