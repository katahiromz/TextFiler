// TextFiler_impl.h
#pragma once

#include "tstring.h"

namespace khmz {
    struct TextFiler_impl {
        TextFiler *m_self;
        tstring_t m_text;
        ENCODING m_encoding;

    public:
        TextFiler_impl(TextFiler *self);
        TextFiler_impl(TextFiler *self, const tchar_t *text);
        TextFiler_impl(TextFiler *self, const tstring_t& text);
        virtual ~TextFiler_impl();

        static bool load_raw(const tchar_t *filePath, binary_t& raw);
        static bool save_raw(const tchar_t *filePath, const binary_t& raw);
        static ENCODING detect_encoding(const void *ptr, size_t size);
        static bool is_utf8_valid(const void *ptr, size_t size);
        static bool _load_raw_inner(const tchar_t *filePath, binary_t& raw);
        static bool _save_raw_inner(const tchar_t *filePath, const binary_t& raw);
        static bool _bin_to_text(const binary_t& bin, tstring_t& text, ENCODING encoding);
        static bool _text_to_bin(const tstring_t& text, binary_t& bin, ENCODING encoding);
    };
} // namespace khmz
