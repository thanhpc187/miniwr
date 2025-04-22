#include "ArchiveWriter.h"
#include <chrono>
#include <ctime>
#include <stdexcept>
#include <zlib.h>

namespace miniwr {

namespace {
    constexpr uint32_t ZIP_LOCAL_HEADER_SIGNATURE = 0x04034b50;
    constexpr uint32_t ZIP_CENTRAL_DIR_SIGNATURE = 0x02014b50;
    constexpr uint32_t ZIP_END_OF_CENTRAL_DIR_SIGNATURE = 0x06054b50;
    constexpr uint16_t ZIP_VERSION_MADE_BY = 0x033F;  // UNIX + Version 6.3
    constexpr uint16_t ZIP_VERSION_NEEDED = 0x0014;   // Version 2.0
    constexpr uint16_t ZIP_GENERAL_PURPOSE_FLAGS = 0x0000;
    constexpr uint16_t ZIP_COMPRESSION_METHOD_DEFLATE = 0x0008;
    constexpr uint16_t ZIP_COMPRESSION_METHOD_STORE = 0x0000;
}

ArchiveWriter::ArchiveWriter(const std::filesystem::path& archivePath)
    : archivePath_(archivePath),
      archive_(archivePath, std::ios::binary),
      compressor_(Compressor::create("deflate")) {
    
    if (!archive_) {
        throw std::runtime_error("Failed to create archive file: " + archivePath.string());
    }
}

ArchiveWriter::~ArchiveWriter() {
    try {
        if (archive_.is_open()) {
            close();
        }
    } catch (...) {
        // Destructor shouldn't throw
    }
}

void ArchiveWriter::addFile(const std::filesystem::path& filepath,
                          CompressionLevel level) {
    if (!std::filesystem::exists(filepath)) {
        throw std::runtime_error("File not found: " + filepath.string());
    }

    // Read file content
    std::ifstream file(filepath, std::ios::binary);
    std::vector<uint8_t> content((std::istreambuf_iterator<char>(file)),
                                std::istreambuf_iterator<char>());
    file.close();

    // Prepare entry
    ZipEntry entry;
    entry.filename = filepath.generic_string();
    entry.uncompressedSize = static_cast<uint32_t>(content.size());
    entry.crc32 = calculateCrc32(content);

    auto [modTime, modDate] = getModificationTimeAndDate(
        std::filesystem::last_write_time(filepath));
    entry.modificationTime = modTime;
    entry.modificationDate = modDate;

    // Set POSIX permissions
    auto perms = std::filesystem::status(filepath).permissions();
    entry.externalAttrs = static_cast<uint16_t>(
        static_cast<uint32_t>(perms) & 0xFFFF) << 16;

    // Store header position
    entry.headerOffset = archive_.tellp();

    // Compress content if needed
    std::vector<uint8_t> compressedData;
    uint16_t compressionMethod;
    
    if (level == CompressionLevel::Store) {
        compressedData = std::move(content);
        compressionMethod = ZIP_COMPRESSION_METHOD_STORE;
    } else {
        compressedData = compressor_->compress(content, level);
        compressionMethod = ZIP_COMPRESSION_METHOD_DEFLATE;
    }
    
    entry.compressedSize = static_cast<uint32_t>(compressedData.size());

    // Write local file header
    writeLocalFileHeader(entry);

    // Write file data
    archive_.write(reinterpret_cast<const char*>(compressedData.data()),
                  compressedData.size());

    entries_.push_back(entry);
}

void ArchiveWriter::addDirectory(const std::filesystem::path& dirpath,
                               CompressionLevel level) {
    if (!std::filesystem::exists(dirpath)) {
        throw std::runtime_error("Directory not found: " + dirpath.string());
    }

    for (const auto& entry : std::filesystem::recursive_directory_iterator(dirpath)) {
        if (std::filesystem::is_regular_file(entry)) {
            addFile(entry.path(), level);
        }
    }
}

void ArchiveWriter::close() {
    if (!archive_.is_open()) {
        return;
    }

    writeCentralDirectory();
    writeEndOfCentralDirectory();
    archive_.close();
}

void ArchiveWriter::writeLocalFileHeader(const ZipEntry& entry) {
    // Local file header signature
    archive_.write(reinterpret_cast<const char*>(&ZIP_LOCAL_HEADER_SIGNATURE), 4);

    // Version needed to extract
    archive_.write(reinterpret_cast<const char*>(&ZIP_VERSION_NEEDED), 2);

    // General purpose bit flag
    archive_.write(reinterpret_cast<const char*>(&ZIP_GENERAL_PURPOSE_FLAGS), 2);

    // Compression method (DEFLATE)
    archive_.write(reinterpret_cast<const char*>(&ZIP_COMPRESSION_METHOD_DEFLATE), 2);

    // Last mod time and date
    archive_.write(reinterpret_cast<const char*>(&entry.modificationTime), 2);
    archive_.write(reinterpret_cast<const char*>(&entry.modificationDate), 2);

    // CRC-32
    archive_.write(reinterpret_cast<const char*>(&entry.crc32), 4);

    // Compressed size
    archive_.write(reinterpret_cast<const char*>(&entry.compressedSize), 4);

    // Uncompressed size
    archive_.write(reinterpret_cast<const char*>(&entry.uncompressedSize), 4);

    // Filename length
    uint16_t filenameLength = static_cast<uint16_t>(entry.filename.length());
    archive_.write(reinterpret_cast<const char*>(&filenameLength), 2);

    // Extra field length (none)
    uint16_t extraFieldLength = 0;
    archive_.write(reinterpret_cast<const char*>(&extraFieldLength), 2);

    // Filename
    archive_.write(entry.filename.c_str(), filenameLength);
}

void ArchiveWriter::writeCentralDirectory() {
    auto centralDirOffset = archive_.tellp();

    for (const auto& entry : entries_) {
        // Central directory header signature
        archive_.write(reinterpret_cast<const char*>(&ZIP_CENTRAL_DIR_SIGNATURE), 4);

        // Version made by
        archive_.write(reinterpret_cast<const char*>(&ZIP_VERSION_MADE_BY), 2);

        // Version needed to extract
        archive_.write(reinterpret_cast<const char*>(&ZIP_VERSION_NEEDED), 2);

        // General purpose bit flag
        archive_.write(reinterpret_cast<const char*>(&ZIP_GENERAL_PURPOSE_FLAGS), 2);

        // Compression method
        archive_.write(reinterpret_cast<const char*>(&ZIP_COMPRESSION_METHOD_DEFLATE), 2);

        // Last mod time and date
        archive_.write(reinterpret_cast<const char*>(&entry.modificationTime), 2);
        archive_.write(reinterpret_cast<const char*>(&entry.modificationDate), 2);

        // CRC-32
        archive_.write(reinterpret_cast<const char*>(&entry.crc32), 4);

        // Compressed size
        archive_.write(reinterpret_cast<const char*>(&entry.compressedSize), 4);

        // Uncompressed size
        archive_.write(reinterpret_cast<const char*>(&entry.uncompressedSize), 4);

        // Filename length
        uint16_t filenameLength = static_cast<uint16_t>(entry.filename.length());
        archive_.write(reinterpret_cast<const char*>(&filenameLength), 2);

        // Extra field length (none)
        uint16_t extraFieldLength = 0;
        archive_.write(reinterpret_cast<const char*>(&extraFieldLength), 2);

        // File comment length (none)
        uint16_t fileCommentLength = 0;
        archive_.write(reinterpret_cast<const char*>(&fileCommentLength), 2);

        // Disk number start
        uint16_t diskNumberStart = 0;
        archive_.write(reinterpret_cast<const char*>(&diskNumberStart), 2);

        // Internal file attributes
        uint16_t internalFileAttrs = 0;
        archive_.write(reinterpret_cast<const char*>(&internalFileAttrs), 2);

        // External file attributes (POSIX permissions)
        archive_.write(reinterpret_cast<const char*>(&entry.externalAttrs), 4);

        // Relative offset of local header
        uint32_t localHeaderOffset = static_cast<uint32_t>(entry.headerOffset);
        archive_.write(reinterpret_cast<const char*>(&localHeaderOffset), 4);

        // Filename
        archive_.write(entry.filename.c_str(), filenameLength);
    }

    auto centralDirSize = archive_.tellp() - centralDirOffset;

    // End of central directory record
    archive_.write(reinterpret_cast<const char*>(&ZIP_END_OF_CENTRAL_DIR_SIGNATURE), 4);

    // Number of this disk
    uint16_t diskNumber = 0;
    archive_.write(reinterpret_cast<const char*>(&diskNumber), 2);

    // Disk where central directory starts
    archive_.write(reinterpret_cast<const char*>(&diskNumber), 2);

    // Number of central directory records on this disk
    uint16_t numEntries = static_cast<uint16_t>(entries_.size());
    archive_.write(reinterpret_cast<const char*>(&numEntries), 2);

    // Total number of central directory records
    archive_.write(reinterpret_cast<const char*>(&numEntries), 2);

    // Size of central directory
    archive_.write(reinterpret_cast<const char*>(&centralDirSize), 4);

    // Offset of start of central directory
    archive_.write(reinterpret_cast<const char*>(&centralDirOffset), 4);

    // ZIP file comment length (none)
    uint16_t zipCommentLength = 0;
    archive_.write(reinterpret_cast<const char*>(&zipCommentLength), 2);
}

void ArchiveWriter::writeEndOfCentralDirectory() {
    // Already written as part of writeCentralDirectory()
}

uint32_t ArchiveWriter::calculateCrc32(const std::vector<uint8_t>& data) {
    return crc32(0L, data.data(), data.size());
}

std::pair<uint16_t, uint16_t> ArchiveWriter::getModificationTimeAndDate(
    const std::filesystem::file_time_type& ftime) {
    
    using namespace std::chrono;
    auto sctp = time_point_cast<system_clock::duration>(
        ftime - std::filesystem::file_time_type::clock::now() + system_clock::now());
    
    std::time_t tt = system_clock::to_time_t(sctp);
    std::tm* tm = std::localtime(&tt);

    uint16_t time = static_cast<uint16_t>(
        (tm->tm_hour << 11) |    // 5 bits
        (tm->tm_min << 5) |      // 6 bits
        (tm->tm_sec >> 1));      // 5 bits

    uint16_t date = static_cast<uint16_t>(
        ((tm->tm_year - 80) << 9) |  // 7 bits
        ((tm->tm_mon + 1) << 5) |    // 4 bits
        tm->tm_mday);                // 5 bits

    return {time, date};
}
} 