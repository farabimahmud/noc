#!/bin/bash

./build/X86_MESI_Two_Level/gem5.opt \
    --debug-flag=Naive,RubyNetwork,RubySlicc \
    --debug-file=debug.out \
    configs/example/se.py \
    --num-cpus=16 \
    --num-dirs=16 \
    --network=garnet2.0 \
    --topology=Mesh_XY \
    --mesh-rows=4 \
    --ruby \
    --l2cache \
    --num-l2caches=16 \
    --caches \
    -c a.out \
    --bypass=optimized \
    --optimization_rate=0 \
