#!/bin/bash

TEST_COMMANDS=set,get,incr,lpush,rpush,lpop,rpop,lrange,hset
N_REQUESTS=100000

OUTPUT_DIR="$1"
SUBFOLDER="$2"
N_CLIENTS="$3"
N_THREADS="$4"
HOST="$5"
PORT="$6"

echo "Output dir will be $OUTPUT_DIR/$SUBFOLDER"

STAT_FILE=stat
test -d $OUTPUT_DIR/$SUBFOLDER || mkdir -p $OUTPUT_DIR/$SUBFOLDER
echo -e 'n_clients='$N_CLIENTS >> $OUTPUT_DIR/$SUBFOLDER/$STAT_FILE
echo -e 'n_threads='$N_THREADS >> $OUTPUT_DIR/$SUBFOLDER/$STAT_FILE
echo -e 'n_requests='$N_REQUESTS >> $OUTPUT_DIR/$SUBFOLDER/$STAT_FILE
echo -e 'testing_commands='$TEST_COMMANDS >> $OUTPUT_DIR/$SUBFOLDER/$STAT_FILE

for i in 1 2 3 4 5
do
redis-benchmark -h "${HOST}" -p "${PORT}" -c $N_CLIENTS -n $N_REQUESTS -t $TEST_COMMANDS --threads $N_THREADS -r 10000 --csv > $OUTPUT_DIR/$SUBFOLDER/test-$i.csv
done