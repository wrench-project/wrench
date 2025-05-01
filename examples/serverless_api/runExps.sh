#!/usr/bin/env bash
LOGOPTS="--wrench-full-log"

for sched in random fcfs balance; do
  for inv in 1 10 20 50 100 250; do
    OUTDIR=results/${sched}_${inv}
    mkdir -p "$OUTDIR"
    echo "Running $sched with $inv invocations..."
    ../../build/examples/serverless_api/wrench-example-serverless \
        $inv \
        --scheduler=$sched \
        --invocations=100 \
        $LOGOPTS \
      > $OUTDIR/stdout.log 2>&1
  done
done
