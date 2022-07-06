#!/bin/bash

# usage: ./run_bench.sh [host] [port]

HOST="$1"
PORT="$2"
if [ -z ${HOST} ]; then
  HOST=127.0.0.1
fi
if [ -z ${PORT} ]; then
  PORT=9527
fi

for nc in 50 100 150 200 250 300; do
  ./bench.sh single-thread c="${nc}" $nc 1 "${HOST}" "${PORT}"
  echo "Done benchmarking single thread n_clients=${nc}"
done

for nt in 5 10 20 40 60 80 100; do
  ./bench.sh multi-threads t="${nt}" 50 $nt "${HOST}" "${PORT}"
  echo "Done benchmarking multi threads n_threads=${nt}"
done