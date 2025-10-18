// TextFiler_impl.cpp
#include "TextFiler.h"
#include "TextFiler_impl.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <vector>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
    #include <windows.h>
#else
    #include <errno.h>
#endif

using namespace khmz;

static const khmz::ENCODING c_default_encoding = khmz::ENCODING_UTF8;

#ifndef _WIN32

static inline bool is_system_little_endian() {
    uint16_t v = 1;
    return (*(uint8_t *)&v) == 1;
}

/* UTF-8 validation for POSIX */
static bool utf8_validate(const char *ptr, size_t size) {
    size_t i = 0;
    while (i < size) {
        unsigned char c = (unsigned char)ptr[i];
        if (c <= 0x7F) {
            // ASCII
            i++;
            continue;
        } else if ((c >> 5) == 0x6) {
            // 2-byte
            if (i + 1 >= size) return false;
            unsigned char c1 = (unsigned char)ptr[i+1];
            if ((c1 >> 6) != 0x2) return false;
            uint32_t code = ((c & 0x1F) << 6) | (c1 & 0x3F);
            if (code < 0x80) return false; // overlong
            i += 2;
        } else if ((c >> 4) == 0xE) {
            // 3-byte
            if (i + 2 >= size) return false;
            unsigned char c1 = (unsigned char)ptr[i+1];
            unsigned char c2 = (unsigned char)ptr[i+2];
            if ((c1 >> 6) != 0x2 || (c2 >> 6) != 0x2) return false;
            uint32_t code = ((c & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
            if (code < 0x800) return false; // overlong
            // surrogate halves are invalid in UTF-8
            if (code >= 0xD800 && code <= 0xDFFF) return false;
            i += 3;
        } else if ((c >> 3) == 0x1E) {
            // 4-byte
            if (i + 3 >= size) return false;
            unsigned char c1 = (unsigned char)ptr[i+1];
            unsigned char c2 = (unsigned char)ptr[i+2];
            unsigned char c3 = (unsigned char)ptr[i+3];
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
static bool u16_to_utf8(const uint16_t *u16, size_t u16len, std::string &out) {
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

static bool utf8_to_u16(const char *ptr, size_t size, std::vector<uint16_t> &out) {
    out.clear();
    size_t i = 0;
    while (i < size) {
        unsigned char c = (unsigned char)ptr[i];
        uint32_t code = 0;
        if (c <= 0x7F) {
            code = c;
            ++i;
        } else if ((c >> 5) == 0x6) {
            if (i + 1 >= size) return false;
            unsigned char c1 = (unsigned char)ptr[i+1];
            if ((c1 >> 6) != 0x2) return false;
            code = ((c & 0x1F) << 6) | (c1 & 0x3F);
            i += 2;
        } else if ((c >> 4) == 0xE) {
            if (i + 2 >= size) return false;
            unsigned char c1 = (unsigned char)ptr[i+1];
            unsigned char c2 = (unsigned char)ptr[i+2];
            if ((c1 >> 6) != 0x2 || (c2 >> 6) != 0x2) return false;
            code = ((c & 0x0F) << 12) | ((c1 & 0x3F) << 6) | (c2 & 0x3F);
            i += 3;
        } else if ((c >> 3) == 0x1E) {
            if (i + 3 >= size) return false;
            unsigned char c1 = (unsigned char)ptr[i+1];
            unsigned char c2 = (unsigned char)ptr[i+2];
            unsigned char c3 = (unsigned char)ptr[i+3];
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

#endif  // ndef _WIN32

/* Byte-swap u16 array in-place */
static void swap_bytes_u16_inplace(uint16_t *ptr, size_t len) {
    for (size_t i = 0; i < len; ++i) {
        uint16_t v = ptr[i];
        ptr[i] = (uint16_t)(((v & 0xFF) << 8) | ((v >> 8) & 0xFF));
    }
}

/* Implementations of conversion helpers */

// binary-to-wide
static bool b_to_w(const binary_t& bin, std::wstring& text, ENCODING encoding) {
    text.clear();
    size_t size = bin.size();
    const byte_t *ptr = (const byte_t*)(bin.data());

    ENCODING enc = encoding;

#ifdef _WIN32
    if (enc == ENCODING_BINARY)
        encoding = c_default_encoding;

    if (enc == ENCODING_UTF8_WITH_BOM) {
        if (size >= 3) {
            const char *p = (const char *)(ptr + 3);
            int len = (int)size - 3;
            int wideLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, p, len, NULL, 0);
            if (wideLen <= 0) return false;
            text.resize(wideLen);
            MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, p, len, &text[0], wideLen);
            return true;
        }
        return true;
    } else if (enc == ENCODING_UTF8_WITHOUT_BOM) {
        const char *p = (const char *)ptr;
        int len = (int)size;
        int wideLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, p, len, NULL, 0);
        if (wideLen <= 0) return false;
        text.resize(wideLen);
        MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, p, len, &text[0], wideLen);
        return true;
    } else if (enc == ENCODING_UTF16_LE_WITH_BOM || enc == ENCODING_UTF16_LE_WITHOUT_BOM ||
               enc == ENCODING_UTF16_BE_WITH_BOM || enc == ENCODING_UTF16_BE_WITHOUT_BOM) {
        size_t offset = 0;
        if ((enc == ENCODING_UTF16_LE_WITH_BOM || enc == ENCODING_UTF16_BE_WITH_BOM) && size >= 2)
            offset = 2;
        if ((size - offset) % 2 != 0) return false;
        size_t u16len = (size - offset) / 2;
        if (u16len == 0) {
            text.clear();
            return true;
        }
        // On Windows wchar_t is UTF-16 (2 bytes)
        const uint16_t *u16ptr = (const uint16_t *)(ptr + offset);
        if ((enc == ENCODING_UTF16_BE_WITH_BOM) || (enc == ENCODING_UTF16_BE_WITHOUT_BOM)) {
            // need to swap to little endian on little-endian systems
            text.resize(u16len);
            for (size_t i = 0; i < u16len; ++i) {
                uint16_t v = ((uint16_t)ptr[offset + i*2] << 8) | (uint16_t)ptr[offset + i*2 + 1];
                text[i] = (wchar_t)v;
            }
            return true;
        } else {
            // UTF-16 LE
            text.assign((const wchar_t*)u16ptr, (const wchar_t*)u16ptr + u16len);
            return true;
        }
    } else if (enc == ENCODING_ANSI || enc == ENCODING_ASCII) {
        const char *p = (const char *)ptr;
        int len = (int)size;
        // treat ASCII as CP_UTF8 for safety in many cases? but for ANSI we should use CP_ACP
        UINT codePage = (enc == ENCODING_ANSI) ? CP_ACP : CP_UTF8;
        int wideLen = MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS, p, len, NULL, 0);
        if (wideLen <= 0 && enc == ENCODING_ANSI) {
            // Fallback: try without MB_ERR_INVALID_CHARS
            wideLen = MultiByteToWideChar(codePage, 0, p, len, NULL, 0);
            if (wideLen <= 0) return false;
            text.resize(wideLen);
            MultiByteToWideChar(codePage, 0, p, len, &text[0], wideLen);
            return true;
        } else if (wideLen <= 0) {
            return false;
        }
        text.resize(wideLen);
        MultiByteToWideChar(codePage, MB_ERR_INVALID_CHARS, p, len, &text[0], wideLen);
        return true;
    }
    return false;
#else
    // POSIX implementations (narrow/wide split usually means UNICODE is not defined on POSIX,
    // but provide general implementations)
    if (enc == ENCODING_BINARY)
        enc = c_default_encoding;

    if (enc == ENCODING_UTF8_WITH_BOM) {
        if (size >= 3) {
            text.assign((const char *)(ptr + 3), size - 3);
            // Note: on POSIX std::wstring is not used here; but implement conversion from UTF-8 to wchar_t
            // Convert UTF-8 string to wstring (using mbstowcs is locale-dependent). We'll do a simple conversion
            // via utf8 -> u16 -> wide if wide is 2 bytes; otherwise perform narrow->wide naive conversion.
        } else {
            text.clear();
        }
        // convert UTF-8 narrow string to wstring (assuming wchar_t can hold Unicode)
        std::vector<uint16_t> u16;
        if (!utf8_to_u16(text.c_str(), text.size(), u16)) {
            return false;
        }
        // Convert u16 -> wstring (assuming wchar_t is 4 bytes on typical Linux)
        text.clear();
        std::wstring wout;
        if (sizeof(wchar_t) == 2) {
            wout.resize(u16.size());
            for (size_t i = 0; i < u16.size(); ++i) wout[i] = (wchar_t)u16[i];
        } else { // 4 bytes
            // expand surrogates
            for (size_t i = 0; i < u16.size(); ++i) {
                uint16_t w = u16[i];
                if (w >= 0xD800 && w <= 0xDBFF) {
                    if (i + 1 >= u16.size()) return false;
                    uint16_t w2 = u16[++i];
                    uint32_t code = 0x10000 + (((uint32_t)(w - 0xD800) << 10) | (uint32_t)(w2 - 0xDC00));
                    wout.push_back((wchar_t)code);
                } else {
                    wout.push_back((wchar_t)w);
                }
            }
        }
        text.clear();
        text.assign(wout.begin(), wout.end());
        return true;
    } else if (enc == ENCODING_UTF8_WITHOUT_BOM) {
        std::string s((const char *)ptr, size);
        std::vector<uint16_t> u16;
        if (!utf8_to_u16(s.c_str(), s.size(), u16)) return false;
        std::wstring wout;
        if (sizeof(wchar_t) == 2) {
            wout.resize(u16.size());
            for (size_t i = 0; i < u16.size(); ++i) wout[i] = (wchar_t)u16[i];
        } else {
            for (size_t i = 0; i < u16.size(); ++i) {
                uint16_t w = u16[i];
                if (w >= 0xD800 && w <= 0xDBFF) {
                    if (i + 1 >= u16.size()) return false;
                    uint16_t w2 = u16[++i];
                    uint32_t code = 0x10000 + (((uint32_t)(w - 0xD800) << 10) | (uint32_t)(w2 - 0xDC00));
                    wout.push_back((wchar_t)code);
                } else {
                    wout.push_back((wchar_t)w);
                }
            }
        }
        text.assign(wout.begin(), wout.end());
        return true;
    } else if (enc == ENCODING_UTF16_LE_WITH_BOM || enc == ENCODING_UTF16_LE_WITHOUT_BOM ||
               enc == ENCODING_UTF16_BE_WITH_BOM || enc == ENCODING_UTF16_BE_WITHOUT_BOM) {
        size_t offset = 0;
        if ((enc == ENCODING_UTF16_LE_WITH_BOM || enc == ENCODING_UTF16_BE_WITH_BOM) && size >= 2) offset = 2;
        if ((size - offset) % 2 != 0) return false;
        size_t u16len = (size - offset) / 2;
        if (u16len == 0) {
            text.clear();
            return true;
        }
        std::vector<uint16_t> u16(u16len);
        if ((enc == ENCODING_UTF16_BE_WITH_BOM) || (enc == ENCODING_UTF16_BE_WITHOUT_BOM)) {
            for (size_t i = 0; i < u16len; ++i) {
                u16[i] = (uint16_t)(((uint16_t)ptr[offset + i*2] << 8) | (uint16_t)ptr[offset + i*2 + 1]);
            }
        } else {
            for (size_t i = 0; i < u16len; ++i) {
                u16[i] = (uint16_t)((uint16_t)ptr[offset + i*2] | ((uint16_t)ptr[offset + i*2 + 1] << 8));
            }
            if (!is_system_little_endian()) {
                // host is big-endian; swap
                for (size_t i = 0; i < u16len; ++i) {
                    uint16_t v = u16[i];
                    u16[i] = (uint16_t)(((v & 0xFF) << 8) | ((v >> 8) & 0xFF));
                }
            }
        }
        // convert u16 -> wstring
        std::wstring wout;
        if (sizeof(wchar_t) == 2) {
            wout.resize(u16len);
            for (size_t i = 0; i < u16len; ++i) wout[i] = (wchar_t)u16[i];
        } else {
            // wchar_t is 4 bytes
            for (size_t i = 0; i < u16len; ++i) {
                uint16_t w = u16[i];
                if (w >= 0xD800 && w <= 0xDBFF) {
                    if (i + 1 >= u16len) return false;
                    uint16_t w2 = u16[++i];
                    uint32_t code = 0x10000 + (((uint32_t)(w - 0xD800) << 10) | (uint32_t)(w2 - 0xDC00));
                    wout.push_back((wchar_t)code);
                } else {
                    wout.push_back((wchar_t)w);
                }
            }
        }
        text.assign(wout.begin(), wout.end());
        return true;
    } else if (enc == ENCODING_ANSI || enc == ENCODING_ASCII) {
        // On POSIX treat as UTF-8 (ASCII) bytes, convert to wstring using utf8_to_u16 then widen
        std::string s((const char *)ptr, size);
        std::vector<uint16_t> u16;
        if (!utf8_to_u16(s.c_str(), s.size(), u16)) return false;
        std::wstring wout;
        if (sizeof(wchar_t) == 2) {
            wout.resize(u16.size());
            for (size_t i = 0; i < u16.size(); ++i) wout[i] = (wchar_t)u16[i];
        } else {
            for (size_t i = 0; i < u16.size(); ++i) wout.push_back((wchar_t)u16[i]);
        }
        text.assign(wout.begin(), wout.end());
        return true;
    }
    return false;
#endif
}

// binary-to-narrow
static bool b_to_a(const binary_t& bin, std::string& text, ENCODING encoding) {
    text.clear();
    size_t size = bin.size();
    const byte_t *ptr = (const byte_t*)(bin.data());
    ENCODING enc = encoding;

#ifdef _WIN32
    if (enc == ENCODING_BINARY) return false;

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
        // Convert UTF-16 to UTF-8
        std::wstring w;
        if (!b_to_w(bin, w, enc)) return false;
        if (w.empty()) { text.clear(); return true; }
        int utf8len = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), NULL, 0, NULL, NULL);
        if (utf8len <= 0) return false;
        text.resize(utf8len);
        WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &text[0], utf8len, NULL, NULL);
        return true;
    } else if (enc == ENCODING_ANSI || enc == ENCODING_ASCII) {
        // Convert ANSI to UTF-8 (so internal narrow string becomes UTF-8)
        int utf8len = MultiByteToWideChar(CP_ACP, 0, (const char*)ptr, (int)size, NULL, 0);
        if (utf8len <= 0) {
            // maybe ASCII
            text.assign((const char*)ptr, size);
            return true;
        }
        std::wstring w;
        w.resize(utf8len);
        MultiByteToWideChar(CP_ACP, 0, (const char*)ptr, (int)size, &w[0], utf8len);
        int outlen = WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), NULL, 0, NULL, NULL);
        if (outlen <= 0) return false;
        text.resize(outlen);
        WideCharToMultiByte(CP_UTF8, 0, w.c_str(), (int)w.size(), &text[0], outlen, NULL, NULL);
        return true;
    }
    return false;
