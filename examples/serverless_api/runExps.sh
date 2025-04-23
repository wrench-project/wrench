#!/usr/bin/env bash
PLATFORM="/home/vince/github/wrench-serverless/build/examples/serverless_api/four_hosts.xml"
LOGOPTS="--wrench-full-log"

for sched in random fcfs balance; do
  for inv in 1 10 20 50 100 250; do
    OUTDIR=results/${sched}_${inv}
    mkdir -p "$OUTDIR"
    echo "Running $sched with $inv invocations..."
    /home/vince/github/wrench-serverless/build/examples/serverless_api/wrench-example-serverless $PLATFORM \
        --scheduler=$sched \
        --invocations=$inv \
        $LOGOPTS \
      > $OUTDIR/stdout.log 2>&1
  done
done
