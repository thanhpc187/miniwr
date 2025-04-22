#pragma once

#include "ArgParser.h"
#include "../core/ArchiveWriter.h"
#include "../core/ArchiveReader.h"
#include <memory>

namespace miniwr {

/**
 * @brief Main application class implementing the Facade pattern
 */
class MiniWrApp {
public:
    /**
     * @brief Run the application with command line arguments
     * @param argc Argument count
     * @param argv Argument values
     * @return Exit code (0 = success)
     */
    static int run(int argc, char* argv[]);

private:
    static int handleAdd(const Arguments& args);
    static int handleExtract(const Arguments& args);
    static void showProgress(const std::string& operation,
                           size_t current,
                           size_t total);
}; 