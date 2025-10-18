#include <windows.h>
#include <stddef.h>
#include <stdint.h>
#include "win_impl.h"

namespace khmz {

// binary-to-wide
bool bin_to_text_on_win(const binary_t& bin, std::wstring& text, ENCODING enc) {
    text.clear();
    size_t size = bin.size();
    const byte_t *ptr = (const byte_t*)(bin.data());

    if (enc == ENCODING_BINARY)
        enc = ENCODING_DEFAULT;

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
}

// wide-to-binary
bool text_to_bin_on_win(const std::wstring& text, binary_t& bin, ENCODING enc) {
    bin.clear();

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
}

} // namespace khmz
