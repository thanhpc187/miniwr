#pragma once

#include "Compressor.h"
#include <filesystem>
#include <fstream>
#include <memory>
#include <string>
#include <vector>

namespace miniwr {

struct ZipEntry;  // Forward declaration

/**
 * @brief ZIP archive reader
 */
class ArchiveReader {
public:
    explicit ArchiveReader(const std::filesystem::path& archivePath);
    ~ArchiveReader();

    /**
     * @brief Extract all files from the archive
     * @param outputDir Output directory path
     * @param overwriteAll If true, overwrite existing files without asking
     */
    void extractAll(const std::filesystem::path& outputDir,
                   bool overwriteAll = false);

    /**
     * @brief List all files in the archive
     * @return Vector of filenames
     */
    std::vector<std::string> listFiles() const;

private:
    std::filesystem::path archivePath_;
    std::ifstream archive_;
    std::unique_ptr<Compressor> compressor_;
    std::vector<ZipEntry> entries_;

    void readCentralDirectory();
    void extractFile(const ZipEntry& entry,
                    const std::filesystem::path& outputDir,
                    bool overwriteAll);
    bool shouldOverwrite(const std::filesystem::path& path) const;
    void createDirectoryStructure(const std::filesystem::path& path) const;
}; 