#else
    // POSIX
    if (enc == ENCODING_BINARY) return false;

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
#endif
}

// wide-to-binary
static bool w_to_b(const std::wstring& text, binary_t& bin, ENCODING encoding) {
    bin.clear();
    ENCODING enc = encoding;

#ifdef _WIN32
    if (enc == ENCODING_UTF8_WITH_BOM || enc == ENCODING_UTF8_WITHOUT_BOM) {
        int utf8len = WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), NULL, 0, NULL, NULL);
        if (utf8len < 0) return false;
        size_t add = (enc == ENCODING_UTF8_WITH_BOM) ? 3 : 0;
        try {
            bin.resize(add + utf8len);
        } catch (...) { return false; }
        size_t pos = 0;
        if (add) {
            bin[0] = 0xEF; bin[1] = 0xBB; bin[2] = 0xBF;
            pos = 3;
        }
        if (utf8len > 0)
            WideCharToMultiByte(CP_UTF8, 0, text.c_str(), (int)text.size(), &bin[pos], utf8len, NULL, NULL);
        return true;
    } else if (enc == ENCODING_UTF16_LE_WITH_BOM || enc == ENCODING_UTF16_LE_WITHOUT_BOM ||
               enc == ENCODING_UTF16_BE_WITH_BOM || enc == ENCODING_UTF16_BE_WITHOUT_BOM) {
        // On Windows wchar_t is 2 bytes (UTF-16). Copy directly for LE, swap for BE if needed.
        size_t u16len = text.size();
        size_t bytes = u16len * 2;
        size_t add = ((enc == ENCODING_UTF16_LE_WITH_BOM) || (enc == ENCODING_UTF16_BE_WITH_BOM)) ? 2 : 0;
        try {
            bin.resize(add + bytes);
        } catch (...) { return false; }
        size_t pos = 0;
        if (add) {
            if (enc == ENCODING_UTF16_LE_WITH_BOM) {
                bin[0] = 0xFF; bin[1] = 0xFE;
            } else {
                bin[0] = 0xFE; bin[1] = 0xFF;
            }
            pos = 2;
        }
        if ((enc == ENCODING_UTF16_LE_WITH_BOM) || (enc == ENCODING_UTF16_LE_WITHOUT_BOM)) {
            // copy raw bytes
            memcpy(&bin[pos], text.data(), bytes);
            return true;
        } else {
            // BE: need to swap bytes
            for (size_t i = 0; i < u16len; ++i) {
                uint16_t v = (uint16_t)text[i];
                bin[pos + i*2] = (uint8_t)((v >> 8) & 0xFF);
                bin[pos + i*2 + 1] = (uint8_t)(v & 0xFF);
            }
            return true;
        }
    } else if (enc == ENCODING_ANSI || enc == ENCODING_ASCII) {
        int len = WideCharToMultiByte(CP_ACP, 0, text.c_str(), (int)text.size(), NULL, 0, NULL, NULL);
        if (len < 0) return false;
        try {
            bin.resize(len);
        } catch (...) { return false; }
        if (len > 0)
            WideCharToMultiByte(CP_ACP, 0, text.c_str(), (int)text.size(), &bin[0], len, NULL, NULL);
        return true;
    }
    return false;
