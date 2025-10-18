#include <iostream>
#include <cstdio>
#include <string>

#include "TextFiler.h"
#include "TextFiler_impl.h"

using namespace khmz;

int main() {
    const char* file1 = "test_utf8_bom.txt";
    const std::string sample = "Hello 世界";

    // 1) Save with UTF-8 BOM and reload
    {
        TextFiler tf;
        tf.text() = sample;
        tf.encoding() = ENCODING_UTF8_WITH_BOM;
        if (!tf.save(file1)) {
            std::cerr << "save UTF-8 with BOM failed\n";
            return 1;
        }
    }
    {
        TextFiler tf;
        if (!tf.load(file1)) {
            std::cerr << "load UTF-8 with BOM failed\n";
            return 2;
        }
        if (tf.encoding() != ENCODING_UTF8_WITH_BOM) {
            std::cerr << "encoding mismatch (expected UTF-8 BOM)\n";
            return 3;
        }
        if (tf.text() != sample) {
            std::cerr << "text mismatch after UTF-8 BOM round-trip\n";
            return 4;
        }
    }

    // 2) Save with UTF-16 LE BOM and reload
    const char* file2 = "test_utf16_le.bin";
    {
        TextFiler tf;
        tf.text() = sample;
        tf.encoding() = ENCODING_UTF16_LE_WITH_BOM;
        if (!tf.save(file2)) {
            std::cerr << "save UTF-16 LE with BOM failed\n";
            return 5;
        }
    }
    {
        TextFiler tf;
        if (!tf.load(file2)) {
            std::cerr << "load UTF-16 LE with BOM failed\n";
            return 6;
        }
        if (tf.encoding() != ENCODING_UTF16_LE_WITH_BOM) {
            std::cerr << "encoding mismatch (expected UTF-16 LE BOM)\n";
            return 7;
        }
        if (tf.text() != sample) {
            std::cerr << "text mismatch after UTF-16 LE round-trip\n";
            return 8;
        }
    }

    // 3) Binary detection: write some bytes including a NUL, expect ENCODING_BINARY
    const char* file3 = "test_binary.bin";
    {
        FILE* f = fopen(file3, "wb");
        if (!f) {
            std::cerr << "fopen failed for binary test\n";
            return 9;
        }
        unsigned char buf[4] = {0x00, 0xFF, 0x00, 0x01};
        fwrite(buf, 1, sizeof(buf), f);
        fclose(f);
    }
    {
        binary_t raw;
        if (!TextFiler_impl::load_raw(file3, raw)) {
            std::cerr << "load_raw failed for binary test\n";
            return 10;
        }
        ENCODING enc = TextFiler_impl::detect_encoding(raw.c_str(), raw.size());
        if (enc != ENCODING_BINARY) {
            std::cerr << "expected binary encoding, got: " << enc << "\n";
            return 11;
        }
    }

    // cleanup
    std::remove(file1);
    std::remove(file2);
    std::remove(file3);

    std::cout << "All tests passed\n";
    return 0;
}