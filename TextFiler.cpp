// TextFiler.cpp
// Author: katahiromz
// License: MIT
#include "TextFiler.h"
#include "detail/TextFiler_impl.h"

namespace khmz {

//////////////////////////////////////////////////////////////////////////
// TextFiler

TextFiler::TextFiler() {
    m_pimpl = new TextFiler_impl(this);
}

TextFiler::TextFiler(const tstring_t& text) {
    m_pimpl = new TextFiler_impl(this, text);
}

TextFiler::~TextFiler() {
    delete m_pimpl;
}

      tstring_t& TextFiler::text()       { return m_pimpl->m_text; }
const tstring_t& TextFiler::text() const { return m_pimpl->m_text; }

void TextFiler::clear() {
    m_pimpl->m_text.clear();
}

bool TextFiler::load(const tchar_t *filePath) {
    binary_t bin;
    if (!load_raw(filePath, bin)) {
        text().clear();
        return false;
    }
    ENCODING enc = detect_encoding(filePath, bin.c_str(), bin.size());
    if (!m_pimpl->_bin_to_text(bin, text(), enc))
        return false;
    encoding() = enc;
    return true;
}

bool TextFiler::save(const tchar_t *filePath) {
    binary_t bin;
    if (!m_pimpl->_text_to_bin(text(), bin, encoding()))
        return false;
    if (!save_raw(filePath, bin))
        return false;
    return true;
}

ENCODING& TextFiler::encoding() {
    return m_pimpl->m_encoding;
}
const ENCODING& TextFiler::encoding() const {
    return m_pimpl->m_encoding;
}

bool TextFiler::is_utf8() const {
    switch (encoding()) {
    case ENCODING_UTF8_WITHOUT_BOM:
    case ENCODING_UTF8_WITH_BOM:
    case ENCODING_ASCII:
        return true;
    default:
        return false;
    }
}
bool TextFiler::is_utf16_le() const {
    switch (encoding()) {
    case ENCODING_UTF16_LE_WITHOUT_BOM:
    case ENCODING_UTF16_LE_WITH_BOM:
        return true;
    default:
        return false;
    }
}

bool TextFiler::is_utf16_be() const {
    switch (encoding()) {
    case ENCODING_UTF16_BE_WITHOUT_BOM:
    case ENCODING_UTF16_BE_WITH_BOM:
        return true;
    default:
        return false;
    }
}

bool TextFiler::is_binary() const {
    return !(is_utf8() || is_utf16_le() || is_utf16_be());
}
bool TextFiler::is_ascii() const {
    return encoding() == ENCODING_ASCII;
}

void TextFiler::remove_bom() {
    switch (encoding()) {
    case ENCODING_BINARY:
    case ENCODING_UTF8_WITHOUT_BOM:
    case ENCODING_UTF16_LE_WITHOUT_BOM:
    case ENCODING_UTF16_BE_WITHOUT_BOM:
    case ENCODING_ANSI:
    case ENCODING_ASCII:
        // No BOM
        break;
    case ENCODING_UTF8_WITH_BOM:
        encoding() = ENCODING_UTF8_WITHOUT_BOM;
        break;
    case ENCODING_UTF16_LE_WITH_BOM:
        encoding() = ENCODING_UTF16_LE_WITHOUT_BOM;
        break;
    case ENCODING_UTF16_BE_WITH_BOM:
        encoding() = ENCODING_UTF16_BE_WITHOUT_BOM;
        break;
    }
}

//////////////////////////////////////////////////////////////////////////
// Useful functions

bool load_raw(const tchar_t *filePath, binary_t& raw) {
    if (!TextFiler_impl::_load_raw(filePath, raw)) {
        raw.clear();
        return false;
    }
    return true;
}

bool save_raw(const tchar_t *filePath, const binary_t& raw) {
    if (!TextFiler_impl::_save_raw(filePath, raw))
        return false;
    return true;
}

bool is_known_binary_file(const tchar_t *filePath) {
    const tchar_t *patterns[] = {
        _T(".jpg"), _T(".jpeg"), _T(".png"), _T(".gif"), _T(".bmp"), _T(".tiff"),
        _T(".exe"), _T(".dll"), _T(".ocx"), _T(".zip"), _T(".rar"), _T(".7z"),
        _T(".pdf"), _T(".bin")
    };
    const tstring_t fileName = filePath;
    for (size_t iPat = 0; iPat < _countof(patterns); ++iPat) {
        const tstring_t ext = patterns[iPat];
        if (fileName.size() >= ext.size() &&
            _tcsicmp(fileName.c_str() + fileName.size() - ext.size(), ext.c_str()) == 0) {
            return true;
        }
    }
    return false;
}

bool is_utf8_valid(const void *ptr, size_t size) {
    return TextFiler_impl::_is_utf8_valid(ptr, size);
}

ENCODING detect_encoding(const tchar_t *filePath, const void *ptr, size_t size) {
    if (filePath) {
        if (is_known_binary_file(filePath))
            return ENCODING_BINARY;
    }

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

} // namespace khmz