#else
    // POSIX: convert wstring -> target encoding. We'll first convert wstring -> u16 (if wchar_t != 2)
    std::vector<uint16_t> u16;
    if (sizeof(wchar_t) == 2) {
        u16.resize(text.size());
        for (size_t i = 0; i < text.size(); ++i) u16[i] = (uint16_t)text[i];
    } else {
        // wchar_t is 4 bytes (UTF-32); convert to UTF-16 code units
        for (size_t i = 0; i < text.size(); ++i) {
            uint32_t code = (uint32_t)text[i];
            if (code <= 0xFFFF) {
                u16.push_back((uint16_t)code);
            } else {
                code -= 0x10000;
                uint16_t high = (uint16_t)((code >> 10) + 0xD800);
                uint16_t low = (uint16_t)((code & 0x3FF) + 0xDC00);
                u16.push_back(high);
                u16.push_back(low);
            }
        }
    }

    if (enc == ENCODING_UTF8_WITH_BOM || enc == ENCODING_UTF8_WITHOUT_BOM) {
        std::string out;
        if (!u16_to_utf8(u16.empty() ? NULL : &u16[0], u16.size(), out)) return false;
        size_t add = (enc == ENCODING_UTF8_WITH_BOM) ? 3 : 0;
        try {
            bin.resize(add + out.size());
        } catch (...) { return false; }
        if (add) {
            bin[0] = 0xEF; bin[1] = 0xBB; bin[2] = 0xBF;
        }
        if (!out.empty())
            memcpy(&bin[add], out.data(), out.size());
        return true;
    } else if (enc == ENCODING_UTF16_LE_WITH_BOM || enc == ENCODING_UTF16_LE_WITHOUT_BOM ||
               enc == ENCODING_UTF16_BE_WITH_BOM || enc == ENCODING_UTF16_BE_WITHOUT_BOM) {
        size_t add = ((enc == ENCODING_UTF16_LE_WITH_BOM) || (enc == ENCODING_UTF16_BE_WITH_BOM)) ? 2 : 0;
        size_t bytes = u16.size() * 2;
        try {
            bin.resize(add + bytes);
        } catch (...) { return false; }
        size_t pos = 0;
        if (add) {
            if (enc == ENCODING_UTF16_LE_WITH_BOM) {
                bin[0] = 0xFF; bin[1] = 0xFE;
            } else {
                bin[0] = 0xFE; bin[1] = 0xFF;
            }
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
        // Narrow bytes: try to output as UTF-8 if characters fit; for ANSI we just convert via u16 -> utf8
        std::string out;
        if (!u16_to_utf8(u16.empty() ? NULL : &u16[0], u16.size(), out)) return false;
        try {
            bin = out;
        } catch (...) { return false; }
        return true;
    }
    return false;
#endif
}

