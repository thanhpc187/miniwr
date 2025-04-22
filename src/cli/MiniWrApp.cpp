#include "MiniWrApp.h"
#include <iostream>
#include <iomanip>
#include <chrono>
#include <thread>

namespace miniwr {

namespace {
    enum ExitCode {
        Success = 0,
        InvalidArguments = 1,
        FileError = 2,
        CompressionError = 3,
        DecompressionError = 4
    };
}

int MiniWrApp::run(int argc, char* argv[]) {
    try {
        Arguments args = ArgParser::parse(argc, argv);

        switch (args.command) {
            case Command::Add:
                return handleAdd(args);
            case Command::Extract:
                return handleExtract(args);
            default:
                throw std::runtime_error("Invalid command");
        }
    }
    catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << std::endl;
        return InvalidArguments;
    }
}

int MiniWrApp::handleAdd(const Arguments& args) {
    try {
        ArchiveWriter writer(args.archivePath);

        size_t totalFiles = 0;
        size_t processedFiles = 0;

        // Count total files first
        for (const auto& path : args.inputPaths) {
            if (std::filesystem::is_directory(path)) {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                    if (std::filesystem::is_regular_file(entry)) {
                        ++totalFiles;
                    }
                }
            }
            else if (std::filesystem::is_regular_file(path)) {
                ++totalFiles;
            }
        }

        // Process files
        for (const auto& path : args.inputPaths) {
            if (std::filesystem::is_directory(path)) {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(path)) {
                    if (std::filesystem::is_regular_file(entry)) {
                        showProgress("Compressing", ++processedFiles, totalFiles);
                        writer.addFile(entry.path(), args.compressionLevel);
                    }
                }
            }
            else if (std::filesystem::is_regular_file(path)) {
                showProgress("Compressing", ++processedFiles, totalFiles);
                writer.addFile(path, args.compressionLevel);
            }
        }

        writer.close();
        std::cout << "\nDone. " << processedFiles << " files compressed." << std::endl;
        return Success;
    }
    catch (const std::exception& e) {
        std::cerr << "Compression error: " << e.what() << std::endl;
        return CompressionError;
    }
}

int MiniWrApp::handleExtract(const Arguments& args) {
    try {
        ArchiveReader reader(args.archivePath);
        auto outputDir = args.outputDir.empty() ? std::filesystem::current_path() : args.outputDir;

        auto files = reader.listFiles();
        size_t totalFiles = files.size();
        size_t processedFiles = 0;

        std::cout << "Extracting " << totalFiles << " files to " << outputDir << std::endl;
        reader.extractAll(outputDir, args.force);

        std::cout << "\nDone. " << totalFiles << " files extracted." << std::endl;
        return Success;
    }
    catch (const std::exception& e) {
        std::cerr << "Decompression error: " << e.what() << std::endl;
        return DecompressionError;
    }
}

void MiniWrApp::showProgress(const std::string& operation,
                           size_t current,
                           size_t total) {
    const int barWidth = 50;
    float progress = static_cast<float>(current) / total;
    int pos = static_cast<int>(barWidth * progress);

    std::cout << "\r" << operation << ": [";
    for (int i = 0; i < barWidth; ++i) {
        if (i < pos) std::cout << "=";
        else if (i == pos) std::cout << ">";
        else std::cout << " ";
    }
    std::cout << "] " << std::fixed << std::setprecision(1)
              << (progress * 100.0) << "% (" << current << "/" << total << ")"
              << std::flush;

    if (current == total) {
        std::cout << std::endl;
    }
}
} 