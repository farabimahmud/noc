#!/bin/bash

./build/NULL/gem5.debug \
    --debug-flags=Vanilla \
    --debug-file=debug.out \
    configs/example/garnet_synth_traffic.py  \
    --num-cpus=64 \
    --num-dirs=64 \
    --network=garnet2.0 \
    --sim-cycles=100000 \
    --topology=Mesh_XY \
    --mesh-rows=8  \
    --synthetic=uniform_random \
    --injectionrate=0.01 \
    --bypass=bypass_vanilla \
    --flit_jitter_threshold=40 \
    --attack-enabled \
    --attack-node=0 \
    --attack-rate=0.01 \
    --single-sender-id=0 \


    # --num-packets-max=1 \

    # --sim-cycles=10000 \

    # --debug-flags=AttackPacketGenerator \
    # --debug-file=debug.out \

