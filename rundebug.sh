#!/bin/bash

./build/NULL/gem5.debug \
    --debug-flags=AttackPacketGenerator \
    --debug-file=debug.out \
    configs/example/garnet_synth_traffic.py  \
    --num-cpus=16 \
    --num-dirs=16 \
    --network=garnet2.0 \
    --topology=Mesh_XY \
    --mesh-rows=4  \
    --synthetic=uniform_random \
    --injectionrate=0.1 \
    --attack-node=0 \
    --attack-rate=0.2 \
    --attack-enabled \
    --bypass=bypass_none \
    --flit_jitter_threshold=40 \
#     --num-packets-max=1 \


