#!/bin/bash
# Run script for Opus Ripper GUI

# Check if build directory exists
if [ ! -d "build" ]; then
    echo "Build directory not found. Creating and building..."
    mkdir build
    cd build
    cmake ..
    make -j$(nproc)
    cd ..
fi

# Check if executable exists
if [ ! -f "build/opus-ripper-gui" ]; then
    echo "Executable not found. Building..."
    cd build
    make -j$(nproc)
    cd ..
fi

# Run the application
echo "Starting Opus Ripper GUI..."
./build/opus-ripper-gui "$@"