#!/bin/bash

./build/X86_MESI_Two_Level/gem5.opt \
    --outdir=bypass \
    --listener-mode=off \
    --debug-flags=Flitisize \
    --debug-file=debug.out \
    configs/example/se.py \
    --num-cpus=64 \
    --num-dirs=64 \
    --network=garnet2.0 \
    --topology=Mesh_XY \
    --mesh-row=8 \
    --maxinsts=1000000 \
    --ruby \
    --caches \
    --l2cache \
    --num-l2caches=64 \
    --bypass=bypass_all_out \
    --rodinia-attack \
    --attack-enabled \
    --attack-node=63 \
    --max-hpc=5 \
    --upper-limit=80 \
    --cmd="/home/grads/f/farabi/benchmarks/rodinia_3.0/openmp/backprop/backprop;a.out" \
    --options="64 63" \
    --destination-list=0,63 \
    --target-latency=40 \
    --attack-rate=1 \
    --fast-forward=9223372036854775807 \

