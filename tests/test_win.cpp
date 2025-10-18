#include "../TextFiler.h"
#include "../detail/TextFiler_impl.h"
#include <windows.h>
#include <wchar.h>
#include <stdio.h>
#include <ctype.h>
#include <string>
#include <iostream>

using namespace khmz;

int main()
{
    const wchar_t* file1 = L"test_utf8_bom.txt";
    const std::wstring sample = L"Hello 世界";

    // 1) Save with UTF-8 BOM and reload
    {
        TextFiler tf;
        tf.text() = sample;
        tf.encoding() = ENCODING_UTF8_WITH_BOM;
        if (!tf.save(file1)) {
            std::wcerr << L"save UTF-8 with BOM failed\n";
            return 1;
        }
    }
    {
        TextFiler tf;
        if (!tf.load(file1)) {
            std::wcerr << L"load UTF-8 with BOM failed\n";
            return 2;
        }
        if (tf.encoding() != ENCODING_UTF8_WITH_BOM) {
            std::wcerr << L"encoding mismatch (expected UTF-8 BOM)\n";
            return 3;
        }
        if (tf.text() != sample) {
            std::wcerr << L"text mismatch after UTF-8 BOM round-trip\n";
            return 4;
        }
    }

    // 2) Save with UTF-16 LE BOM and reload
    const wchar_t* file2 = L"test_utf16_le.bin";
    {
        TextFiler tf;
        tf.text() = sample;
        tf.encoding() = ENCODING_UTF16_LE_WITH_BOM;
        if (!tf.save(file2)) {
            std::wcerr << L"save UTF-16 LE with BOM failed\n";
            return 5;
        }
    }
    {
        TextFiler tf;
        if (!tf.load(file2)) {
            std::wcerr << L"load UTF-16 LE with BOM failed\n";
            return 6;
        }
        if (tf.encoding() != ENCODING_UTF16_LE_WITH_BOM) {
            std::wcerr << L"encoding mismatch (expected UTF-16 LE BOM)\n";
            return 7;
        }
        if (tf.text() != sample) {
            std::wcerr << L"text mismatch after UTF-16 LE round-trip\n";
            return 8;
        }
    }

    // 3) Binary detection: write some bytes including a NUL, expect ENCODING_BINARY
    const wchar_t* file3 = L"test_binary.bin";
    {
        FILE* f = _wfopen(file3, L"wb");
        if (!f) {
            std::wcerr << L"_wfopen failed for binary test\n";
            return 9;
        }
        unsigned char buf[4] = {0x00, 0xFF, 0x00, 0x01};
        fwrite(buf, 1, sizeof(buf), f);
        fclose(f);
    }
    {
        binary_t raw;
        if (!TextFiler_impl::load_raw(file3, raw)) {
            std::wcerr << L"load_raw failed for binary test\n";
            return 10;
        }
        ENCODING enc = TextFiler_impl::detect_encoding(raw.c_str(), raw.size());
        if (enc != ENCODING_BINARY) {
            std::wcerr << L"expected binary encoding, got: " << enc << L"\n";
            return 11;
        }
    }

    // cleanup files
    DeleteFileW(file1);
    DeleteFileW(file2);
    DeleteFileW(file3);

    std::wcout << L"All Windows tests passed\n";
    return 0;
}
