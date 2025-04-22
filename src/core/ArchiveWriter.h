#pragma once

#include "Compressor.h"
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace miniwr {

/**
 * @brief ZIP file entry metadata
 */
struct ZipEntry {
    std::string filename;
    uint32_t crc32;
    uint32_t compressedSize;
    uint32_t uncompressedSize;
    uint16_t modificationTime;  // DOS format
    uint16_t modificationDate;  // DOS format
    uint16_t externalAttrs;     // POSIX permissions in high byte
    std::streampos headerOffset;  // Local file header position
};

/**
 * @brief ZIP archive writer
 */
class ArchiveWriter {
public:
    explicit ArchiveWriter(const std::filesystem::path& archivePath);
    ~ArchiveWriter();

    /**
     * @brief Add a file to the archive
     * @param filepath Path to the file to add
     * @param level Compression level
     */
    void addFile(const std::filesystem::path& filepath,
                CompressionLevel level = CompressionLevel::Default);

    /**
     * @brief Add a directory to the archive recursively
     * @param dirpath Path to the directory
     * @param level Compression level
     */
    void addDirectory(const std::filesystem::path& dirpath,
                     CompressionLevel level = CompressionLevel::Default);

    /**
     * @brief Finalize and close the archive
     */
    void close();

private:
    std::filesystem::path archivePath_;
    std::ofstream archive_;
    std::unique_ptr<Compressor> compressor_;
    std::vector<ZipEntry> entries_;

    void writeLocalFileHeader(const ZipEntry& entry);
    void writeCentralDirectory();
    void writeEndOfCentralDirectory();

    static uint32_t calculateCrc32(const std::vector<uint8_t>& data);
    static std::pair<uint16_t, uint16_t> getModificationTimeAndDate(
        const std::filesystem::file_time_type& ftime);
}; 