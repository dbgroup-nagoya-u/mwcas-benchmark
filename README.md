# MwCAS Benchmark

[![Ubuntu 24.04](https://github.com/dbgroup-nagoya-u/mwcas-benchmark/actions/workflows/ubuntu_24.yaml/badge.svg)](https://github.com/dbgroup-nagoya-u/mwcas-benchmark/actions/workflows/ubuntu_24.yaml) [![Ubuntu 22.04](https://github.com/dbgroup-nagoya-u/mwcas-benchmark/actions/workflows/ubuntu_22.yaml/badge.svg)](https://github.com/dbgroup-nagoya-u/mwcas-benchmark/actions/workflows/ubuntu_22.yaml)

- [Build](#build)
    - [Prerequisites](#prerequisites)
    - [Build Options](#build-options)
    - [Build and Run Unit Tests](#build-and-run-unit-tests)
- [Usage](#usage)
- [Acknowledgments](#acknowledgments)

## Build

### Prerequisites

```bash
sudo apt update && sudo apt install -y build-essential cmake libgflags-dev
cd <path_to_your_workspace>
git clone --recursive https://github.com/dbgroup-nagoya-u/mwcas-benchmark.git
cd mwcas-benchmark
```

### Build Options

#### Parameters for Benchmarking

- `MWCAS_BENCH_TARGET_NUM`: The maximum number of target words of MwCAS (default `8`).
- `MWCAS_BENCH_USE_PMWCAS`: A flag for using microsoft/pmwcas as a competitor (default `OFF`).
    - If you use microsoft/pmwcas, you need to install `libnuma-dev` by `apt`.
- `MWCAS_BENCH_USE_TBBMALLOC`: A flag for overriding entire memory allocation by Intel oneTBB malloc (default `OFF`).
    - You can set up  [Intel OneAPI Base Toolkit](https://www.intel.com/content/www/us/en/developer/tools/oneapi/base-toolkit.html) (i.e., Threading Building Blocks) by following [this instruction](https://www.intel.com/content/www/us/en/developer/tools/oneapi/base-toolkit-download.html?operatingsystem=linux&linux-install-type=apt). If you prefer to install oneTBB separately, you can use `intel-oneapi-tbb-devel` instead of `intel-basekit`.

#### Parameters for Unit Testing

- `MWCAS_BENCH_BUILD_TESTS`: build unit tests for this repository if `ON` (default `OFF`).

### Build and Run Unit Tests

```bash
mkdir build && cd build
cmake .. \
  -DCMAKE_BUILD_TYPE=Release \
  -DMWCAS_BENCH_BUILD_TESTS=ON
cmake --build . --parallel --config Release
ctest -C Release
```

## Usage

The following command displays available CLI options:

```bash
./build/mwcas_bench --helpshort
```

The benchmark program requires the number of target words.

```bash
./build/mwcas_bench --<competitor> <target_word_num>
```

For example, the following command performs 3wCAS benchmark with our MwCAS implementation.

```bash
./build/mwcas_bench --mwcas 3
```

We prepare scripts in `bin` directory to measure performance with a variety of parameters.

## Acknowledgments

This work is based on results obtained from project JPNP16007 commissioned by the New Energy and Industrial Technology Development Organization (NEDO). In addition, this work was supported partly by KAKENHI (16H01722 and 20K19804).
