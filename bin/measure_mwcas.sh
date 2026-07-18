#!/bin/bash

set -u

################################################################################
# Documents
################################################################################

BENCH_BIN=""
CONFIG_ENV=""
NUMA_NODES=""
readonly WORKSPACE_DIR=$(cd $(dirname ${BASH_SOURCE:-${0}})/.. && pwd)
readonly RANDOM_ID=$(cat /dev/urandom | base64 | tr -dc 'a-zA-Z0-9' | head -c 10)
readonly TMP_PATH="/tmp/mwcas_benchmark-$(id -un)-${RANDOM_ID}"

usage() {
  cat 1>&2 << EOS
Usage:
  ${BASH_SOURCE:-${0}} <bench_bin> <config> 1> results.csv 2> error.log
Description:
  Run benchmark to measure throughput/latency. All the benchmark results are
  output in CSV format.
Arguments:
  <bench_bin>: A path to a binary file for benchmarking.
  <config>: A path to a configuration file for benchmarking.
Options:
  -h: Show this message and exit.
  -n: Only execute benchmark on the CPUs of nodes. See "man numactl" for details.
EOS
  exit 1
}

################################################################################
# Parse options
################################################################################

while getopts n:lhtT: OPT
do
  case ${OPT} in
    n) NUMA_NODES=${OPTARG}
      ;;
    h) usage
      ;;
    \?) usage
      ;;
  esac
done
shift $((${OPTIND} - 1))

################################################################################
# Parse arguments
################################################################################

if [ ${#} != 2 ]; then
  usage
fi

BENCH_BIN=${1}
CONFIG_ENV=${2}

if [ ! -f "${BENCH_BIN}" ]; then
  echo "There is no specified benchmark binary."
  exit 1
fi
if [ ! -f "${CONFIG_ENV}" ]; then
  echo "There is no specified configuration file."
  exit 1
fi

if [ -n "${NUMA_NODES}" ]; then
  BENCH_BIN="numactl -N ${NUMA_NODES} -m ${NUMA_NODES} ${BENCH_BIN}"
fi

################################################################################
# Run benchmark
################################################################################

source "${CONFIG_ENV}"

for IMPL in ${IMPL_CANDIDATES}; do
  for SKEW_PARAMETER in ${SKEW_CANDIDATES}; do
    for TARGET_NUM in ${TARGET_CANDIDATES}; do
      for THREAD_NUM in ${THREAD_CANDIDATES}; do
        for LOOP in `seq ${BENCH_REPEAT_COUNT}`; do
          TMP_OUTPUT="${TMP_PATH}-output-$(date +%Y%m%d-%H%m%S-%N).csv"
          ${BENCH_BIN} \
          --${IMPL} \
          --csv \
          --num_thread ${THREAD_NUM} \
          --skew_parameter ${SKEW_PARAMETER} \
          --arr-cap ${ARRAY_CAPACITY} \
          --timeout ${TIMEOUT} \
          ${TARGET_NUM} \
          >> "${TMP_OUTPUT}"
          sed \
            "s/^/${IMPL},${TARGET_NUM},${SKEW_PARAMETER},${THREAD_NUM},/g" \
            "${TMP_OUTPUT}"
          rm -f "${TMP_OUTPUT}"
        done
      done
    done
  done
done
