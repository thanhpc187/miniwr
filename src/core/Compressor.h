#pragma once

#include <cstdint>
#include <span>
#include <string>
#include <vector>
#include <memory>

namespace miniwr {

/**
 * @brief Compression level enumeration
 */
enum class CompressionLevel {
    Store = 0,  ///< No compression
    Fast = 1,   ///< Fast compression
    Default = 6,///< Default compression
    Maximum = 9 ///< Maximum compression
};

/**
 * @brief Abstract interface for compression algorithms
 */
class Compressor {
public:
    virtual ~Compressor() = default;

    /**
     * @brief Compress a block of data
     * @param input Input data span
     * @param level Compression level
     * @return Compressed data vector
     */
    virtual std::vector<uint8_t> compress(
        std::span<const uint8_t> input,
        CompressionLevel level = CompressionLevel::Default) = 0;

    /**
     * @brief Decompress a block of data
     * @param input Compressed data span
     * @param expectedSize Expected size of decompressed data (if known)
     * @return Decompressed data vector
     */
    virtual std::vector<uint8_t> decompress(
        std::span<const uint8_t> input,
        size_t expectedSize = 0) = 0;

    /**
     * @brief Create a new compressor instance
     * @param type Compression type string ("deflate", "gzip")
     * @return Unique pointer to compressor instance
     */
    static std::unique_ptr<Compressor> create(const std::string& type);
}; 