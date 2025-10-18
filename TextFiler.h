// TextFiler.h
#pragma once

#include <stdio.h>
#include "detail/tstring.h"

namespace khmz {
    struct TextFiler_impl;

    enum ENCODING {
        ENCODING_BINARY,
        ENCODING_UTF8_WITHOUT_BOM,
        ENCODING_UTF8_WITH_BOM,
        ENCODING_UTF16_LE_WITHOUT_BOM,
        ENCODING_UTF16_LE_WITH_BOM,
        ENCODING_UTF16_BE_WITHOUT_BOM,
        ENCODING_UTF16_BE_WITH_BOM,
        ENCODING_ANSI,
        ENCODING_ASCII,
        // aliases
        ENCODING_UTF8 = ENCODING_UTF8_WITHOUT_BOM,
        ENCODING_UTF16_LE = ENCODING_UTF16_LE_WITH_BOM,
        ENCODING_UTF16_BE = ENCODING_UTF16_BE_WITH_BOM,
        ENCODING_DEFAULT = ENCODING_UTF8,
    };

    // generic helper functions
    bool load_raw(const tchar_t *filePath, binary_t& raw);
    bool save_raw(const tchar_t *filePath, const binary_t& raw);
    bool is_utf8_valid(const void *ptr, size_t size);
    ENCODING detect_encoding(const tchar_t *filePath, const void *ptr, size_t size);

    class TextFiler {
    public:
        TextFiler();
        TextFiler(const tstring_t& text);
        virtual ~TextFiler();

        bool load(const tchar_t *filePath);
        bool save(const tchar_t *filePath);

              tstring_t& text();
        const tstring_t& text() const;

              ENCODING& encoding();
        const ENCODING& encoding() const;
        bool is_utf8() const;
        bool is_utf16_le() const;
        bool is_utf16_be() const;
        bool is_binary() const;
        bool is_ascii() const;

        void clear();
        void remove_bom();

    protected:
        struct TextFiler_impl *m_pimpl;
    };
} // namespace khmz
