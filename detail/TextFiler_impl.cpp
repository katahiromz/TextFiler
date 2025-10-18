// TextFiler_impl.cpp
// Author: katahiromz
// License: MIT
#include "../TextFiler.h"
#include "TextFiler_impl.h"
#include "../tstring.h"
#include "pstdint.h"
#include <sys/types.h>
#include <sys/stat.h>
#include <assert.h>
#include <vector>
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

bool TextFiler_impl::_load_raw(const tchar_t *filePath, binary_t& raw) {
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

bool TextFiler_impl::_save_raw(const tchar_t *filePath, const binary_t& raw) {
    FILE *fout = _tfopen(filePath, _T("wb"));
    if (!fout)
        return false;

    bool ok = (fwrite(&raw[0], raw.size(), 1, fout) != 0);
    fclose(fout);
    return ok;
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

bool TextFiler_impl::_is_utf8_valid(const void *ptr, size_t size) {
#ifdef _WIN32
    INT wideLen = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, (char *)ptr, (INT)size, NULL, 0);
    return (wideLen > 0);
#else
    return utf8_validate((char *)ptr, size);
#endif
}

bool TextFiler_impl::_make_dir(const tchar_t *dir) {
#ifdef _WIN32
    return CreateDirectoryW(dir, NULL) || GetLastError() == ERROR_ALREADY_EXISTS;
#else
    if (mkdir(dir))
        return true;
    struct stat st;
    return (stat(path.c_str(), &st) == 0) && S_ISDIR(info.st_mode);
#endif
}

bool TextFiler_impl::_make_dir_path(const tchar_t *dir) {
    if (!dir || !dir[0])
        return false;
    if (_make_dir(dir))
        return true;
    tstring_t str = dir;
#ifdef _WIN32
    size_t pos = str.find_last_of(L"\\/");
#else
    size_t pos = str.find('/');
#endif
    if (pos == str.npos)
        return false;
    tstring_t parent = str.substr(0, pos);
    if (parent.empty())
        return false;
    if (!_make_dir_path(parent.c_str()))
        return false;
    return _make_dir(dir);
}

} // namespace khmz
