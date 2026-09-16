#!/usr/bin/env bash
set -e

echo "[+] Detecting Operating System..."
OS="$(uname -s)"

if [ "$OS" = "Darwin" ]; then
    echo "[+] Setting up environment for macOS..."
    if ! command -v brew &> /dev/null; then
        echo "[-] Homebrew not found. Installing..."
        /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
    fi
    brew update
    brew install cmake opencv onnxruntime
elif [ "$OS" = "Linux" ]; then
    echo "[+] Setting up environment for Linux..."
    sudo apt-get update && sudo apt-get install -y \
        build-essential \
        cmake \
        git \
        libopencv-dev \
        pkg-config
    
    echo "[+] Downloading ONNX Runtime Release..."
    wget -q https://github.com/microsoft/onnxruntime/releases/download/v1.16.3/onnxruntime-linux-x64-1.16.3.tgz
    tar -xzf onnxruntime-linux-x64-1.16.3.tgz
    sudo cp -r onnxruntime-linux-x64-1.16.3/include/* /usr/local/include/
    sudo cp -r onnxruntime-linux-x64-1.16.3/lib/* /usr/local/lib/
    sudo ldconfig
    rm -rf onnxruntime-linux-x64-1.16.3*
fi

echo "[+] Building binary..."
mkdir -p build && cd build
cmake ..
make -j$(nproc 2>/dev/null || sysctl -n hw.ncpu)

echo "[+] Build completed successfully! Binary located at build/pipeline_engine"
