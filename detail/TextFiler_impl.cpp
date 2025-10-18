// TextFiler_impl.cpp
#include "../TextFiler.h"
#include "TextFiler_impl.h"
#include "tstring.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <vector>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>

#ifdef _WIN32
    #include <windows.h>
    #include "win_impl.h"
#else
    #include <errno.h>
    #include "posix_impl.h"
#endif

namespace khmz {

//////////////////////////////////////////////////////////////////////////
// TextFiler_impl

TextFiler_impl::TextFiler_impl(TextFiler *self) {
    m_self = self;
    m_encoding = ENCODING_DEFAULT;
}

TextFiler_impl::TextFiler_impl(TextFiler *self, const tchar_t *text) {
    m_self = self;
    m_text = text;
    m_encoding = ENCODING_DEFAULT;
}

TextFiler_impl::TextFiler_impl(TextFiler *self, const tstring_t& text) {
    m_self = self;
    m_text = text;
    m_encoding = ENCODING_DEFAULT;
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

ENCODING TextFiler_impl::detect_encoding(const void *ptr, size_t size) {
    if (!size)
        return ENCODING_ASCII;

    const byte_t *pb = reinterpret_cast<const byte_t *>(ptr);
    if (size >= 2) {
        if (pb[0] == 0xFF && pb[1] == 0xFE)
            return ENCODING_UTF16_LE_WITH_BOM;
        if (pb[0] == 0xFE && pb[1] == 0xFF)
            return ENCODING_UTF16_BE_WITH_BOM;
        if (size >= 3 && pb[0] == 0xEF && pb[1] == 0xBB && pb[2] == 0xBF)
            return ENCODING_UTF8_WITH_BOM;
    }

    bool is_ascii = true;
    for (size_t ib = 0; ib < size; ++ib) {
        if (pb[ib] == 0)
            return ENCODING_BINARY;
        if (pb[ib] & 0x80)
            is_ascii = false;
    }

    if (is_ascii)
        return ENCODING_ASCII;

    if (is_utf8_valid(pb, size))
        return ENCODING_UTF8_WITHOUT_BOM;

    return ENCODING_ANSI;
}

bool TextFiler_impl::_bin_to_text(const binary_t& bin, tstring_t& text, ENCODING enc) {
#ifdef _WIN32
    return bin_to_text_on_win(bin, text, enc);
#else
    return bin_to_text_on_posix(bin, text, enc);
#endif
}

bool TextFiler_impl::_text_to_bin(const tstring_t& text, binary_t& bin, ENCODING enc) {
#ifdef _WIN32
    return text_to_bin_on_win(text, bin, enc);
#else
    return text_to_bin_on_posix(text, bin, enc);
#endif
}

bool TextFiler_impl::is_utf8_valid(const void *ptr, size_t size) {
#ifdef _WIN32
    INT wideLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (char *)ptr, (INT)size, NULL, 0);
    return (wideLen > 0);
#else
    return utf8_validate((char *)ptr, size);
#endif
}

} // namespace khmz
