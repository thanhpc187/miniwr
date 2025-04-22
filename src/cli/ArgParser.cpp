#include "ArgParser.h"
#include <iostream>
#include <stdexcept>
#include <string>

namespace miniwr {

namespace {
    constexpr const char* VERSION = "1.0.0";
    constexpr const char* USAGE = R"(MiniWinRAR - Simple compression utility

Usage:
    miniwr a <archive.zip> <file|folder> [file2 ...] [-m0..9] [--threads N]
    miniwr x <archive.zip> [-C <dir_out>] [--force]
    miniwr --help
    miniwr --version

Commands:
    a     Add files/folders to archive
    x     Extract archive contents

Options:
    -m0..9        Set compression level (0=store, 9=max)
    -C <dir>      Extract to specified directory
    --force       Overwrite existing files without asking
    --threads N   Use N threads for compression (default: 1)
    --help        Show this help message
    --version     Show version information
)";
}

Arguments ArgParser::parse(int argc, char* argv[]) {
    if (argc < 2) {
        throw std::runtime_error("No command specified. Use --help for usage.");
    }

    std::string arg1 = argv[1];
    if (arg1 == "--help" || arg1 == "-h") {
        printHelp();
        std::exit(0);
    }
    if (arg1 == "--version" || arg1 == "-v") {
        printVersion();
        std::exit(0);
    }

    Arguments args;
    args.command = parseCommand(arg1);

    if (args.command == Command::Invalid) {
        throw std::runtime_error("Invalid command: " + arg1);
    }

    if (argc < 3) {
        throw std::runtime_error("Archive path not specified");
    }
    args.archivePath = argv[2];

    // Parse remaining arguments
    for (int i = 3; i < argc; ++i) {
        std::string arg = argv[i];

        if (arg.starts_with("-m")) {
            args.compressionLevel = parseCompressionLevel(arg.substr(2));
        }
        else if (arg == "-C" && i + 1 < argc) {
            args.outputDir = argv[++i];
        }
        else if (arg == "--force") {
            args.force = true;
        }
        else if (arg == "--threads" && i + 1 < argc) {
            args.numThreads = std::stoi(argv[++i]);
            if (args.numThreads < 1) {
                throw std::runtime_error("Number of threads must be >= 1");
            }
        }
        else if (args.command == Command::Add) {
            args.inputPaths.push_back(arg);
        }
    }

    // Validate arguments
    if (args.command == Command::Add && args.inputPaths.empty()) {
        throw std::runtime_error("No input files specified");
    }

    return args;
}

void ArgParser::printHelp() {
    std::cout << USAGE << std::endl;
}

void ArgParser::printVersion() {
    std::cout << "MiniWinRAR version " << VERSION << std::endl;
}

Command ArgParser::parseCommand(const std::string& cmd) {
    if (cmd == "a") return Command::Add;
    if (cmd == "x") return Command::Extract;
    return Command::Invalid;
}

CompressionLevel ArgParser::parseCompressionLevel(const std::string& level) {
    try {
        int value = std::stoi(level);
        if (value < 0 || value > 9) {
            throw std::runtime_error("Compression level must be between 0 and 9");
        }
        if (value == 0) return CompressionLevel::Store;
        if (value == 1) return CompressionLevel::Fast;
        if (value == 9) return CompressionLevel::Maximum;
        return CompressionLevel::Default;
    }
    catch (const std::exception&) {
        throw std::runtime_error("Invalid compression level: " + level);
    }
}
} 