// narrow-to-binary
static bool a_to_b(const std::string& text, binary_t& bin, ENCODING encoding) {
    bin.clear();
    ENCODING enc = encoding;

#ifdef _WIN32
    if (enc == ENCODING_BINARY) return false;

    if (enc == ENCODING_UTF8_WITH_BOM || enc == ENCODING_UTF8_WITHOUT_BOM) {
        size_t add = (enc == ENCODING_UTF8_WITH_BOM) ? 3 : 0;
        try {
            bin.resize(add + text.size());
        } catch (...) { return false; }
        if (add) { bin[0]=0xEF; bin[1]=0xBB; bin[2]=0xBF; }
        if (!text.empty()) memcpy(&bin[add], text.data(), text.size());
        return true;
    } else if (enc == ENCODING_UTF16_LE_WITH_BOM || enc == ENCODING_UTF16_LE_WITHOUT_BOM ||
               enc == ENCODING_UTF16_BE_WITH_BOM || enc == ENCODING_UTF16_BE_WITHOUT_BOM) {
        // convert UTF-8 narrow -> UTF-16 bytes
        int wideLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.c_str(), (int)text.size(), NULL, 0);
        if (wideLen < 0) return false;
        if (wideLen == 0) {
            // empty string
            size_t add = ((enc == ENCODING_UTF16_LE_WITH_BOM) || (enc == ENCODING_UTF16_BE_WITH_BOM)) ? 2 : 0;
            if (add) {
                bin.resize(add);
                if (enc == ENCODING_UTF16_LE_WITH_BOM) { bin[0]=0xFF; bin[1]=0xFE; }
                else { bin[0]=0xFE; bin[1]=0xFF; }
            } else bin.clear();
            return true;
        }
        std::wstring w;
        w.resize(wideLen);
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), (int)text.size(), &w[0], wideLen);
        size_t u16len = w.size();
        size_t add = ((enc == ENCODING_UTF16_LE_WITH_BOM) || (enc == ENCODING_UTF16_BE_WITH_BOM)) ? 2 : 0;
        try {
            bin.resize(add + u16len * 2);
        } catch (...) { return false; }
        size_t pos = 0;
        if (add) {
            if (enc == ENCODING_UTF16_LE_WITH_BOM) { bin[0]=0xFF; bin[1]=0xFE; } else { bin[0]=0xFE; bin[1]=0xFF; }
            pos = 2;
        }
        if ((enc == ENCODING_UTF16_LE_WITH_BOM) || (enc == ENCODING_UTF16_LE_WITHOUT_BOM)) {
            memcpy(&bin[pos], w.data(), u16len * 2);
            return true;
        } else {
            for (size_t i = 0; i < u16len; ++i) {
                uint16_t v = (uint16_t)w[i];
                bin[pos + i*2] = (uint8_t)((v >> 8) & 0xFF);
                bin[pos + i*2 + 1] = (uint8_t)(v & 0xFF);
            }
            return true;
        }
    } else if (enc == ENCODING_ANSI || enc == ENCODING_ASCII) {
        // Assume input text is UTF-8; convert to ANSI via wide conversion
        int wideLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.c_str(), (int)text.size(), NULL, 0);
        if (wideLen < 0) return false;
        std::wstring w;
        if (wideLen > 0) {
            w.resize(wideLen);
            MultiByteToWideChar(CP_UTF8, 0, text.c_str(), (int)text.size(), &w[0], wideLen);
        }
        int outLen = WideCharToMultiByte(CP_ACP, 0, w.c_str(), (int)w.size(), NULL, 0, NULL, NULL);
        if (outLen < 0) return false;
        try {
            bin.resize(outLen);
        } catch (...) { return false; }
        if (outLen > 0) WideCharToMultiByte(CP_ACP, 0, w.c_str(), (int)w.size(), &bin[0], outLen, NULL, NULL);
        return true;
    }
    return false;
