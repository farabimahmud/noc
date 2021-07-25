#!/bin/bash

./build/X86_MESI_Two_Level/gem5.opt \
    --debug-flag=Vanilla_X86 \
    --debug-file=debug.out \
    configs/example/se.py \
    --num-cpus=64 \
    --num-dirs=64 \
    --network=garnet2.0 \
    --topology=Mesh_XY \
    --mesh-rows=8 \
    --ruby \
    --l2cache \
    --num-l2caches=64 \
    --caches \
    -c attack.out \
    --bypass=bypass_none \
    --attack-enabled \
    --attack-node=0 \
    --attack-rate=0.01 \
