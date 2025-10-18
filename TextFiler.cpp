// TextFiler.cpp
#include "TextFiler.h"
#include "TextFiler_impl.h"

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

void TextFiler::reserve(size_t size) {
    m_pimpl->m_text.reserve(size);
}
void TextFiler::clear() {
    m_pimpl->m_text.clear();
}

bool TextFiler::load(const tchar_t *filePath) {
    binary_t bin;
    if (!m_pimpl->load_raw(filePath, bin)) {
        text().clear();
        return false;
    }
    ENCODING enc = m_pimpl->detect_encoding(bin.c_str(), bin.size());
    if (!m_pimpl->_bin_to_text(bin, text(), enc))
        return false;
    encoding() = enc;
    return true;
}

bool TextFiler::save(const tchar_t *filePath) {
    binary_t bin;
    if (!m_pimpl->_text_to_bin(text(), bin, encoding()))
        return false;
    if (!m_pimpl->save_raw(filePath, bin))
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

} // namespace khmz
