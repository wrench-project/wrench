#!/usr/bin/env bash
LOGOPTS="--wrench-full-log"

# sweep over host‐counts and invocation‐counts independently
for sched in random fcfs balance; do
  for hosts in 2 3 4 5 6 7 8 9 10 11 12 13 14 15 16 17 18 19 20 21 22 23 24 25 26 27 28 29 30 31 32; do
    OUTDIR=results/${sched}_${hosts}
      mkdir -p "$OUTDIR"
      echo "Running $sched with $hosts hosts..."
      ../../build/examples/serverless_api/wrench-example-serverless \
          $hosts \
          --scheduler=$sched \
          --invocations=500 \
          $LOGOPTS \
        > $OUTDIR/stdout.log 2>&1
  done
done
