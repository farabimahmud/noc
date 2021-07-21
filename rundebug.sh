#!/bin/bash

./build/NULL/gem5.debug \
    --debug-flags=Vanilla \
    --debug-file=debug.out \
    configs/example/garnet_synth_traffic.py  \
    --num-cpus=64 \
    --num-dirs=64 \
    --network=garnet2.0 \
    --sim-cycles=1000 \
    --topology=Mesh_XY \
    --mesh-rows=8  \
    --synthetic=uniform_random \
    --injectionrate=0.01 \
    --bypass=bypass_all_out \
    --attack-enabled \
    --attack-node=0 \
    --attack-rate=0.1 \
    --single-sender-id=0 \
    --fixed-target-enabled \
    --fixed-target-near=1 \
    --fixed-target-far=53 \
    --max-hpc=3 \
    --lower_limit=10 \
    --upper_limit=40 \
    --delta_s=4 \



    # --num-packets-max=1 \
    # --randomly-selected-targets \

    # --sim-cycles=10000 \

    # --debug-flags=AttackPacketGenerator \
    # --debug-file=debug.out \

