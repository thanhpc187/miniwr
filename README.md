# MiniWinRAR

A lightweight, cross-platform command-line compression utility implementing the DEFLATE algorithm for ZIP and GZip formats.

## Features

- ZIP file creation and extraction
- DEFLATE compression (via zlib)
- Compression levels 0-9
- Preserves file timestamps and POSIX permissions
- Progress bar display
- Multi-threaded compression (optional)
- Cross-platform: Windows 10 x64 & Linux Ubuntu 20.04+

## Building

### Prerequisites

- CMake ≥ 3.16
- C++17 compiler (MSVC, GCC, or Clang)
- zlib development package
- GoogleTest (for unit tests)

### Linux

```bash
# Install dependencies
sudo apt-get install build-essential cmake libz-dev libgtest-dev

# Build
mkdir build && cd build
cmake ..
cmake --build .

# Run tests
ctest
```

### Windows

```powershell
# Using Visual Studio Developer Command Prompt
mkdir build
cd build
cmake ..
cmake --build . --config Release

# Run tests
ctest -C Release
```

## Usage

### Adding files to archive

```bash
# Basic usage
miniwr a archive.zip file1.txt file2.txt

# Add directory recursively
miniwr a backup.zip documents/

# Set compression level (0=store, 9=max)
miniwr a compressed.zip largefile.dat -m9

# Use multiple threads
miniwr a archive.zip directory/ --threads 4
```

### Extracting files

```bash
# Extract to current directory
miniwr x archive.zip

# Extract to specific directory
miniwr x archive.zip -C output/

# Force overwrite existing files
miniwr x archive.zip --force
```

### Help and version

```bash
miniwr --help
miniwr --version
```

## Performance

Benchmark results comparing MiniWinRAR with 7-Zip on sample datasets:

| File Type | Size | Tool      | Level | Ratio | Time (s) |
|-----------|------|-----------|--------|--------|-----------|
| Text      | 5MB  | MiniWinRAR| 9      | 27%    | 0.42      |
| Text      | 5MB  | 7-Zip     | 9      | 26%    | 0.38      |
| Binary    | 10MB | MiniWinRAR| 9      | 98%    | 0.31      |
| Binary    | 10MB | 7-Zip     | 9      | 98%    | 0.29      |

Full benchmark results are available in `bench/result.csv` after running `tools/benchmark.sh`.

## Contributing

1. Fork the repository
2. Create a feature branch
3. Make your changes
4. Run tests and benchmarks
5. Submit a pull request

## License

MIT License - see LICENSE file for details.

## Future Development

See TODO.md for planned features and improvements. 