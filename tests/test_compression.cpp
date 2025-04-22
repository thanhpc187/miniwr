#include <gtest/gtest.h>
#include "../src/core/DeflateCompressor.h"
#include <string>
#include <vector>

namespace miniwr {
namespace test {

class CompressionTest : public ::testing::Test {
protected:
    std::unique_ptr<Compressor> compressor;

    void SetUp() override {
        compressor = std::make_unique<DeflateCompressor>();
    }
};

TEST_F(CompressionTest, CompressAndDecompress) {
    // Test data
    std::string testData = "Hello, World! This is a test string for compression.";
    std::vector<uint8_t> input(testData.begin(), testData.end());

    // Compress
    auto compressed = compressor->compress(input, CompressionLevel::Default);
    ASSERT_LT(compressed.size(), input.size()) << "Compressed data should be smaller";

    // Decompress
    auto decompressed = compressor->decompress(compressed, input.size());
    ASSERT_EQ(decompressed.size(), input.size()) << "Decompressed size mismatch";

    // Compare content
    std::string decompressedStr(decompressed.begin(), decompressed.end());
    ASSERT_EQ(decompressedStr, testData) << "Decompressed data mismatch";
}

TEST_F(CompressionTest, CompressEmpty) {
    std::vector<uint8_t> empty;
    auto compressed = compressor->compress(empty);
    ASSERT_TRUE(compressed.empty()) << "Compressed empty data should be empty";

    auto decompressed = compressor->decompress(compressed);
    ASSERT_TRUE(decompressed.empty()) << "Decompressed empty data should be empty";
}

TEST_F(CompressionTest, CompressLargeData) {
    // Generate 1MB of repeating data
    std::vector<uint8_t> input;
    input.reserve(1024 * 1024);
    for (int i = 0; i < 1024 * 1024; ++i) {
        input.push_back(static_cast<uint8_t>(i % 256));
    }

    // Compress with different levels
    auto compressedMax = compressor->compress(input, CompressionLevel::Maximum);
    auto compressedFast = compressor->compress(input, CompressionLevel::Fast);
    auto compressedStore = compressor->compress(input, CompressionLevel::Store);

    // Verify compression ratios
    ASSERT_LT(compressedMax.size(), compressedFast.size())
        << "Maximum compression should be better than fast";
    ASSERT_EQ(compressedStore.size(), input.size())
        << "Store should not compress data";

    // Verify decompression
    auto decompressed = compressor->decompress(compressedMax, input.size());
    ASSERT_EQ(decompressed, input) << "Decompressed data mismatch";
}

} // namespace test
} // namespace miniwr 