#pragma once

#include "../core/Compressor.h"
#include <filesystem>
#include <string>
#include <vector>

namespace miniwr {

/**
 * @brief Command type enumeration
 */
enum class Command {
    Add,
    Extract,
    Help,
    Version,
    Invalid
};

/**
 * @brief Command line arguments structure
 */
struct Arguments {
    Command command = Command::Invalid;
    std::filesystem::path archivePath;
    std::vector<std::filesystem::path> inputPaths;
    std::filesystem::path outputDir;
    CompressionLevel compressionLevel = CompressionLevel::Default;
    bool force = false;
    int numThreads = 1;
};

/**
 * @brief Command line argument parser
 */
class ArgParser {
public:
    /**
     * @brief Parse command line arguments
     * @param argc Argument count
     * @param argv Argument values
     * @return Parsed arguments structure
     */
    static Arguments parse(int argc, char* argv[]);

    /**
     * @brief Print help message
     */
    static void printHelp();

    /**
     * @brief Print version information
     */
    static void printVersion();

private:
    static Command parseCommand(const std::string& cmd);
    static CompressionLevel parseCompressionLevel(const std::string& level);
}; 