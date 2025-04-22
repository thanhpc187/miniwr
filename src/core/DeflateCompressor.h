#pragma once

#include "Compressor.h"
#include <zlib.h>

namespace miniwr {

/**
 * @brief DEFLATE compression implementation using zlib
 */
class DeflateCompressor : public Compressor {
public:
    DeflateCompressor();
    ~DeflateCompressor() override;

    std::vector<uint8_t> compress(
        std::span<const uint8_t> input,
        CompressionLevel level = CompressionLevel::Default) override;

    std::vector<uint8_t> decompress(
        std::span<const uint8_t> input,
        size_t expectedSize = 0) override;

private:
    static constexpr size_t CHUNK_SIZE = 16384;  // 16KB chunks
    z_stream stream_;
    bool streamInitialized_;

    void initStream();
    void endStream();
}; 