#!/bin/bash

# Benchmark script for MiniWinRAR vs 7-Zip
# Requires: 7z, miniwr, sha256sum

SAMPLES_DIR="samples"
BENCH_DIR="bench"
RESULT_FILE="$BENCH_DIR/result.csv"

# Create directories
mkdir -p "$SAMPLES_DIR"
mkdir -p "$BENCH_DIR"

# Create test files if they don't exist
if [ ! -f "$SAMPLES_DIR/text.txt" ]; then
    dd if=/dev/urandom bs=1M count=5 | base64 > "$SAMPLES_DIR/text.txt"
fi

if [ ! -f "$SAMPLES_DIR/binary.dat" ]; then
    dd if=/dev/urandom bs=1M count=10 of="$SAMPLES_DIR/binary.dat"
fi

if [ ! -d "$SAMPLES_DIR/mixed" ]; then
    mkdir -p "$SAMPLES_DIR/mixed"
    cp "$SAMPLES_DIR/text.txt" "$SAMPLES_DIR/mixed/"
    cp "$SAMPLES_DIR/binary.dat" "$SAMPLES_DIR/mixed/"
    echo "Test directory" > "$SAMPLES_DIR/mixed/readme.txt"
fi

# Initialize results file
echo "File,Size,Tool,Level,CompressedSize,Ratio,Time,SHA256" > "$RESULT_FILE"

# Function to run compression test
run_test() {
    local file="$1"
    local tool="$2"
    local level="$3"
    local outfile="$4"
    
    # Get original size and hash
    local size=$(stat -f%z "$file")
    local orig_hash=$(sha256sum "$file" | cut -d' ' -f1)
    
    # Time compression
    local start=$(date +%s.%N)
    
    if [ "$tool" = "miniwr" ]; then
        ./miniwr a "$outfile" "$file" -m"$level"
    else
        7z a -tzip -mx="$level" "$outfile" "$file" > /dev/null
    fi
    
    local end=$(date +%s.%N)
    local time=$(echo "$end - $start" | bc)
    
    # Get compressed size and ratio
    local comp_size=$(stat -f%z "$outfile")
    local ratio=$(echo "scale=2; $comp_size * 100 / $size" | bc)
    
    # Extract and verify
    local temp_dir=$(mktemp -d)
    
    if [ "$tool" = "miniwr" ]; then
        ./miniwr x "$outfile" -C "$temp_dir" --force
    else
        7z x "$outfile" -o"$temp_dir" > /dev/null
    fi
    
    local extr_hash=$(cd "$temp_dir" && sha256sum $(basename "$file") | cut -d' ' -f1)
    rm -rf "$temp_dir"
    
    # Verify hash matches
    if [ "$orig_hash" != "$extr_hash" ]; then
        echo "ERROR: Hash mismatch for $file with $tool level $level"
        return 1
    fi
    
    # Record results
    echo "$file,$size,$tool,$level,$comp_size,$ratio,$time,$orig_hash" >> "$RESULT_FILE"
}

# Test each file with different compression levels
for file in "$SAMPLES_DIR/text.txt" "$SAMPLES_DIR/binary.dat" "$SAMPLES_DIR/mixed"; do
    for level in 0 1 6 9; do
        # Test with miniwr
        run_test "$file" "miniwr" "$level" "$BENCH_DIR/miniwr_test.zip"
        rm -f "$BENCH_DIR/miniwr_test.zip"
        
        # Test with 7z
        run_test "$file" "7z" "$level" "$BENCH_DIR/7z_test.zip"
        rm -f "$BENCH_DIR/7z_test.zip"
    done
done

echo "Benchmark complete. Results saved to $RESULT_FILE" 