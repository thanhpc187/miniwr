#include "DeflateCompressor.h"
#include <stdexcept>

namespace miniwr {

DeflateCompressor::DeflateCompressor() : streamInitialized_(false) {
    initStream();
}

DeflateCompressor::~DeflateCompressor() {
    if (streamInitialized_) {
        endStream();
    }
}

void DeflateCompressor::initStream() {
    stream_ = {};
    streamInitialized_ = true;
}

void DeflateCompressor::endStream() {
    if (streamInitialized_) {
        deflateEnd(&stream_);
        streamInitialized_ = false;
    }
}

std::vector<uint8_t> DeflateCompressor::compress(
    std::span<const uint8_t> input,
    CompressionLevel level) {
    
    if (input.empty()) {
        return {};
    }

    // Initialize deflate
    stream_.zalloc = Z_NULL;
    stream_.zfree = Z_NULL;
    stream_.opaque = Z_NULL;

    int ret = deflateInit(&stream_, static_cast<int>(level));
    if (ret != Z_OK) {
        throw std::runtime_error("Failed to initialize deflate");
    }

    // Set input
    stream_.avail_in = static_cast<uInt>(input.size());
    stream_.next_in = const_cast<Bytef*>(input.data());

    // Prepare output buffer
    std::vector<uint8_t> output;
    output.reserve(input.size());  // Initial guess
    std::vector<uint8_t> buffer(CHUNK_SIZE);

    // Compress data
    do {
        stream_.avail_out = CHUNK_SIZE;
        stream_.next_out = buffer.data();

        ret = deflate(&stream_, Z_FINISH);
        if (ret == Z_STREAM_ERROR) {
            deflateEnd(&stream_);
            throw std::runtime_error("Compression error");
        }

        size_t have = CHUNK_SIZE - stream_.avail_out;
        output.insert(output.end(), buffer.begin(), buffer.begin() + have);
    } while (stream_.avail_out == 0);

    deflateEnd(&stream_);
    return output;
}

std::vector<uint8_t> DeflateCompressor::decompress(
    std::span<const uint8_t> input,
    size_t expectedSize) {
    
    if (input.empty()) {
        return {};
    }

    // Initialize inflate
    stream_.zalloc = Z_NULL;
    stream_.zfree = Z_NULL;
    stream_.opaque = Z_NULL;
    stream_.avail_in = 0;
    stream_.next_in = Z_NULL;

    int ret = inflateInit(&stream_);
    if (ret != Z_OK) {
        throw std::runtime_error("Failed to initialize inflate");
    }

    // Set input
    stream_.avail_in = static_cast<uInt>(input.size());
    stream_.next_in = const_cast<Bytef*>(input.data());

    // Prepare output buffer
    std::vector<uint8_t> output;
    output.reserve(expectedSize > 0 ? expectedSize : input.size() * 2);
    std::vector<uint8_t> buffer(CHUNK_SIZE);

    // Decompress data
    do {
        stream_.avail_out = CHUNK_SIZE;
        stream_.next_out = buffer.data();

        ret = inflate(&stream_, Z_NO_FLUSH);
        switch (ret) {
            case Z_NEED_DICT:
            case Z_DATA_ERROR:
            case Z_MEM_ERROR:
                inflateEnd(&stream_);
                throw std::runtime_error("Decompression error");
        }

        size_t have = CHUNK_SIZE - stream_.avail_out;
        output.insert(output.end(), buffer.begin(), buffer.begin() + have);
    } while (stream_.avail_out == 0);

    inflateEnd(&stream_);
    return output;
}
} 