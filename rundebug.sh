#!/bin/bash

./build/NULL/gem5.debug \
    --debug-flags=Naive \
    --debug-file=debug.out \
    configs/example/garnet_synth_traffic.py  \
    --num-cpus=16 \
    --num-dirs=16 \
    --network=garnet2.0 \
    --topology=Mesh_XY \
    --mesh-rows=4  \
    --synthetic=uniform_random \
    --injectionrate=0.2 \
    --bypass=jitter_all \
    --num-packets-max=10 \
    --single-sender-id=0 \
    --single-dest-id=15 \
    --flit_jitter_threshold=20 \

#     --num-packets-max=1 \

