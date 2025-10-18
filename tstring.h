// tstring.h
#pragma once

#include <string>
#include <string.h>

#ifdef _WIN32
    #include <tchar.h>
#else
    // TODO:
#endif

namespace khmz {
#ifdef _WIN32
    typedef wchar_t tchar_t;
    typedef std::wstring tstring_t;
#else
    typedef char tchar_t;
    typedef std::string tstring_t;
#endif

    typedef unsigned char byte_t;
    typedef std::string binary_t;
} // namespace khmz
