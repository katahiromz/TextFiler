// TextFiler_impl.cpp
#include "TextFiler.h"
#include "TextFiler_impl.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>

static const ENCODING c_default_encoding = ENCODING_UTF8;

static bool b_to_w(const binary_t& bin, std::wstring& text, ENCODING encoding) {
    // TODO:
    return false;
}
static bool b_to_a(const binary_t& bin, std::string& text, ENCODING encoding) {
    // TODO:
    return false;
}
static bool w_to_b(const std::wstring& text, binary_t& bin, ENCODING encoding) {
    // TODO:
    return false;
}
static bool a_to_b(const std::string& text, binary_t& bin, ENCODING encoding) {
    // TODO:
    return false;
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

bool TextFiler_impl::bin_to_text(const binary_t& bin, tstring_t& text, ENCODING encoding) {
#ifdef UNICODE
    return b_to_w(bin, text, encoding);
#else
    return b_to_a(bin, text, encoding);
#endif
}

bool TextFiler_impl::text_to_bin(const tstring_t& text, binary_t& bin, ENCODING encoding) {
#ifdef UNICODE
    return w_to_b(text, bin, encoding);
#else
    return w_to_b(text, bin, encoding);
#endif
}

bool TextFiler_impl::is_utf8_valid(const char *ptr, size_t size) {
#ifdef _WIN32
    INT wideLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (char *)ptr, (INT)size, NULL, 0);
    return (wideLen > 0);
#else
    // TODO:
    return true;
#endif
}

} // namespace khmz
