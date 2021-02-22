# Simple MwCAS benchmark

![Unit Tests](https://github.com/dbgroup-nagoya-u/simple-mwcas-benchmark/workflows/Unit%20Tests/badge.svg?branch=main)

## Build

### Prerequisites

```bash
sudo apt update && sudo apt install -y build-essential cmake libgflags-dev libgoogle-glog-dev googletest
```

### Build Options

List build options.

- `BUILD_TESTS`: build unit tests for this repository if `on` (default: `off`).

### Build and Run Unit Tests

```bash
mkdir build && cd build
cmake -DCMAKE_BUILD_TYPE=Release -DBUILD_TESTS=on ..
make -j
ctest -C Release
```
