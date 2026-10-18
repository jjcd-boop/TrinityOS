#include "trinity_png_rows.h"
#include <cstdint>
#include <cstdio>

int main() {
    // Filter None + one RGB pixel; alpha should become 255.
    const std::uint8_t source[4] = {0, 9, 17, 33};
    std::uint8_t output[4] = {};
    std::uint8_t scratch[6] = {};
    const auto rc = trinity_png_rows_to_rgba(
        source, sizeof(source), 1, 1, 3,
        output, sizeof(output), 1, 1, scratch, sizeof(scratch));
    if (rc != 0 || output[0] != 9 || output[1] != 17
        || output[2] != 33 || output[3] != 255) {
        std::fprintf(stderr, "FFI failure: result=%d rgba=%u,%u,%u,%u\n",
            rc, output[0], output[1], output[2], output[3]);
        return 1;
    }
    std::puts("PASS C++/Rust PNG row ABI smoke");
    return 0;
}
