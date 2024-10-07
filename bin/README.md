# Utility Scripts for Experiments

## Usage

### Show Options

```bash
./bin/measure_mwcas.sh -h
```

### Run Benchmark with Specified Configurations

```bash
./bin/measure_mwcas.sh <bench_bin> <config> 1> results.csv 2> error.log
```

#### Example: Measure Throughput

```bash
./bin/measure_mwcas.sh ./build/mwcas_bench ./bin/bench.env 1> results.csv 2> error.log
```

#### Example: Measure Percentile Latency

```bash
./bin/measure_mwcas.sh -l ./build/mwcas_bench ./bin/bench.env 1> results.csv 2> error.log
```

## Configurations

### Parameters for Running Benchmark with Different Settings

- `THREAD_CANDIDATES`: The number of worker threads for executing MwCAS.
- `TARGET_CANDIDATES`: The number of target words of MwCAS.
- `SKEW_CANDIDATES`: A skew parameter in a Zipf distribution.
- `IMPL_CANDIDATES`: A competitor for MwCAS benchmark.

### Environment Settings

- `BENCH_REPEAT_COUNT`: The number of execution per setting.
- `OPERATION_COUNT`: The number of MwCAS operations per worker.
- `ARRAY_CAPACITY`: The number of words in a MwCAS target array.
- `TIMEOUT`: A timeout for each execution.
