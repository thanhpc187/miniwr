#include "ArchiveReader.h"
#include <iostream>
#include <stdexcept>
#include <string>
#include <zlib.h>

namespace miniwr {

namespace {
    constexpr uint32_t ZIP_LOCAL_HEADER_SIGNATURE = 0x04034b50;
    constexpr uint32_t ZIP_CENTRAL_DIR_SIGNATURE = 0x02014b50;
    constexpr uint32_t ZIP_END_OF_CENTRAL_DIR_SIGNATURE = 0x06054b50;
    constexpr size_t END_OF_CENTRAL_DIR_SIZE = 22;
    constexpr size_t MAX_COMMENT_SIZE = 65535;
}

ArchiveReader::ArchiveReader(const std::filesystem::path& archivePath)
    : archivePath_(archivePath),
      archive_(archivePath, std::ios::binary),
      compressor_(Compressor::create("deflate")) {
    
    if (!archive_) {
        throw std::runtime_error("Failed to open archive file: " + archivePath.string());
    }

    readCentralDirectory();
}

ArchiveReader::~ArchiveReader() {
    if (archive_.is_open()) {
        archive_.close();
    }
}

void ArchiveReader::readCentralDirectory() {
    // Find end of central directory record
    archive_.seekg(0, std::ios::end);
    auto fileSize = archive_.tellg();

    // Search for end of central directory signature
    const size_t bufSize = std::min(static_cast<size_t>(fileSize),
                                  END_OF_CENTRAL_DIR_SIZE + MAX_COMMENT_SIZE);
    std::vector<char> buffer(bufSize);

    archive_.seekg(-static_cast<std::streamoff>(bufSize), std::ios::end);
    archive_.read(buffer.data(), bufSize);

    auto pos = bufSize - END_OF_CENTRAL_DIR_SIZE;
    bool found = false;

    while (pos > 0) {
        if (*reinterpret_cast<uint32_t*>(&buffer[pos]) == ZIP_END_OF_CENTRAL_DIR_SIGNATURE) {
            found = true;
            break;
        }
        --pos;
    }

    if (!found) {
        throw std::runtime_error("Invalid ZIP file: End of central directory not found");
    }

    // Read central directory offset
    uint32_t centralDirOffset = *reinterpret_cast<uint32_t*>(&buffer[pos + 16]);
    uint16_t numEntries = *reinterpret_cast<uint16_t*>(&buffer[pos + 10]);

    // Read central directory entries
    archive_.seekg(centralDirOffset);
    
    for (uint16_t i = 0; i < numEntries; ++i) {
        uint32_t signature;
        archive_.read(reinterpret_cast<char*>(&signature), 4);
        
        if (signature != ZIP_CENTRAL_DIR_SIGNATURE) {
            throw std::runtime_error("Invalid central directory entry");
        }

        // Skip version made by and needed
        archive_.seekg(4, std::ios::cur);

        // Read general purpose flags
        uint16_t flags;
        archive_.read(reinterpret_cast<char*>(&flags), 2);

        // Read compression method
        uint16_t compressionMethod;
        archive_.read(reinterpret_cast<char*>(&compressionMethod), 2);

        ZipEntry entry;
        
        // Read modification time and date
        archive_.read(reinterpret_cast<char*>(&entry.modificationTime), 2);
        archive_.read(reinterpret_cast<char*>(&entry.modificationDate), 2);

        // Read CRC and sizes
        archive_.read(reinterpret_cast<char*>(&entry.crc32), 4);
        archive_.read(reinterpret_cast<char*>(&entry.compressedSize), 4);
        archive_.read(reinterpret_cast<char*>(&entry.uncompressedSize), 4);

        // Read filename length and extra field length
        uint16_t filenameLength;
        uint16_t extraFieldLength;
        uint16_t fileCommentLength;
        archive_.read(reinterpret_cast<char*>(&filenameLength), 2);
        archive_.read(reinterpret_cast<char*>(&extraFieldLength), 2);
        archive_.read(reinterpret_cast<char*>(&fileCommentLength), 2);

        // Skip disk number start and internal attributes
        archive_.seekg(4, std::ios::cur);

        // Read external attributes
        archive_.read(reinterpret_cast<char*>(&entry.externalAttrs), 4);

        // Read local header offset
        archive_.read(reinterpret_cast<char*>(&entry.headerOffset), 4);

        // Read filename
        std::string filename(filenameLength, '\0');
        archive_.read(&filename[0], filenameLength);
        entry.filename = filename;

        // Skip extra field and file comment
        archive_.seekg(extraFieldLength + fileCommentLength, std::ios::cur);

        entries_.push_back(entry);
    }
}

void ArchiveReader::extractAll(const std::filesystem::path& outputDir,
                             bool overwriteAll) {
    for (const auto& entry : entries_) {
        extractFile(entry, outputDir, overwriteAll);
    }
}

void ArchiveReader::extractFile(const ZipEntry& entry,
                              const std::filesystem::path& outputDir,
                              bool overwriteAll) {
    auto outputPath = outputDir / entry.filename;

    // Create directory structure
    createDirectoryStructure(outputPath.parent_path());

    // Check if file exists and should be overwritten
    if (std::filesystem::exists(outputPath) && !overwriteAll) {
        if (!shouldOverwrite(outputPath)) {
            std::cout << "Skipping " << entry.filename << std::endl;
            return;
        }
    }

    // Read local file header
    archive_.seekg(entry.headerOffset);

    uint32_t signature;
    archive_.read(reinterpret_cast<char*>(&signature), 4);
    if (signature != ZIP_LOCAL_HEADER_SIGNATURE) {
        throw std::runtime_error("Invalid local file header");
    }

    // Skip to the compressed data
    archive_.seekg(26, std::ios::cur);  // Skip fixed-size fields
    
    uint16_t filenameLength;
    uint16_t extraFieldLength;
    archive_.read(reinterpret_cast<char*>(&filenameLength), 2);
    archive_.read(reinterpret_cast<char*>(&extraFieldLength), 2);
    
    archive_.seekg(filenameLength + extraFieldLength, std::ios::cur);

    // Read compressed data
    std::vector<uint8_t> compressedData(entry.compressedSize);
    archive_.read(reinterpret_cast<char*>(compressedData.data()),
                 entry.compressedSize);

    // Decompress data
    std::vector<uint8_t> decompressedData = compressor_->decompress(
        compressedData, entry.uncompressedSize);

    // Verify CRC32
    uint32_t crc = crc32(0L, decompressedData.data(), decompressedData.size());
    if (crc != entry.crc32) {
        throw std::runtime_error("CRC32 check failed for " + entry.filename);
    }

    // Write output file
    std::ofstream outFile(outputPath, std::ios::binary);
    if (!outFile) {
        throw std::runtime_error("Failed to create output file: " + outputPath.string());
    }

    outFile.write(reinterpret_cast<const char*>(decompressedData.data()),
                 decompressedData.size());
    outFile.close();

    // Set file permissions
    std::filesystem::permissions(outputPath,
        static_cast<std::filesystem::perms>(entry.externalAttrs >> 16));
}

std::vector<std::string> ArchiveReader::listFiles() const {
    std::vector<std::string> files;
    files.reserve(entries_.size());
    
    for (const auto& entry : entries_) {
        files.push_back(entry.filename);
    }
    
    return files;
}

bool ArchiveReader::shouldOverwrite(const std::filesystem::path& path) const {
    std::cout << "File already exists: " << path.string() << std::endl;
    std::cout << "Overwrite? (y/N/all): ";
    
    std::string response;
    std::getline(std::cin, response);
    
    if (response == "all") {
        return true;  // Caller should set overwriteAll = true
    }
    
    return response == "y" || response == "Y";
}

void ArchiveReader::createDirectoryStructure(const std::filesystem::path& path) const {
    if (!path.empty() && !std::filesystem::exists(path)) {
        std::filesystem::create_directories(path);
    }
}
} 