#else
    // POSIX: assume input text is UTF-8 (std::string)
    if (enc == ENCODING_UTF8_WITH_BOM || enc == ENCODING_UTF8_WITHOUT_BOM) {
        size_t add = (enc == ENCODING_UTF8_WITH_BOM) ? 3 : 0;
        try {
            bin.resize(add + text.size());
        } catch (...) { return false; }
        if (add) { bin[0]=0xEF; bin[1]=0xBB; bin[2]=0xBF; }
        if (!text.empty()) memcpy(&bin[add], text.data(), text.size());
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
#endif
}

namespace khmz {

//////////////////////////////////////////////////////////////////////////
// TextFiler_impl

TextFiler_impl::TextFiler_impl(TextFiler *self) {
    m_self = self;
    m_encoding = c_default_encoding;
}

TextFiler_impl::TextFiler_impl(TextFiler *self, const tchar_t *text) {
    m_self = self;
    m_text = text;
    m_encoding = c_default_encoding;
}

TextFiler_impl::TextFiler_impl(TextFiler *self, const tstring_t& text) {
    m_self = self;
    m_text = text;
    m_encoding = c_default_encoding;
}

TextFiler_impl::~TextFiler_impl() { }

bool TextFiler_impl::load_raw(const tchar_t *filePath, binary_t& raw) {
    if (!_load_raw_inner(filePath, raw)) {
        raw.clear();
        return false;
    }
    return true;
}

bool TextFiler_impl::save_raw(const tchar_t *filePath, const binary_t& raw) {
    if (!_save_raw_inner(filePath, raw))
        return false;
    return true;
}

bool TextFiler_impl::_load_raw_inner(const tchar_t *filePath, binary_t& raw) {
#ifdef NO_OLDNAMES
    struct _stat st;
#else
    struct stat st;
#endif
    if (_tstat(filePath, &st) != 0)
        return false;

    if (st.st_size == 0) {
        raw.clear();
        return true;
    }

    try {
        raw.resize(st.st_size);
    } catch (...) {
        return false;
    }

    FILE *fin = _tfopen(filePath, _T("rb"));
    if (!fin)
        return false;

    bool ok = (fread(&raw[0], st.st_size, 1, fin) != 0);
    fclose(fin);
    return ok;
}

bool TextFiler_impl::_save_raw_inner(const tchar_t *filePath, const binary_t& raw) {
    FILE *fout = _tfopen(filePath, _T("wb"));
    if (!fout)
        return false;

    bool ok = (fwrite(&raw[0], raw.size(), 1, fout) != 0);
    fclose(fout);
    return ok;
}

ENCODING TextFiler_impl::detect_encoding(const byte_t *ptr, size_t size) {
    if (!size)
        return ENCODING_ASCII;

    if (size >= 2) {
        if (ptr[0] == 0xFF && ptr[1] == 0xFE)
            return ENCODING_UTF16_LE_WITH_BOM;
        if (ptr[0] == 0xFE && ptr[1] == 0xFF)
            return ENCODING_UTF16_BE_WITH_BOM;
        if (size >= 3 && ptr[0] == 0xEF && ptr[1] == 0xBB && ptr[2] == 0xBF)
            return ENCODING_UTF8_WITH_BOM;
    }

    bool is_ascii = true;
    for (size_t ib = 0; ib < size; ++ib) {
        if (ptr[ib] == 0)
            return ENCODING_BINARY;
        if (ptr[ib] & 0x80)
            is_ascii = false;
    }

    if (is_ascii)
        return ENCODING_ASCII;

    if (is_utf8_valid((char *)ptr, size))
        return ENCODING_UTF8_WITHOUT_BOM;

    return ENCODING_ANSI;
}

bool TextFiler_impl::_bin_to_text(const binary_t& bin, tstring_t& text, ENCODING encoding) {
#ifdef UNICODE
    return b_to_w(bin, text, encoding);
#else
    return b_to_a(bin, text, encoding);
#endif
}

bool TextFiler_impl::_text_to_bin(const tstring_t& text, binary_t& bin, ENCODING encoding) {
#ifdef UNICODE
    return w_to_b(text, bin, encoding);
#else
    return a_to_b(text, bin, encoding);
#endif
}

bool TextFiler_impl::is_utf8_valid(const char *ptr, size_t size) {
#ifdef _WIN32
    INT wideLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (char *)ptr, (INT)size, NULL, 0);
    return (wideLen > 0);
#else
    return utf8_validate(ptr, size);
#endif
}

} // namespace khmz
