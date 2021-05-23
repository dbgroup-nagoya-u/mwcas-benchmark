#!/bin/bash
set -ue

########################################################################################
# Documents
########################################################################################

BENCH_BIN=""
NUMA_NODES=""
WORKSPACE_DIR=$(cd $(dirname ${BASH_SOURCE:-${0}})/.. && pwd)

usage() {
  cat 1>&2 << EOS
Usage:
  ${BASH_SOURCE:-${0}} <path_to_bench_bin> 1> results.csv 2> error.log
Description:
  Run MwCAS benchmark to measure latency. All the benchmark results are output in CSV
  format. Note that all the benchmark settings are set via "config/bench.env".
Arguments:
  <path_to_bench_bin>: A path to the binary file for benchmarking.
Options:
  -N: Only execute benchmark on the CPUs of nodes. See "man numactl" for details.
  -h: Show this messsage and exit.
EOS
  exit 1
}

########################################################################################
# Parse options
########################################################################################

while getopts n:h OPT
do
  case ${OPT} in
    N) NUMA_NODES=${OPTARG}
      ;;
    h) usage
      ;;
    \?) usage
      ;;
  esac
done
shift $((${OPTIND} - 1))

########################################################################################
# Parse arguments
########################################################################################

if [ ${#} != 1 ]; then
  usage
fi

BENCH_BIN=${1}
if [ -n "${NUMA_NODES}" ]; then
  BENCH_BIN="numactl -N ${NUMA_NODES} -m ${NUMA_NODES} ${BENCH_BIN}"
fi

########################################################################################
# Run benchmark
########################################################################################

source ${WORKSPACE_DIR}/config/bench.env

for SKEW_PARAMETER in ${SKEW_CANDIDATES}; do
  for TARGET_NUM in ${TARGET_CANDIDATES}; do
    for THREAD_NUM in ${THREAD_CANDIDATES}; do
      for IMPL in ${IMPL_CANDIDATES}; do
        if [ ${IMPL} == 0 ]; then
          IMPL_ARGS="--ours=t --pmwcas=f --single=f"
        elif [ ${IMPL} == 1 ]; then
          IMPL_ARGS="--ours=f --pmwcas=t --single=f"
        elif [ ${TARGET_NUM} == 1 ]; then
          IMPL_ARGS="--ours=f --pmwcas=f --single=t"
        else
          continue
        fi
        for LOOP in {1..${BENCH_REPEAT_COUNT}}; do
          echo -n "${SKEW_PARAMETER},${TARGET_NUM},${IMPL},${THREAD_NUM},"
          ${BENCH_BIN} \
            --csv --throughput=f ${IMPL_ARGS} \
            --num_exec ${OPERATION_COUNT} --num_thread ${THREAD_NUM} \
            --num_target ${TARGET_NUM} --skew_parameter ${SKEW_PARAMETER}
          echo ""
        done
      done
    done
  done
done
