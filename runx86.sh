#!/bin/bash

./build/X86_MESI_Two_Level/gem5.opt \
    --listener-mode=off \
    configs/example/se.py \
    --num-cpus=64 \
    --num-dirs=64 \
    --network=garnet \
    --topology=Mesh_XY \
    --mesh-rows=8 \
    --ruby \
    --l2cache \
    --num-l2caches=64 \
    --caches \
    -c /home/farabi/benchmarks/rodinia_3.0/openmp/backprop/backprop \
    -o 65536 \
#    --bypass=bypass_all_out \
#    --attack-enabled \
#    --attack-node=0 \
#    --attack-rate=0.01 \
#    --lower_limit=20 \
